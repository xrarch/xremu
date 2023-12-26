#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "lsic.h"
#include "ebus.h"

#define RS_USER   1
#define RS_INT    2
#define RS_MMU    4
#define RS_TBMISS 8
#define RS_LEGACY 128

#define RS_ECAUSE_SHIFT 28
#define RS_ECAUSE_MASK  15

#define PTE_VALID     1
#define PTE_WRITABLE  2
#define PTE_KERNEL    4
#define PTE_NONCACHED 8
#define PTE_GLOBAL    16

#define UNCACHEDSTALL  3
#define CACHEMISSSTALL (UNCACHEDSTALL+1) // per the MIPS R3000 as i read in a paper somewhere

#define signext23(n) (((int32_t)(n << 9)) >> 9)
#define signext18(n) (((int32_t)(n << 14)) >> 14)
#define signext5(n)  (((int32_t)(n << 27)) >> 27)
#define signext16(n) (((int32_t)(n << 16)) >> 16)

bool CPUSimulateCaches = true;
bool CPUSimulateCacheStalls = false;
bool CPUPrintCache = false;

int CPUStall = 0;

int CPUProgress;

uint32_t Reg[32];

enum Xr17032Registers {
	LR = 31,
};

uint32_t ControlReg[32];

enum Xr17032ControlRegisters {
	RS         = 0,
	WHAMI      = 1,
	EB         = 5,
	EPC        = 6,
	EBADADDR   = 7,
	TBMISSADDR = 9,
	TBPC       = 10,

	ITBPTE     = 16,
	ITBTAG     = 17,
	ITBINDEX   = 18,
	ITBCTRL    = 19,
	ICACHECTRL = 20,
	ITBADDR    = 21,

	DTBPTE     = 24,
	DTBTAG     = 25,
	DTBINDEX   = 26,
	DTBCTRL    = 27,
	DCACHECTRL = 28,
	DTBADDR    = 29,
};

enum Xr17032CacheTypes {
	NOCACHE  = 1,
	CACHE    = 0,
};

enum Xr17032Exceptions {
	EXCINTERRUPT = 1,
	EXCSYSCALL   = 2,
	EXCBUSERROR  = 4,
	
	EXCNMI       = 5,
	EXCBRKPOINT  = 6,
	EXCINVINST   = 7,
	EXCINVPRVG   = 8,
	EXCUNALIGNED = 9,

	EXCPAGEFAULT = 12,
	EXCPAGEWRITE = 13,

	EXCITLBMISS  = 14,
	EXCDTLBMISS  = 15,
};

bool UserBreak = false;
bool Halted = false;
bool Running = true;

uint32_t PC = 0;

uint32_t CurrentException;

bool IFetch = false;

#define CACHESIZELOG 14
#define CACHELINELOG 4 // WARNING: 16 bytes is special cased in CopyWithLength.
#define CACHEWAYLOG 1
#define CACHESETLOG (CACHESIZELOG-CACHELINELOG-CACHEWAYLOG)

#define CACHESIZE (1<<CACHESIZELOG)
#define CACHELINES (CACHESIZE>>CACHELINELOG)
#define CACHELINESIZE (1<<CACHELINELOG)
#define CACHESETS (1<<CACHESETLOG)
#define CACHEWAYS (1<<CACHEWAYLOG)

uint32_t ICacheTags[CACHELINES];
uint8_t ICache[CACHESIZE];

uint32_t DCacheTags[CACHELINES];
uint8_t DCache[CACHESIZE];

#define WRITEBUFFERLOG 2
#define WRITEBUFFERDEPTH (1 << WRITEBUFFERLOG)

uint32_t WriteBufferTags[WRITEBUFFERDEPTH];
uint8_t WriteBuffer[WRITEBUFFERDEPTH*CACHELINESIZE];

int WriteBufferWBIndex = 0;
int WriteBufferSize = 0;
int WriteBufferCyclesTilNextWrite = 0;

#define DTLBSIZELOG 5
#define DTLBSIZE (1<<DTLBSIZELOG)

#define ITLBSIZELOG 5
#define ITLBSIZE (1<<ITLBSIZELOG)

uint64_t DTlb[DTLBSIZE];
uint64_t ITlb[ITLBSIZE];

uint32_t CPULocked = 0;

static inline void Xr17032Exception(int exception) {
	if (CurrentException) {
		fprintf(stderr, "double exception, shouldnt ever happen");
		abort();
	}

	CurrentException = exception;
}

static inline uint32_t RoR(uint32_t x, uint32_t n) {
    return (x >> n & 31) | (x << (32-n) & 31);
}

uint32_t ITlbLastLookup = -1;
uint64_t ITlbLastResult = -1;

uint32_t DTlbLastLookup = -1;
uint64_t DTlbLastResult = -1;

bool TlbMissWrite = false;

static bool ITlbLookup(uint32_t virt, uint64_t *tlbe, bool writing) {
	uint32_t vpn = virt >> 12;
	uint32_t matching = (ControlReg[ITBTAG] & 0xFFF00000) | vpn;

	for (int i = 0; i < ITLBSIZE; i++) {
		uint64_t tbe = ITlb[i];

		uint32_t mask = (tbe & PTE_GLOBAL) ? 0xFFFFF : 0xFFFFFFFF;

		if (((tbe >> 32) & mask) == (matching & mask)) {
			*tlbe = tbe;

			return true;
		}
	}

	// ITLB miss! Set up some stuff to make it easier on the miss handler.

	if (!(ControlReg[RS] & RS_TBMISS)) {
		ControlReg[TBMISSADDR] = virt;
		TlbMissWrite = writing;
	}

	ControlReg[ITBTAG] = matching;
	ControlReg[ITBADDR] = (ControlReg[ITBADDR] & 0xFFC00000) | (vpn << 2);

	Xr17032Exception(EXCITLBMISS);

	return false;
}

static bool DTlbLookup(uint32_t virt, uint64_t *tlbe, bool writing) {
	uint32_t vpn = virt >> 12;
	uint32_t matching = (ControlReg[DTBTAG] & 0xFFF00000) | vpn;

	for (int i = 0; i < DTLBSIZE; i++) {
		uint64_t tbe = DTlb[i];

		uint32_t mask = (tbe & PTE_GLOBAL) ? 0xFFFFF : 0xFFFFFFFF;

		if (((tbe >> 32) & mask) == (matching & mask)) {
			*tlbe = tbe;

			return true;
		}
	}

	// DTLB miss! Set up some stuff to make it easier on the miss handler.

	if (!(ControlReg[RS] & RS_TBMISS)) {
		ControlReg[TBMISSADDR] = virt;
		TlbMissWrite = writing;
	}

	ControlReg[DTBTAG] = matching;
	ControlReg[DTBADDR] = (ControlReg[DTBADDR] & 0xFFC00000) | (vpn << 2);

	Xr17032Exception(EXCDTLBMISS);

	return false;
}

static inline bool CPUTranslate(uint32_t virt, uint32_t *phys, int *cachetype, bool writing) {
	uint64_t tlbe;
	uint32_t vpn = virt >> 12;

	if (IFetch) {
		if (ITlbLastLookup == vpn) {
			// This matches the last lookup, avoid searching the whole ITLB.

			tlbe = ITlbLastResult;
		} else {
			if (!ITlbLookup(virt, &tlbe, writing))
				return false;
		}
	} else {
		if (DTlbLastLookup == vpn) {
			// This matches the last lookup, avoid searching the whole DTLB.

			tlbe = DTlbLastResult;
		} else {
			if (!DTlbLookup(virt, &tlbe, writing))
				return false;
		}
	}

	if (!(tlbe & PTE_VALID)) {
		// Not valid! Page fault time.

		if (ControlReg[RS] & RS_TBMISS) {
			// This page fault happened while handling a TLB miss, which means
			// it was a fault on a page table. Clear the TBMISS flag from RS and
			// report the original missed address as the faulting address. Also,
			// reset the PC to point to the instruction after the one that
			// caused the original TLB miss, so that the faulting PC is reported
			// correctly. (in effect this is just
			// ControlReg[EPC] = ControlReg[TBPC] but it's done like this due to
			// the way the emulator receives exceptions).

			ControlReg[EBADADDR] = ControlReg[TBMISSADDR];
			PC = ControlReg[TBPC]+4;
			writing = TlbMissWrite;
		} else {
			ControlReg[EBADADDR] = virt;
		}

		Xr17032Exception(writing ? EXCPAGEWRITE : EXCPAGEFAULT);

		return false;
	}

	if ((tlbe & PTE_KERNEL) && (ControlReg[RS] & RS_USER)) {
		// Kernel mode page and we're in usermode! 

		ControlReg[EBADADDR] = virt;

		Xr17032Exception(writing ? EXCPAGEWRITE : EXCPAGEFAULT);

		return false;
	}

	if (writing && !(tlbe & PTE_WRITABLE)) {
		ControlReg[EBADADDR] = virt;

		Xr17032Exception(EXCPAGEWRITE);

		return false;
	}

	uint32_t physaddr = (tlbe & 0x1FFFFE0) << 7;
	int cached = (tlbe & PTE_NONCACHED) ? NOCACHE : CACHE;

	if (IFetch) {
		ITlbLastLookup = vpn;
		ITlbLastResult = tlbe;
	} else {
		DTlbLastLookup = vpn;
		DTlbLastResult = tlbe;
	}

	*cachetype = cached;
	*phys = physaddr + (virt & 0xFFF);

	//printf("virt=%x phys=%x\n", virt, *phys);

	return true;
}

uint32_t CacheFillCount = 0;
uint32_t ICacheFillCount = 0;

uint32_t CacheHitCount = 0;
uint32_t ICacheHitCount = 0;

#define NOCACHEAREA 0xC0000000

uint32_t AccessMasks[5] = {
	0x00000000,
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
	0xFFFFFFFF
};

static bool CPUAccess(uint32_t address, uint32_t *dest, uint32_t srcvalue, uint32_t length, bool writing) {
	if (address & (length - 1)) {
		Xr17032Exception(EXCUNALIGNED);

		ControlReg[EBADADDR] = address;

		return false;
	}

	int cachetype = CACHE;

	if (ControlReg[RS] & RS_MMU) {
		if (!CPUTranslate(address, &address, &cachetype, writing))
			return false;

	} else if (address >= NOCACHEAREA) {
		cachetype = NOCACHE;
	}

	if (!CPUSimulateCaches) {
		cachetype = NOCACHE;
	}

	if (cachetype == NOCACHE) {
		int result;

		if (writing) {
			result = EBusWrite(address, &srcvalue, length);
		} else {
			result = EBusRead(address, dest, length);
		}

		if (result == EBUSERROR) {
			ControlReg[EBADADDR] = address;

			Xr17032Exception(EXCBUSERROR);

			return false;
		}

		if (!writing)
			*dest &= AccessMasks[length];

		if (CPUSimulateCaches)
			CPUStall += UNCACHEDSTALL;
	} else {
		// simulate a writethrough icache and dcache,
		// along with a 4-entry writebuffer.

		uint32_t lineaddr = address & ~(CACHELINESIZE - 1);
		uint32_t lineoff = address & (CACHELINESIZE - 1);

		uint32_t lineno = address >> CACHELINELOG;

		uint32_t set = lineno & (CACHESETS - 1);

		uint32_t insertindex = -1;

		uint32_t *Tags = IFetch ? ICacheTags : DCacheTags;
		uint8_t *Cache = IFetch ? ICache : DCache;

		uint8_t *cacheline;
		uint32_t line = -1;

		// try to find the matching cache line.

		for (int i = 0; i < CACHEWAYS; i++) {
			if (Tags[set * CACHEWAYS + i] == lineaddr) {
				// found the cache line.

				line = set * CACHEWAYS + i;

				cacheline = &Cache[line * CACHELINESIZE];

				break;
			}

			// remember the index if this line is free.

			if (!Tags[set * CACHEWAYS + i])
				insertindex = i;
		}

		bool found = false;
		bool searched = false;
		uint8_t *wbaddr;

		if (line == -1) {
			// we didn't find the cache line, so we have to read it into the
			// cache. incur an appropriate stall.

			CPUStall += CACHEMISSSTALL;

			if (insertindex == -1) {
				line = set * CACHEWAYS + ((IFetch ? ICacheFillCount : CacheFillCount) & (CACHEWAYS - 1));
			} else {
				line = set * CACHEWAYS + insertindex;
			}

			cacheline = &Cache[line * CACHELINESIZE];
			insertindex = -1;

			if (!IFetch) {
				// this isn't an instruction fetch, so search the writebuffer
				// for a matching cache line.

				searched = true;

				for (int i = 0; i < WRITEBUFFERDEPTH; i++) {
					if (WriteBufferTags[i] == lineaddr) {
						// found the cache line in the writebuffer.

						insertindex = i;

						wbaddr = &WriteBuffer[i * CACHELINESIZE];

						found = true;

						break;
					} else if (WriteBufferTags[i] == 0) {
						// found a free write buffer index. remember this just
						// in case we need it later.

						insertindex = i;
					}
				}
			}

			if (found) {
				// found the cache line in the writebuffer, so copy it from
				// there into the cache.

				CopyWithLength(cacheline, wbaddr, CACHELINESIZE);
			} else {
				// did not find the cache line, so have to read it directly
				// over the bus.

				if (EBusRead(lineaddr, cacheline, CACHELINESIZE) == EBUSERROR) {
					ControlReg[EBADADDR] = address;

					Xr17032Exception(EXCBUSERROR);

					return false;
				}
			}

			Tags[line] = lineaddr;

			// do miss count accounting for cacheprint.

			if (IFetch)
				ICacheFillCount++;
			else
				CacheFillCount++;
		} else {
			// do hit count accounting for cacheprint.

			if (IFetch)
				ICacheHitCount++;
			else
				CacheHitCount++;

			insertindex = -1;
		}

		uint8_t *addr = &cacheline[lineoff];

		if (writing) {
			// this is a write operation. if we haven't already searched the
			// writebuffer for this cacheline, do it now since we will need to
			// overwrite the existing entry if one exists.

			if (!searched) {
				for (int i = 0; i < WRITEBUFFERDEPTH; i++) {
					if (WriteBufferTags[i] == lineaddr) {
						// found the cache line in the writebuffer.

						insertindex = i;

						wbaddr = &WriteBuffer[i * CACHELINESIZE];

						found = true;

						break;
					} else if (WriteBufferTags[i] == 0) {
						// found a free write buffer index. remember this just
						// in case we need it later.

						insertindex = i;
					}
				}
			}

			if (!found) {
				// didn't find our cacheline in the writebuffer, select a write
				// buffer entry to write out immediately and incur an
				// appropriate stall so that we can replace it with our
				// cacheline. if we have a saved insertindex, we can use that,
				// otherwise we select the entry based on the cacheline number.

				if (insertindex == -1) {
					// no saved insert index.

					insertindex = lineno & (WRITEBUFFERDEPTH-1);

					EBusWrite(WriteBufferTags[insertindex], &WriteBuffer[insertindex*CACHELINESIZE], CACHELINESIZE);

					CPUStall += UNCACHEDSTALL;
				} else {
					// there was a saved insert index, so increment the write
					// buffer size since we are replacing a free entry.

					WriteBufferSize++;
				}

				wbaddr = &WriteBuffer[insertindex * CACHELINESIZE];

				WriteBufferTags[insertindex] = lineaddr;

				CopyWithLength(wbaddr, cacheline, CACHELINESIZE);
			}

			wbaddr = &wbaddr[lineoff];

			switch (length) {
				case 1:
					*addr = (uint8_t)srcvalue;
					*wbaddr = (uint8_t)srcvalue;
					break;

				case 2:
					*(uint16_t*)addr = (uint16_t)srcvalue;
					*(uint16_t*)wbaddr = (uint16_t)srcvalue;
					break;

				case 4:
					*(uint32_t*)addr = srcvalue;
					*(uint32_t*)wbaddr = srcvalue;
					break;
			}

			// if a writebuffer write isn't pending, set the timer.

			if (!WriteBufferCyclesTilNextWrite)
				WriteBufferCyclesTilNextWrite = UNCACHEDSTALL;
		} else {
			// this is a read operation, just copy the value.

			switch (length) {
				case 1:
					*dest = *addr;
					break;

				case 2:
					*dest = *(uint16_t*)addr;
					break;

				case 4:
					*dest = *(uint32_t*)addr;
					break;
			}
		}
	}

	return true;
}

#define CPUReadByte(address, value) CPUAccess(address, value, 0, 1, false);
#define CPUReadInt(address, value)  CPUAccess(address, value, 0, 2, false);
#define CPUReadLong(address, value) CPUAccess(address, value, 0, 4, false);

#define CPUWriteByte(address, value) CPUAccess(address, 0, value, 1, true);
#define CPUWriteInt(address, value)  CPUAccess(address, 0, value, 2, true);
#define CPUWriteLong(address, value) CPUAccess(address, 0, value, 4, true);

void CPUReset() {
	PC = 0xFFFE1000;
	ControlReg[RS] = 0;
	ControlReg[EB] = 0;
	ControlReg[ICACHECTRL] = (CACHELINELOG << 16) | (CACHEWAYLOG << 8) | (CACHESIZELOG - CACHELINELOG);
	ControlReg[DCACHECTRL] = (CACHELINELOG << 16) | (CACHEWAYLOG << 8) | (CACHESIZELOG - CACHELINELOG);
	ControlReg[ITBCTRL] = ITLBSIZELOG;
	ControlReg[DTBCTRL] = DTLBSIZELOG;
	ControlReg[WHAMI] = 0;
	CurrentException = 0;
}

int TimeToNextPrint = 2000;

uint32_t CPUDoCycles(uint32_t cycles, uint32_t dt) {
	if (!Running)
		return cycles;

	if (UserBreak && !CurrentException) {
		// there's a pending user-initiated NMI, so do that.

		Xr17032Exception(EXCNMI);

		UserBreak = false;
	}

	if (CPUPrintCache) {
		TimeToNextPrint -= dt;

		if (TimeToNextPrint <= 0) {
			// it's time to print some cache statistics.

			int itotal = ICacheFillCount+ICacheHitCount;
			int dtotal = CacheFillCount+CacheHitCount;

			printf("icache misses: %d (%.2f%% miss rate)\n", ICacheFillCount, (double)ICacheFillCount/(double)itotal*100.0);
			printf("dcache misses: %d (%.2f%% miss rate)\n", CacheFillCount, (double)CacheFillCount/(double)dtotal*100.0);

			CacheFillCount = 0;
			ICacheFillCount = 0;

			CacheHitCount = 0;
			ICacheHitCount = 0;

			TimeToNextPrint = 2000;
		}
	}

	if (Halted) {
		// if there's an exception (such as an NMI), or interrupts are enabled
		// and there is an interrupt pending, then unhalt the processor and
		// continue execution.

		if (CurrentException || ((ControlReg[RS] & RS_INT) && LSICInterruptPending)) {
			Halted = false;
		} else {
			return cycles;
		}
	}

	uint32_t cyclesdone = 0;

	uint32_t newstate;
	uint32_t evec;
	uint32_t currentpc;
	uint32_t ir;
	uint32_t maj;
	uint32_t majoropcode;
	uint32_t funct;
	uint32_t shift;
	uint32_t shifttype;
	uint32_t val;
	uint32_t rd;
	uint32_t ra;
	uint32_t rb;
	uint32_t imm;

	int status;

	for (; cyclesdone < cycles; cyclesdone++) {
		// make sure the zero register is always zero, except during TLB misses,
		// where it may be used as a scratch register.

		if (!(ControlReg[RS] & RS_TBMISS))
			Reg[0] = 0;

		if (CPUProgress <= 0) {
			// the CPU did a poll-y looking thing too many times this tick.
			// skip the rest of the tick so as not to eat up too much of the
			// host's CPU.

			return cycles;
		}

		if (CPUSimulateCacheStalls && CPUStall) {
			// there's a simulated cache stall of some number of cycles, so
			// decrement the remaining stall and return.

			CPUStall--;

			continue;
		}

		if (CPUSimulateCaches) {
			if (WriteBufferCyclesTilNextWrite && (!--WriteBufferCyclesTilNextWrite)) {
				bool found = false;
				uint32_t i;

				// it's time to write an entry out of the write buffer.
				// first search from the next index to the end.

				for (i = WriteBufferWBIndex; i < WRITEBUFFERDEPTH; i++) {
					if (WriteBufferTags[i]) {
						found = true;
						break;
					}
				}

				// if we didn't find a pending entry, search from the beginning
				// to the next index.

				if (!found) {
					for (i = 0; i < WriteBufferWBIndex; i++) {
						if (WriteBufferTags[i]) {
							found = true;
							break;
						}
					}
				}

				// if we found a pending entry, write it out, and update the
				// index for the next write-out. set up CyclesTilNextWrite to
				// indicate when to write the next entry.

				if (found) {
					EBusWrite(WriteBufferTags[i], &WriteBuffer[i * CACHELINESIZE], CACHELINESIZE);

					WriteBufferTags[i] = 0;

					WriteBufferWBIndex = (i + 1) & (WRITEBUFFERDEPTH - 1);

					if (WriteBufferSize--)
						WriteBufferCyclesTilNextWrite = UNCACHEDSTALL;
				}
			}
		}

		if (CurrentException || ((ControlReg[RS] & RS_INT) && LSICInterruptPending)) {
			// there's a pending exception, or interrupts are enabled and
			// there is a pending interrupt. figure out where to send the
			// program counter off to, and what to set the status register bits
			// to.

			if (ControlReg[EB] == 0) {
				// there is no exception block. reset the CPU.

				CurrentException = 0;
				CPUReset();
				continue;
			}

			// if CurrentException is null, then we must have come here
			// because of an interrupt.

			if (!CurrentException)
				CurrentException = EXCINTERRUPT;

			// enter kernel mode, disable interrupts.
			newstate = ControlReg[RS] & 0xFC;

			if (newstate & RS_LEGACY) {
				// legacy exceptions are enabled, so disable virtual
				// addressing. this is NOT part of the "official" xr17032
				// architecture and is a hack to continue running AISIX in
				// emulation.

				newstate &= ~RS_MMU;
			}

			// this is a general exception, such as a page fault, hardware
			// interrupt, or syscall, so keep virtual addressing enabled
			// and vector through EB.

			evec = ControlReg[EB] | (CurrentException << 8);

			switch(CurrentException) {
				case EXCSYSCALL:
				case EXCINTERRUPT:
					ControlReg[EPC] = PC;
					break;

				case EXCDTLBMISS:
				case EXCITLBMISS:
					// don't overwrite any state if this is a nested TLB miss,
					// since we (by design) should just jump back to the start
					// of the miss handler to deal with the new problem. then,
					// we return to the original faulting instruction, which
					// re-takes its original TLB miss, which completes
					// successfully that time.

					//printf("miss! %d dtbtag=%x itbtag=%x dtbaddr=%x itbaddr=%x tbmissaddr=%x\n", CurrentException, ControlReg[DTBTAG], ControlReg[ITBTAG], ControlReg[DTBADDR], ControlReg[ITBADDR], ControlReg[TBMISSADDR]);

					if (!(ControlReg[RS] & RS_TBMISS)) {
						CurrentException = ControlReg[RS] >> RS_ECAUSE_SHIFT;
						ControlReg[TBPC] = PC-4;
						newstate |= RS_TBMISS;
					}

					break;

				default:
					ControlReg[EPC] = PC-4;
					break;
			}

			// set the program counter to the selected vector, and set the
			// new status register bits.

			//printf("evec %x %x %x\n", evec, ControlReg[RS], newstate);

			PC = evec;

			if (ControlReg[RS] & RS_TBMISS) {
				if (CurrentException == EXCPAGEFAULT || CurrentException == EXCPAGEWRITE) {
					// this was a page fault within a TLB miss, so just clear
					// the TBMISS flag.

					ControlReg[RS] &= ~RS_TBMISS;
					ControlReg[RS] &= 0x0FFFFFFF;
					ControlReg[RS] |= (CurrentException << RS_ECAUSE_SHIFT);
				}
			} else {
				ControlReg[RS] = (CurrentException << RS_ECAUSE_SHIFT) | ((ControlReg[RS] & 0xFFFF)<<8) | newstate;
			}

			CurrentException = 0;
		}

		currentpc = PC;

		PC += 4;

		IFetch = true;

		status = CPUReadLong(currentpc, &ir);

		IFetch = false;

		if (!status) {
			continue;
		}

		// fetch was successful, decode the instruction word and execute
		// the instruction.

		maj = ir & 7;
		majoropcode = ir & 63;

		if (maj == 7) { // JAL
			Reg[LR] = PC;
			PC = (currentpc & 0x80000000) | ((ir >> 3) << 2);
		} else if (maj == 6) { // J
			PC = (currentpc & 0x80000000) | ((ir >> 3) << 2);
		} else if (majoropcode == 57) { // reg instructions 111001
			funct = ir >> 28;

			shifttype = (ir >> 26) & 3;
			shift = (ir >> 21) & 31;

			rd = (ir >> 6) & 31;
			ra = (ir >> 11) & 31;
			rb = (ir >> 16) & 31;

			if (shift) {
				switch(shifttype) {
					case 0: // LSH
						val = Reg[rb] << shift;
						break;

					case 1: // RSH
						val = Reg[rb] >> shift;
						break;

					case 2: // ASH
						val = (int32_t) Reg[rb] >> shift;
						break;

					case 3: // ROR
						val = RoR(Reg[rb], shift);
						break;
				}
			} else {
				val = Reg[rb];
			}

			switch(funct) {
				case 0: // NOR
					Reg[rd] = ~(Reg[ra] | val);
					break;

				case 1: // OR
					Reg[rd] = Reg[ra] | val;
					break;

				case 2: // XOR
					Reg[rd] = Reg[ra] ^ val;
					break;

				case 3: // AND
					Reg[rd] = Reg[ra] & val;
					break;

				case 4: // SLT SIGNED
					if ((int32_t) Reg[ra] < (int32_t) val)
						Reg[rd] = 1;
					else
						Reg[rd] = 0;
					break;

				case 5: // SLT
					if (Reg[ra] < val)
						Reg[rd] = 1;
					else
						Reg[rd] = 0;
					break;

				case 6: // SUB
					Reg[rd] = Reg[ra] - val;
					break;

				case 7: // ADD
					Reg[rd] = Reg[ra] + val;
					break;

				case 8: // *SH
					switch(shifttype) {
						case 0: // LSH
							Reg[rd] = Reg[rb] << Reg[ra];
							break;

						case 1: // RSH
							Reg[rd] = Reg[rb] >> Reg[ra];
							break;

						case 2: // ASH
							Reg[rd] = (int32_t) Reg[rb] >> Reg[ra];
							break;

						case 3: // ROR
							Reg[rd] = RoR(Reg[rb], Reg[ra]);
							break;
					}
					break;

				case 9: // MOV LONG, RD
					CPUWriteLong(Reg[ra] + val, Reg[rd]);
					break;

				case 10: // MOV INT, RD
					CPUWriteInt(Reg[ra] + val, Reg[rd]);
					break;

				case 11: // MOV BYTE, RD
					CPUWriteByte(Reg[ra] + val, Reg[rd]);
					break;

				case 12: // invalid
					Xr17032Exception(EXCINVINST);
					break;

				case 13: // MOV RD, LONG
					CPUReadLong(Reg[ra] + val, &Reg[rd]);
					break;

				case 14: // MOV RD, INT
					CPUReadInt(Reg[ra] + val, &Reg[rd]);
					break;

				case 15: // MOV RD, BYTE
					CPUReadByte(Reg[ra] + val, &Reg[rd]);
					break;

				default: // unreachable
					abort();
			}
		} else if (majoropcode == 49) { // reg instructions 110001
			funct = ir >> 28;

			rd = (ir >> 6) & 31;
			ra = (ir >> 11) & 31;
			rb = (ir >> 16) & 31;

			switch(funct) {
				case 0: // SYS
					Xr17032Exception(EXCSYSCALL);
					break;

				case 1: // BRK
					Xr17032Exception(EXCBRKPOINT);
					break;

				case 2: // WMB
				case 3: // MB
					if (CPUSimulateCaches) {
						// flush writebuffer

						for (int i = 0; i < WRITEBUFFERDEPTH; i++) {
							if (WriteBufferTags[i] == 0)
								continue;

							// flush this entry

							WriteBufferTags[i] = 0;
							EBusWrite(WriteBufferTags[i], &WriteBuffer[i * CACHELINESIZE], CACHELINESIZE);

							// incur a stall

							CPUStall += UNCACHEDSTALL;
						}

						WriteBufferSize = 0;
						WriteBufferCyclesTilNextWrite = 0;
					}

					break;

				case 8: // SC
					if (CPULocked)
						CPUWriteLong(Reg[ra], Reg[rb]);

					Reg[rd] = CPULocked;

					break;

				case 9: // LL
					CPULocked = 1;

					CPUReadLong(Reg[ra], &Reg[rd]);

					break;

				case 11: // MOD
					if (Reg[rb] == 0) {
						Reg[rd] = 0;
						break;
					}

					Reg[rd] = Reg[ra] % Reg[rb];
					break;

				case 12: // DIV SIGNED
					if (Reg[rb] == 0) {
						Reg[rd] = 0;
						break;
					}

					Reg[rd] = (int32_t) Reg[ra] / (int32_t) Reg[rb];
					break;

				case 13: // DIV
					if (Reg[rb] == 0) {
						Reg[rd] = 0;
						break;
					}

					Reg[rd] = Reg[ra] / Reg[rb];
					break;

				case 15: // MUL
					Reg[rd] = Reg[ra] * Reg[rb];
					break;

				default:
					Xr17032Exception(EXCINVINST);
					break;
			}
		} else if (majoropcode == 41) { // privileged instructions 101001
			if (ControlReg[RS] & RS_USER) {
				// current mode is usermode, cause a privilege violation
				// exception.

				Xr17032Exception(EXCINVPRVG);
			} else {
				funct = ir >> 28;

				rd = (ir >> 6) & 31;
				ra = (ir >> 11) & 31;
				rb = (ir >> 16) & 31;

				uint32_t asid;
				uint32_t vpn;
				uint32_t index;
				uint64_t tlbe;
				uint32_t pde;
				uint32_t tbhi;

				switch(funct) {
					case 11: // RFE
						CPULocked = 0;

						if (ControlReg[RS] & RS_TBMISS) {
							PC = ControlReg[TBPC];
						} else {
							PC = ControlReg[EPC];
						}

						ControlReg[RS] = (ControlReg[RS] & 0xF0000000) | ((ControlReg[RS] >> 8) & 0xFFFF);
						//printf("rfe rs=%x\n", ControlReg[RS]);

						break;

					case 12: // HLT
						Halted = true;
						break;

					case 14: // MTCR
						switch(rb) {
							case ICACHECTRL:
								if (!CPUSimulateCaches)
									break;

								if ((Reg[ra] & 3) == 3) {
									// invalidate the entire icache.

									for (int i = 0; i < CACHELINES; i++) {
										ICacheTags[i] = 0;
									}
								} else if ((Reg[ra] & 3) == 2) {
									// invalidate a single page frame of the
									// icache.

									uint32_t phys = Reg[ra] & 0xFFFFF000;

									for (int i = 0; i < CACHELINES; i++) {
										if ((ICacheTags[i] & 0xFFFFF000) == phys)
											ICacheTags[i] = 0;
									}
								}

								break;

							case DCACHECTRL:
								if (!CPUSimulateCaches)
									break;

								if ((Reg[ra] & 3) == 3) {
									// invalidate the entire dcache.

									for (int i = 0; i < CACHELINES; i++) {
										DCacheTags[i] = 0;
									}
								} else if ((Reg[ra] & 3) == 2) {
									// invalidate a single page frame of the
									// dcache.

									uint32_t phys = Reg[ra] & 0xFFFFF000;

									for (int i = 0; i < CACHELINES; i++) {
										if ((DCacheTags[i] & 0xFFFFF000) == phys)
											DCacheTags[i] = 0;
									}
								}

								break;

							case ITBCTRL:
								if ((Reg[ra] & 3) == 3) {
									// invalidate the entire ITLB.

									for (int i = 0; i < ITLBSIZE; i++) {
										ITlb[i] = 0;
									}
								} else if ((Reg[ra] & 3) == 2) {
									// invalidate the entire ITLB except for
									// global entries.

									for (int i = 0; i < ITLBSIZE; i++) {
										if (!(ITlb[i] & PTE_GLOBAL))
											ITlb[i] = 0;
									}
								} else if ((Reg[ra] & 3) == 0) {
									// invalidate a single page in the ITLB.

									uint64_t vpn = (uint64_t)(Reg[ra] >> 12) << 32;

									for (int i = 0; i < ITLBSIZE; i++) {
										if ((ITlb[i] & 0xFFFFF00000000) == vpn)
											ITlb[i] = 0;
									}

									//printf("invl %x\n", Reg[ra] >> 12);

									//Running = false;
								}

								// Reset the lookup hint.

								ITlbLastLookup = -1;

								break;

							case DTBCTRL:
								if ((Reg[ra] & 3) == 3) {
									// invalidate the entire DTLB.

									for (int i = 0; i < DTLBSIZE; i++) {
										DTlb[i] = 0;
									}
								} else if ((Reg[ra] & 3) == 2) {
									// invalidate the entire DTLB except for
									// global entries.

									for (int i = 0; i < DTLBSIZE; i++) {
										if (!(DTlb[i] & PTE_GLOBAL))
											DTlb[i] = 0;
									}
								} else if ((Reg[ra] & 3) == 0) {
									// invalidate a single page in the DTLB.

									uint64_t vpn = (uint64_t)(Reg[ra] >> 12) << 32;

									for (int i = 0; i < DTLBSIZE; i++) {
										if ((DTlb[i] & 0xFFFFF00000000) == vpn)
											DTlb[i] = 0;
									}
								}

								// Reset the lookup hint.

								DTlbLastLookup = -1;

								break;

							case ITBPTE:
								// Write an entry to the ITLB at ITBINDEX, and
								// increment it.

								ITlb[ControlReg[ITBINDEX]] = ((uint64_t)(ControlReg[ITBTAG]) << 32) | Reg[ra];

								//printf("ITB[%d] = %llx\n", ControlReg[ITBINDEX], ITlb[ControlReg[ITBINDEX]]);

								ControlReg[ITBINDEX] += 1;

								if (ControlReg[ITBINDEX] == ITLBSIZE) {
									// Roll over to index four.

									ControlReg[ITBINDEX] = 4;
								}

								break;

							case DTBPTE:
								// Write an entry to the DTLB at DTBINDEX, and
								// increment it.

								DTlb[ControlReg[DTBINDEX]] = ((uint64_t)(ControlReg[DTBTAG]) << 32) | Reg[ra];

								//printf("DTB[%d] = %llx\n", ControlReg[DTBINDEX], DTlb[ControlReg[DTBINDEX]]);

								ControlReg[DTBINDEX] += 1;

								if (ControlReg[DTBINDEX] == DTLBSIZE) {
									// Roll over to index four.

									ControlReg[DTBINDEX] = 4;
								}

								break;

							case ITBINDEX:
								ControlReg[ITBINDEX] = Reg[ra] & (ITLBSIZE - 1);
								//printf("ITBX = %x\n", ControlReg[ITBINDEX]);
								break;

							case DTBINDEX:
								ControlReg[DTBINDEX] = Reg[ra] & (DTLBSIZE - 1);
								//printf("DTBX = %x\n", ControlReg[DTBINDEX]);
								break;

							case DTBTAG:
								ControlReg[DTBTAG] = Reg[ra];

								// Reset the lookup hint.

								DTlbLastLookup = -1;

								break;

							case ITBTAG:
								ControlReg[ITBTAG] = Reg[ra];

								// Reset the lookup hint.

								ITlbLastLookup = -1;
								
								break;

							default:
								ControlReg[rb] = Reg[ra];
								break;
						}

					case 15: // MFCR
						Reg[rd] = ControlReg[rb];
						break;

					default:
						Xr17032Exception(EXCINVINST);
						break;
				}
			}
		} else { // major opcodes
			rd = (ir >> 6) & 31;
			ra = (ir >> 11) & 31;
			imm = ir >> 16;

			switch(majoropcode) {
				// branches
				
				case 61: // BEQ
					if (Reg[rd] == 0)
						PC = currentpc + signext23((ir >> 11) << 2);

					break;

				case 53: // BNE
					if (Reg[rd] != 0)
						PC = currentpc + signext23((ir >> 11) << 2);

					break;

				case 45: // BLT
					if ((int32_t) Reg[rd] < 0)
						PC = currentpc + signext23((ir >> 11) << 2);

					break;

				case 37: // BGT
					if ((int32_t) Reg[rd] > 0)
						PC = currentpc + signext23((ir >> 11) << 2);

					break;

				case 29: // BLE
					if ((int32_t) Reg[rd] <= 0)
						PC = currentpc + signext23((ir >> 11) << 2);

					break;

				case 21: // BGE
					if ((int32_t) Reg[rd] >= 0)
						PC = currentpc + signext23((ir >> 11) << 2);

					break;

				case 13: // BPE
					if ((Reg[rd]&1) == 0)
						PC = currentpc + signext23((ir >> 11) << 2);

					break;

				case 5: // BPO
					if (Reg[rd]&1)
						PC = currentpc + signext23((ir >> 11) << 2);

					break;

				// ALU

				case 60: // ADDI
					Reg[rd] = Reg[ra] + imm;

					break;

				case 52: // SUBI
					Reg[rd] = Reg[ra] - imm;

					break;

				case 44: // SLTI
					if (Reg[ra] < imm)
						Reg[rd] = 1;
					else
						Reg[rd] = 0;

					break;

				case 36: // SLTI signed
					if ((int32_t) Reg[ra] < (int32_t) signext16(imm))
						Reg[rd] = 1;
					else
						Reg[rd] = 0;

					break;

				case 28: // ANDI
					Reg[rd] = Reg[ra] & imm;

					break;

				case 20: // XORI
					Reg[rd] = Reg[ra] ^ imm;

					break;

				case 12: // ORI
					Reg[rd] = Reg[ra] | imm;

					break;

				case 4: // LUI
					Reg[rd] = Reg[ra] | (imm << 16);

					break;

				// LOAD with immediate offset

				case 59: // MOV RD, BYTE
					CPUReadByte(Reg[ra] + imm, &Reg[rd]);

					break;

				case 51: // MOV RD, INT
					CPUReadInt(Reg[ra] + (imm << 1), &Reg[rd]);

					break;

				case 43: // MOV RD, LONG
					CPUReadLong(Reg[ra] + (imm << 2), &Reg[rd]);

					break;

				// STORE with immediate offset

				case 58: // MOV BYTE RD+IMM, RA
					CPUWriteByte(Reg[rd] + imm, Reg[ra]);
					break;

				case 50: // MOV INT RD+IMM, RA
					CPUWriteInt(Reg[rd] + (imm << 1), Reg[ra]);
					break;

				case 42: // MOV LONG RD+IMM, RA
					CPUWriteLong(Reg[rd] + (imm << 2), Reg[ra]);
					break;

				case 26: // MOV BYTE RD+IMM, IMM5
					CPUWriteByte(Reg[rd] + imm, signext5(ra));
					break;

				case 18: // MOV INT RD+IMM, IMM5
					CPUWriteInt(Reg[rd] + (imm << 1), signext5(ra));
					break;

				case 10: // MOV LONG RD+IMM, IMM5
					CPUWriteLong(Reg[rd] + (imm << 2), signext5(ra));
					break;

				case 56: // JALR
					Reg[rd] = PC;

					PC = Reg[ra] + signext18(imm << 2);

					break;

				default:
					Xr17032Exception(EXCINVINST);
					break;
			}
		}

		if (Halted || (!Running))
			return cyclesdone;
	}

	return cyclesdone;
}

void TLBDump(void) {
	printf("ITLB:\n");

	for (int i = 0; i < ITLBSIZE; i++) {
		printf("%d: %016llx\n", i, ITlb[i]);
	}

	printf("\n");

	printf("DTLB:\n");

	for (int i = 0; i < DTLBSIZE; i++) {
		printf("%d: %016llx\n", i, DTlb[i]);
	}
}