#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "lsic.h"
#include "ebus.h"

#define RS_USER 1
#define RS_INT  2
#define RS_MMU  4

#define RS_ECAUSE_SHIFT 28
#define RS_ECAUSE_MASK  15

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

uint32_t ControlReg[16];

enum Xr17032ControlRegisters {
	RS       = 0,
	TBLO     = 2,
	EPC      = 3,
	EVEC     = 4,
	PGTB     = 5,
	TBINDEX  = 6,
	EBADADDR = 7,
	TBVEC    = 8,
	FWVEC    = 9,
	TBSCRATCH = 10,
	TBHI     = 11,
};

enum Xr17032CacheTypes {
	NOCACHE  = 1,
	CACHE    = 0,
};

enum Xr17032Exceptions {
	EXCINTERRUPT = 1,
	EXCSYSCALL   = 2,
	EXCFWCALL    = 3,
	EXCBUSERROR  = 4,
	
	EXCNMI       = 5,
	EXCBRKPOINT  = 6,
	EXCINVINST   = 7,
	EXCINVPRVG   = 8,
	EXCUNALIGNED = 9,


	EXCPAGEFAULT = 12,
	EXCPAGEWRITE = 13,

	EXCTLBMISS   = 15,
};

bool UserBreak = false;
bool Halted = false;
bool Running = true;

uint32_t PC = 0;

uint32_t TLBPC = 0;

uint32_t CurrentException;

bool IFetch = false;

bool TLBMiss = false;

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

#define TLBSIZELOG 6
#define TLBWAYLOG  2
#define TLBSETLOG  (TLBSIZELOG-TLBWAYLOG)

#define TLBSIZE (1<<TLBSIZELOG)
#define TLBWAYS (1<<TLBWAYLOG)
#define TLBSETS (1<<TLBSETLOG)

uint64_t TLB[TLBSIZE];

uint32_t CPULocked = 0;

uint32_t TLBWriteCount = 0;

uint32_t LastInstruction;

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

static inline bool CPUTranslate(uint32_t virt, uint32_t *phys, int *cachetype, bool writing) {
	uint32_t vpn = virt >> 12;
	uint32_t off = virt & 4095;
	uint32_t tbhi = (ControlReg[TBHI] & 0xFFF00000) | vpn;

	// the set function is designed to split the TLB into a userspace and
	// kernel space half.

	uint32_t set = (vpn & ((1 << (TLBSETLOG - 1)) - 1)) | (vpn >> 19 << (TLBSETLOG - 1));

	bool found;

	uint64_t tlbe;
	uint32_t mask;

	// look up in the TLB, if not found there, do a miss

	uint32_t rememberindex = -1;

	for (int i = 0; i < TLBWAYS; i++) {
		tlbe = TLB[set * TLBWAYS + i];

		mask = (tlbe & 16) ? 0xFFFFF : 0xFFFFFFFF;

		found = (tlbe >> 32) == (tbhi & mask);

		if (found) {
			break;
		}

		if (!(tlbe & 1))
			rememberindex = i;
	}

	if (!found) {
		// didn't find the mapping. a TLB miss exception is in order.

		ControlReg[TBHI] = tbhi;
		ControlReg[PGTB] = (ControlReg[PGTB] & 0xFFFFF000) | ((vpn >> 10) << 2);

		if (rememberindex == -1)
			ControlReg[TBINDEX] = set * TLBWAYS + (TLBWriteCount & (TLBWAYS - 1));
		else
			ControlReg[TBINDEX] = set * TLBWAYS + rememberindex;

		Xr17032Exception(EXCTLBMISS);

		return false;
	}

	if (!(tlbe & 1)) {
		// invalid.

		ControlReg[EBADADDR] = virt;

		Xr17032Exception(writing ? EXCPAGEWRITE : EXCPAGEFAULT);

		return false;
	}

	uint32_t ppn = ((tlbe >> 5) & 0xFFFFF) << 12;

	if ((tlbe & 4) && (ControlReg[RS] & RS_USER)) { // kernel (K) bit
		ControlReg[EBADADDR] = virt;

		Xr17032Exception(writing ? EXCPAGEWRITE : EXCPAGEFAULT);

		return false;
	}

	if (writing && !(tlbe & 2)) { // writable (W) bit not set
		ControlReg[EBADADDR] = virt;

		Xr17032Exception(EXCPAGEWRITE);

		return false;
	}

	*cachetype = ((tlbe >> 3) & 1);
	*phys = ppn + off;

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
	PC = 0xFFFE0000;
	ControlReg[RS] = 0;
	ControlReg[EVEC] = 0;
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

			// enter kernel mode, disable interrupts.

			newstate = ControlReg[RS] & 0xFC;

			if (CurrentException == EXCFWCALL) {
				// firmware calls disable virtual addressing and are vectored
				// through FWVEC.

				evec = ControlReg[FWVEC];
				newstate &= 0xF8;

			} else if (CurrentException == EXCTLBMISS) {
				// TLB misses disable virtual addressing and are vectored
				// through TBVEC.

				evec = ControlReg[TBVEC];
				newstate &= 0xF8;

			} else {
				if (newstate&128) {
					// legacy exceptions are enabled, so disable virtual
					// addressing. this is NOT part of the "official" xr17032
					// architecture and is a hack to continue running AISIX in
					// emulation.

					newstate &= 0xF8;
				}

				// this is a general exception, such as a page fault, hardware
				// interrupt, or syscall, so keep virtual addressing enabled
				// and vector through EVEC.
				
				evec = ControlReg[EVEC];
			}

			if (evec == 0) {
				// the corresponding vector is zero, so reset the CPU.

				CPUReset();

			} else {
				// if CurrentException is null, then we must have come here
				// because of an interrupt.

				if (!CurrentException)
					CurrentException = EXCINTERRUPT;

				switch(CurrentException) {
					case EXCINTERRUPT:
					case EXCSYSCALL:
					case EXCFWCALL:
						ControlReg[EPC] = PC;
						break;

					case EXCTLBMISS:
						CurrentException = ControlReg[RS] >> RS_ECAUSE_SHIFT;
						TLBPC = PC-4;
						TLBMiss = true;
						break;

					default:
						ControlReg[EPC] = PC-4;
						break;
				}

				// set the program counter to the selected vector, and set the
				// new status register bits.

				PC = evec;

				ControlReg[RS] = (CurrentException << RS_ECAUSE_SHIFT) | ((ControlReg[RS] & 0xFFFF)<<8) | newstate;
			}

			// clear CurrentException.

			CurrentException = 0;
		}

		currentpc = PC;

		PC += 4;

		IFetch = true;

		status = CPUReadLong(currentpc, &ir);

		IFetch = false;

		if (status) {
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

				if (rd || ((funct >= 9) && (funct <= 11))) {
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

					case 8: // SC
						if (CPULocked)
							CPUWriteLong(Reg[ra], Reg[rb]);

						if (rd == 0)
							break;

						Reg[rd] = CPULocked;

						break;

					case 9: // LL
						CPULocked = 1;

						if (rd == 0)
							break;

						CPUReadLong(Reg[ra], &Reg[rd]);

						break;

					case 11: // MOD
						if (rd == 0)
							break;

						if (Reg[rb] == 0) {
							Reg[rd] = 0;
							break;
						}

						Reg[rd] = Reg[ra] % Reg[rb];
						break;

					case 12: // DIV SIGNED
						if (rd == 0)
							break;

						if (Reg[rb] == 0) {
							Reg[rd] = 0;
							break;
						}

						Reg[rd] = (int32_t) Reg[ra] / (int32_t) Reg[rb];
						break;

					case 13: // DIV
						if (rd == 0)
							break;

						if (Reg[rb] == 0) {
							Reg[rd] = 0;
							break;
						}

						Reg[rd] = Reg[ra] / Reg[rb];
						break;

					case 15: // MUL
						if (rd == 0)
							break;
						
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
					rb = (ir >> 16) & 15;

					uint32_t asid;
					uint32_t vpn;
					uint32_t index;
					uint64_t tlbe;
					uint32_t pde;
					uint32_t tbhi;

					switch(funct) {
						case 0: // TBWR
							index = ControlReg[TBINDEX] & (TLBSIZE - 1);
							tbhi = ControlReg[TBHI];

							if (ControlReg[TBLO] & 16)
								tbhi &= 0x000FFFFF;

							TLB[index] = ((uint64_t)tbhi << 32) | ControlReg[TBLO];
							TLBWriteCount++;

							break;

						case 1: // TBFN
							ControlReg[TBINDEX] = 0x80000000;

							vpn = ControlReg[TBHI] & 0xFFFFF;

							index = (vpn & ((1 << (TLBSETLOG - 1)) - 1)) | (vpn >> 19 << (TLBSETLOG - 1));

							for (int i = 0; i < TLBWAYS; i++) {
								if ((TLB[index * TLBWAYS + i] >> 32) == ControlReg[TBHI]) {
									ControlReg[TBINDEX] = index * TLBWAYS + i;
									break;
								}
							}

							break;

						case 2: // TBRD
							tlbe = TLB[ControlReg[TBINDEX] & (TLBSIZE - 1)];

							ControlReg[TBLO] = tlbe;
							ControlReg[TBHI] = tlbe >> 32;

							break;

						case 3: // TBLD
							pde = Reg[ra];

							if (!(pde & 1)) {
								Reg[rd] = 0;
								break;
							}

							CPUReadLong(((pde >> 5) << 12) | ((ControlReg[TBHI] & 1023) << 2), &Reg[rd]);

							break;

						case 8: // CACHEI
							if (CPUSimulateCaches) {
								if (rd & 1) {
									// invalidate icache
									for (int i = 0; i < CACHELINES; i++) {
										ICacheTags[i] = 0;
									}
								}

								if (rd & 4) {
									// invalidate dcache
									for (int i = 0; i < CACHELINES; i++) {
										DCacheTags[i] = 0;
									}
								}

								// flush writebuffer
								for (int i = 0; i < WRITEBUFFERDEPTH; i++) {
									if (WriteBufferTags[i]) {
										EBusWrite(WriteBufferTags[i], &WriteBuffer[i * CACHELINESIZE], CACHELINESIZE);
										WriteBufferTags[i] = 0;
									}
								}

								WriteBufferSize = 0;
								WriteBufferCyclesTilNextWrite = 0;
							}

							break;

						case 10: // FWC
							Xr17032Exception(EXCFWCALL);
							break;

						case 11: // RFE
							CPULocked = 0;

							if (TLBMiss) {
								TLBMiss = false;
								PC = TLBPC;
							} else {
								PC = ControlReg[EPC];
							}

							ControlReg[RS] = (ControlReg[RS] & 0xF0000000) | (ControlReg[RS] >> 8) & 0xFFFF;

							break;

						case 12: // HLT
							Halted = true;
							break;

						case 14: // MTCR
							ControlReg[rb] = Reg[ra];
							break;

						case 15: // MFCR
							if (rd == 0)
								break;

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

					case 29: // BGE
						if ((int32_t) Reg[rd] >= 0)
							PC = currentpc + signext23((ir >> 11) << 2);

						break;

					case 21: // BLE
						if ((int32_t) Reg[rd] <= 0)
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
						if (rd == 0)
							break;

						Reg[rd] = Reg[ra] + imm;

						break;

					case 52: // SUBI
						if (rd == 0)
							break;

						Reg[rd] = Reg[ra] - imm;

						break;

					case 44: // SLTI
						if (rd == 0)
							break;

						if (Reg[ra] < imm)
							Reg[rd] = 1;
						else
							Reg[rd] = 0;

						break;

					case 36: // SLTI signed
						if (rd == 0)
							break;

						if ((int32_t) Reg[ra] < (int32_t) signext16(imm))
							Reg[rd] = 1;
						else
							Reg[rd] = 0;

						break;

					case 28: // ANDI
						if (rd == 0)
							break;

						Reg[rd] = Reg[ra] & imm;

						break;

					case 20: // XORI
						if (rd == 0)
							break;

						Reg[rd] = Reg[ra] ^ imm;

						break;

					case 12: // ORI
						if (rd == 0)
							break;

						Reg[rd] = Reg[ra] | imm;

						break;

					case 4: // LUI
						if (rd == 0)
							break;

						Reg[rd] = Reg[ra] | (imm << 16);

						break;

					// LOAD with immediate offset

					case 59: // MOV RD, BYTE
						if (rd == 0)
							break;

						CPUReadByte(Reg[ra] + imm, &Reg[rd]);

						break;

					case 51: // MOV RD, INT
						if (rd == 0)
							break;

						CPUReadInt(Reg[ra] + (imm << 1), &Reg[rd]);

						break;

					case 43: // MOV RD, LONG
						if (rd == 0)
							break;

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
						if (rd != 0)
							Reg[rd] = PC;

						PC = Reg[ra] + signext18(imm << 2);

						break;

					default:
						Xr17032Exception(EXCINVINST);
						break;
				}
			}
		}

		if (Halted || (!Running))
			return cyclesdone;
	}

	return cyclesdone;
}