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

#define signext23(n) (((int32_t)(n << 9)) >> 9)
#define signext18(n) (((int32_t)(n << 14)) >> 14)
#define signext5(n)  (((int32_t)(n << 27)) >> 27)
#define signext16(n) (((int32_t)(n << 16)) >> 16)

int CPUProgress;

uint32_t Reg[32];

enum Limn2500Registers {
	LR = 31,
};

uint32_t ControlReg[16];

enum Limn2500ControlRegisters {
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

enum Limn2500Exceptions {
	EXCINTERRUPT = 1,
	EXCSYSCALL   = 2,
	EXCFWCALL    = 3,
	EXCBUSERROR  = 4,
	
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

int CurrentException;

bool IFetch = false;

bool TLBMiss = false;

#define TLBSIZE 64

uint64_t TLB[TLBSIZE];

uint32_t DLastIndex = 0;
uint32_t ILastIndex = 0;

uint32_t CPULocked = 0;

uint32_t TLBWriteCount = 0;

uint32_t LastInstruction;

void Limn2500Exception(int exception) {
	if (CurrentException) {
		fprintf(stderr, "double exception, shouldnt ever happen");
		abort();
	}

	CurrentException = exception;
}

uint32_t RoR(uint32_t x, uint32_t n) {
    return (x >> n & 31) | (x << (32-n) & 31);
}

bool CPUTranslate(uint32_t virt, uint32_t *phys, bool writing) {
	uint32_t vpn = virt>>12;
	uint32_t off = virt&4095;

	uint32_t myasid = ControlReg[TBHI]>>20 & 4095;
	uint32_t tbhi = (myasid<<20) | vpn;

	int start;

	if (IFetch) {
		start = ILastIndex;
	} else {
		start = DLastIndex;
	}

	int j = start ? 0 : 1;

	// look up in the TLB, if not found there, do a miss

	bool found = false;

	uint64_t tlbe;
	uint32_t mask;

	int i;

	while (j < 2) {
		int min;

		if (j == 0) {
			min = 0;
			i = start;
		} else {
			min = start ? start+1 : 0;
			i = TLBSIZE-1;
		}

		while (i >= min) {
			tlbe = TLB[i];

			mask = (tlbe&16) ? 0xFFFFF : 0xFFFFFFFF;

			found = (tlbe>>32) == (tbhi&mask);

			if (found)
				break;

			i--;
		}

		if (found)
			break;

		j++;
	}

	if (!found) {
		// didn't find it. TLB miss
		ControlReg[TBHI] = tbhi;
		ControlReg[PGTB] = (ControlReg[PGTB]&0xFFFFF000)|((vpn>>10)<<2);
		ControlReg[TBINDEX] = TLBWriteCount;

		Limn2500Exception(EXCTLBMISS);
		return false;
	}

	if (!(tlbe&1)) {
		// invalid.
		ControlReg[EBADADDR] = virt;
		Limn2500Exception(writing ? EXCPAGEWRITE : EXCPAGEFAULT);
		return false;
	}

	uint32_t ppn = ((tlbe>>5)&0xFFFFF)<<12;

	if (IFetch) {
		ILastIndex = i;
	} else {
		DLastIndex = i;
	}

	if (((tlbe&4) == 4) && (ControlReg[RS]&RS_USER)) { // kernel (K) bit
		ControlReg[EBADADDR] = virt;
		Limn2500Exception(writing ? EXCPAGEWRITE : EXCPAGEFAULT);
		return false;
	}

	if (writing && ((tlbe&2)==0)) { // writable (W) bit not set
		ControlReg[EBADADDR] = virt;
		Limn2500Exception(EXCPAGEWRITE);
		return false;
	}

	*phys = ppn+off;

	return true;
}

bool CPUReadByte(uint32_t address, uint32_t *value) {
	if ((address < 0x1000) || (address >= 0xFFFFF000)) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCPAGEFAULT);
		return false;
	}

	if (ControlReg[RS]&RS_MMU) {
		if (!CPUTranslate(address, &address, false))
			return false;
	}

	if (EBusRead(address, EBUSBYTE, value) == EBUSERROR) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCBUSERROR);
		return false;
	}

	return true;
}

bool CPUReadInt(uint32_t address, uint32_t *value) {
	if ((address < 0x1000) || (address >= 0xFFFFF000)) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCPAGEFAULT);
		return false;
	}

	if (address & 0x1) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCUNALIGNED);
		return false;
	}

	if (ControlReg[RS]&RS_MMU) {
		if (!CPUTranslate(address, &address, false))
			return false;
	}

	if (EBusRead(address, EBUSINT, value) == EBUSERROR) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCBUSERROR);
		return false;
	}

	return true;
}

bool CPUReadLong(uint32_t address, uint32_t *value) {
	if ((address < 0x1000) || (address >= 0xFFFFF000)) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCPAGEFAULT);
		return false;
	}

	if (address & 0x3) {
		Limn2500Exception(EXCUNALIGNED);
		ControlReg[EBADADDR] = address;
		return false;
	}

	if (ControlReg[RS]&RS_MMU) {
		if (!CPUTranslate(address, &address, false))
			return false;
	}

	if (EBusRead(address, EBUSLONG, value) == EBUSERROR) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCBUSERROR);
		return false;
	}

	return true;
}

bool CPUWriteByte(uint32_t address, uint32_t value) {
	if ((address < 0x1000) || (address >= 0xFFFFF000)) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCPAGEWRITE);
		return false;
	}

	if (ControlReg[RS]&RS_MMU) {
		if (!CPUTranslate(address, &address, true))
			return false;
	}

	if (EBusWrite(address, EBUSBYTE, value) == EBUSERROR) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCBUSERROR);
		return false;
	}

	return true;
}

bool CPUWriteInt(uint32_t address, uint32_t value) {
	if ((address < 0x1000) || (address >= 0xFFFFF000)) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCPAGEWRITE);
		return false;
	}

	if (address & 0x1) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCUNALIGNED);
		return false;
	}

	if (ControlReg[RS]&RS_MMU) {
		if (!CPUTranslate(address, &address, true))
			return false;
	}

	if (EBusWrite(address, EBUSINT, value) == EBUSERROR) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCBUSERROR);
		return false;
	}

	return true;
}

bool CPUWriteLong(uint32_t address, uint32_t value) {
	if ((address < 0x1000) || (address >= 0xFFFFF000)) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCPAGEWRITE);
		return false;
	}

	if (address & 0x3) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCUNALIGNED);
		return false;
	}

	if (ControlReg[RS]&RS_MMU) {
		if (!CPUTranslate(address, &address, true))
			return false;
	}

	if (EBusWrite(address, EBUSLONG, value) == EBUSERROR) {
		ControlReg[EBADADDR] = address;
		Limn2500Exception(EXCBUSERROR);
		return false;
	}

	return true;
}

void CPUReset() {
	PC = 0xFFFE0000;
	ControlReg[RS] = 0;
	ControlReg[EVEC] = 0;
	CurrentException = 0;
}

uint32_t CPUDoCycles(uint32_t cycles) {
	if (!Running)
		return cycles;

	if (UserBreak && !CurrentException) {
		Limn2500Exception(6); // breakpoint
		UserBreak = false;
	}

	if (Halted) {
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
			// skip the rest of the tick so as not to eat up too much of the host's CPU.
			return cycles;
		}

		if (CurrentException || ((ControlReg[RS] & RS_INT) && LSICInterruptPending)) {
			newstate = ControlReg[RS] & 0xFC; // enter kernel mode, disable interrupts

			if (CurrentException == EXCFWCALL) {
				evec = ControlReg[FWVEC];
				newstate &= 0xF8; // disable virtual addressing
			} else if (CurrentException == EXCTLBMISS) {
				evec = ControlReg[TBVEC];
				newstate &= 0xF8; // disable virtual addressing
			} else {
				if (newstate&128) {
					// legacy exceptions, disable virtual addressing
					newstate &= 0xF8;
				}
				
				evec = ControlReg[EVEC];
			}

			if (evec == 0) {
				// fprintf(stderr, "exception but no exception vector, resetting");
				CPUReset();
			} else {
				if (!CurrentException) // must be an interrupt
					CurrentException = EXCINTERRUPT;

				switch(CurrentException) {
					case EXCINTERRUPT:
					case EXCSYSCALL:
					case EXCFWCALL:
						ControlReg[EPC] = PC;
						break;
					case EXCTLBMISS:
						CurrentException = ControlReg[RS]>>RS_ECAUSE_SHIFT;
						TLBPC = PC-4;
						TLBMiss = true;
						break;
					default:
						ControlReg[EPC] = PC-4;
						break;
				}

				PC = evec;

				ControlReg[RS] = (CurrentException<<RS_ECAUSE_SHIFT) | ((ControlReg[RS]&0xFFFF)<<8) | newstate;
			}

			CurrentException = 0;
		}

		currentpc = PC;

		PC += 4;

		IFetch = true;

		status = CPUReadLong(currentpc, &ir);

		IFetch = false;

		if (status) {
			// decode

			maj = ir & 7;
			majoropcode = ir & 63;

			if (maj == 7) { // JAL
				Reg[LR] = PC;
				PC = (currentpc&0x80000000)|((ir>>3)<<2);
			} else if (maj == 6) { // J
				PC = (currentpc&0x80000000)|((ir>>3)<<2);
			} else if (majoropcode == 57) { // reg instructions 111001
				funct = ir>>28;

				shifttype = (ir>>26)&3;
				shift = (ir>>21)&31;

				rd = (ir>>6)&31;
				ra = (ir>>11)&31;
				rb = (ir>>16)&31;

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
							Reg[rd] = ~(Reg[ra]|val);
							break;

						case 1: // OR
							Reg[rd] = Reg[ra]|val;
							break;

						case 2: // XOR
							Reg[rd] = Reg[ra]^val;
							break;

						case 3: // AND
							Reg[rd] = Reg[ra]&val;
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
							Reg[rd] = Reg[ra]-val;
							break;

						case 7: // ADD
							Reg[rd] = Reg[ra]+val;
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

						case 9: // mov long, rd
							CPUWriteLong(Reg[ra]+val, Reg[rd]);
							break;

						case 10: // mov int, rd
							CPUWriteInt(Reg[ra]+val, Reg[rd]);
							break;

						case 11: // mov byte, rd
							CPUWriteByte(Reg[ra]+val, Reg[rd]);
							break;

						case 12: // invalid
							Limn2500Exception(EXCINVINST);
							break;

						case 13: // mov rd, long
							CPUReadLong(Reg[ra]+val, &Reg[rd]);
							break;

						case 14: // mov rd, int
							CPUReadInt(Reg[ra]+val, &Reg[rd]);
							break;

						case 15: // mov rd, byte
							CPUReadByte(Reg[ra]+val, &Reg[rd]);
							break;

						default: // unreachable
							abort();
					}
				}
			} else if (majoropcode == 49) { // reg instructions 110001
				funct = ir>>28;

				rd = (ir>>6)&31;
				ra = (ir>>11)&31;
				rb = (ir>>16)&31;

				switch(funct) {
					case 0: // sys
						Limn2500Exception(EXCSYSCALL);
						break;

					case 1: // brk
						Limn2500Exception(EXCBRKPOINT);
						break;

					case 8: // sc
						if (CPULocked)
							CPUWriteLong(Reg[ra], Reg[rb]);

						if (rd == 0)
							break;

						Reg[rd] = CPULocked;

						break;

					case 9: // ll
						CPULocked = 1;

						if (rd == 0)
							break;

						CPUReadLong(Reg[ra], &Reg[rd]);

						break;

					case 11: // mod
						if (rd == 0)
							break;

						if (Reg[rb] == 0) {
							Reg[rd] = 0;
							break;
						}

						Reg[rd] = Reg[ra] % Reg[rb];
						break;

					case 12: // div signed
						if (rd == 0)
							break;

						if (Reg[rb] == 0) {
							Reg[rd] = 0;
							break;
						}

						Reg[rd] = (int32_t) Reg[ra] / (int32_t) Reg[rb];
						break;

					case 13: // div
						if (rd == 0)
							break;

						if (Reg[rb] == 0) {
							Reg[rd] = 0;
							break;
						}

						Reg[rd] = Reg[ra] / Reg[rb];
						break;

					case 15: // mul
						if (rd == 0)
							break;
						
						Reg[rd] = Reg[ra] * Reg[rb];
						break;

					default:
						Limn2500Exception(EXCINVINST);
						break;
				}
			} else if (majoropcode == 41) { // privileged instructions 101001
				if (ControlReg[RS]&RS_USER) {
					Limn2500Exception(EXCINVPRVG);
				} else {
					funct = ir>>28;

					rd = (ir>>6)&31;
					ra = (ir>>11)&31;
					rb = (ir>>16)&15;

					uint32_t asid;
					uint32_t vpn;
					uint32_t index;
					uint64_t tlbe;
					uint32_t pde;
					uint32_t tbhi;

					switch(funct) {
						case 0: // tbwr
							index = ControlReg[TBINDEX]&(TLBSIZE-1);
							tbhi = ControlReg[TBHI];

							if (ControlReg[TBLO]&16)
								tbhi &= 0x000FFFFF;

							TLB[index] = ((uint64_t)tbhi<<32)|ControlReg[TBLO];

							TLBWriteCount++;

							break;

						case 1: // tbfn
							ControlReg[TBINDEX] = 0x80000000;

							for (int i = 0; i < TLBSIZE; i++) {
								if ((TLB[i]>>32) == ControlReg[TBHI]) {
									ControlReg[TBINDEX] = i;
									break;
								}
							}

							break;

						case 2: // tbrd
							tlbe = TLB[ControlReg[TBINDEX]&(TLBSIZE-1)];

							ControlReg[TBLO] = tlbe;
							ControlReg[TBHI] = tlbe>>32;

							break;

						case 3: // tbld
							pde = ControlReg[TBLO];

							if (!(pde&1)) {
								ControlReg[TBLO] = 0;
								break;
							}

							CPUReadLong(((pde>>5)<<12)|((ControlReg[TBHI]&1023)<<2), &ControlReg[TBLO]);

							break;

						case 10: // fwc
							Limn2500Exception(EXCFWCALL);
							break;

						case 11: // rfe
							CPULocked = 0;

							if (TLBMiss) {
								TLBMiss = false;
								PC = TLBPC;
							} else {
								PC = ControlReg[EPC];
							}

							ControlReg[RS] = (ControlReg[RS]&0xF0000000)|(ControlReg[RS]>>8)&0xFFFF;

							break;

						case 12: // hlt
							Halted = true;
							break;

						case 14: // mtcr
							ControlReg[rb] = Reg[ra];
							break;

						case 15: // mfcr
							if (rd == 0)
								break;

							Reg[rd] = ControlReg[rb];
							break;

						default:
							Limn2500Exception(EXCINVINST);
							break;
					}
				}
			} else { // major opcodes
				rd = (ir>>6)&31;
				ra = (ir>>11)&31;
				imm = ir>>16;

				switch(majoropcode) {
					// branches
					
					case 61: // BEQ
						if (Reg[rd] == 0)
							PC = currentpc + signext23((ir>>11)<<2);
						break;

					case 53: // BNE
						if (Reg[rd] != 0)
							PC = currentpc + signext23((ir>>11)<<2);
						break;

					case 45: // BLT
						if ((int32_t) Reg[rd] < 0)
							PC = currentpc + signext23((ir>>11)<<2);
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
						Reg[rd] = Reg[ra] | (imm<<16);
						break;

					// LOAD with immediate offset

					case 59: // mov rd, byte
						if (rd == 0)
							break;
						CPUReadByte(Reg[ra] + imm, &Reg[rd]);
						break;

					case 51: // mov rd, int
						if (rd == 0)
							break;
						CPUReadInt(Reg[ra] + (imm<<1), &Reg[rd]);
						break;

					case 43: // mov rd, long
						if (rd == 0)
							break;
						CPUReadLong(Reg[ra] + (imm<<2), &Reg[rd]);
						break;

					// STORE with immediate offset

					case 58: // mov byte rd+imm, ra
						CPUWriteByte(Reg[rd] + imm, Reg[ra]);
						break;

					case 50: // mov int rd+imm, ra
						CPUWriteInt(Reg[rd] + (imm<<1), Reg[ra]);
						break;

					case 42: // mov long rd+imm, ra
						CPUWriteLong(Reg[rd] + (imm<<2), Reg[ra]);
						break;

					case 26: // mov byte rd+imm, imm5
						CPUWriteByte(Reg[rd] + imm, signext5(ra));
						break;

					case 18: // mov int rd+imm, imm5
						CPUWriteInt(Reg[rd] + (imm<<1), signext5(ra));
						break;

					case 10: // mov long rd+imm, imm5
						CPUWriteLong(Reg[rd] + (imm<<2), signext5(ra));
						break;

					// jalr

					case 56: // jalr
						if (rd != 0)
							Reg[rd] = PC;
						PC = Reg[ra] + signext18(imm<<2);
						break;

					default:
						Limn2500Exception(EXCINVINST);
						break;
				}
			}
		}

		if (Halted || (!Running))
			return cyclesdone;
	}

	return cyclesdone;
}