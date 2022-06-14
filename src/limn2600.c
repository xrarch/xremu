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

#define RS_ERS_SHIFT 8
#define RS_ERS_MASK  255

#define RS_ASID_SHIFT 16
#define RS_ASID_MASK  4095

#define RS_ECAUSE_SHIFT 28
#define RS_ECAUSE_MASK  15

#define RS_EXC_MASK (RS_ERS_MASK | (RS_ERS_MASK<<RS_ERS_SHIFT) | (RS_ECAUSE_MASK<<RS_ECAUSE_SHIFT))

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
	EPC      = 3,
	EVEC     = 4,
	PGTB     = 5,
	EBADADDR = 7,
	FWVEC    = 9,
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
};

bool UserBreak = false;
bool Halted = false;
bool Running = true;

uint32_t PC = 0;

int CurrentException;

bool IFetch = false;

uint64_t TLB[64];

uint32_t ILastASID = -1;
uint32_t DLastASID = -1;

bool ILastGlobal = false;
bool DLastGlobal = false;

uint32_t ILastVPN = -1;
uint32_t ILastPPN = -1;

uint32_t DLastVPN = -1;
uint32_t DLastPPN = -1;

uint32_t CPULocked = 0;

bool DLastVPNWritable = false;

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
	uint32_t myasid = ControlReg[RS]>>RS_ASID_SHIFT & RS_ASID_MASK;

	uint32_t vpn = virt>>12;
	uint32_t off = virt&4095;

	// this is a little fast path just in case its the same translation as last time
	// (which is very likely)

	if (IFetch) {
		if ((vpn == ILastVPN) && ((myasid == ILastASID) || ILastGlobal)) {
			*phys = ILastPPN|off;
			return true;
		}
	} else {
		if ((vpn == DLastVPN) && ((myasid == DLastASID) || DLastGlobal)) {
			if (writing && (!DLastVPNWritable)) {
				ControlReg[EBADADDR] = virt;
				Limn2500Exception(EXCPAGEWRITE);
				return false;
			}

			*phys = DLastPPN|off;
			return true;
		}
	}

	// fast path failed :(
	// look up in the TLB, if not found there, walk the page tables and insert it

	uint32_t base = (((vpn>>15)|(vpn&7))&31)<<1;

	uint64_t tlbe = TLB[base];

	uint32_t tlblo = tlbe&0xFFFFFFFF;
	uint32_t tlbhi = tlbe>>32;

	uint32_t tlbvpn = tlblo&0xFFFFF;
	uint32_t asid = tlblo>>20;

	bool global = (tlbhi&16) == 16;

	if ((tlbvpn != vpn) || ((tlbhi&1) == 0) || ((asid != myasid) && !global)) {
		// not a match, try the other member of the set

		tlbe = TLB[base+1];

		tlblo = tlbe&0xFFFFFFFF;
		tlbhi = tlbe>>32;

		tlbvpn = tlblo&0xFFFFF;
		asid = tlblo>>20;

		global = (tlbhi&16) == 16;

		if ((tlbvpn != vpn) || ((tlbhi&1) == 0) || ((asid != myasid) && !global)) {
			// not a match, walk page table

			uint32_t pde;

			if (EBusRead(ControlReg[PGTB]+((virt>>22)<<2), EBUSLONG, &pde) == EBUSERROR) {
				ControlReg[EBADADDR] = ControlReg[PGTB]+((virt>>22)<<2);
				Limn2500Exception(EXCBUSERROR);
				return false;
			}

			if ((pde&1) == 0) {
				ControlReg[EBADADDR] = virt;
				Limn2500Exception(writing ? EXCPAGEWRITE : EXCPAGEFAULT);
				return false;
			}

			if (EBusRead(((pde>>5)<<12)+((vpn&1023)<<2), EBUSLONG, &tlbhi) == EBUSERROR) {
				ControlReg[EBADADDR] = ((pde>>5)<<12)+((vpn&1023)<<2);
				Limn2500Exception(EXCBUSERROR);
				return false;
			}

			if ((tlbhi&1) == 0) { // valid (V) bit not set
				ControlReg[EBADADDR] = virt;
				Limn2500Exception(writing ? EXCPAGEWRITE : EXCPAGEFAULT);
				return false;
			}

			global = (tlbhi&16) == 16;

			base = (((vpn>>15)|(vpn&7))&31)<<1;

			if (((TLB[base]>>32)&1) == 1) {
				if (((TLB[base+1]>>32)&1) == 1) {
					if (TLBWriteCount&1) {
						base++;
					}
				} else {
					base++;
				}
			}

			TLBWriteCount++;

			TLB[base] = (((uint64_t)tlbhi)<<32) | (((global ? 0 : myasid)<<20) | vpn);
		}
	}

	uint32_t ppn = ((tlbhi>>5)&0xFFFFF)<<12;

	if (IFetch) {
		ILastPPN = ppn;
		ILastVPN = vpn;
		ILastASID = myasid;
		ILastGlobal = global;
	} else {
		DLastPPN = ppn;
		DLastVPN = vpn;
		DLastASID = myasid;
		DLastVPNWritable = (tlbhi&2)==2; // writable (W) bit
		DLastGlobal = global;
	}

	if (((tlbhi&4) == 4) && (ControlReg[RS]&RS_USER)) { // kernel (K) bit
		ControlReg[EBADADDR] = virt;
		Limn2500Exception(writing ? EXCPAGEWRITE : EXCPAGEFAULT);
		return false;
	}

	if (writing && ((tlbhi&2)==0)) { // writable (W) bit not set
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
			newstate = ControlReg[RS] & 0xFFFFFFFC; // enter kernel mode, disable interrupts

			if (CurrentException == EXCFWCALL) {
				evec = ControlReg[FWVEC];

				newstate &= 0xFFFFFFF8; // disable virtual addressing
			} else {
				if (newstate&128) {
					// legacy exceptions, disable virtual addressing
					newstate &= 0xFFFFFFF8;
				}
				
				evec = ControlReg[EVEC];
			}

			if (evec == 0) {
				// fprintf(stderr, "exception but no exception vector, resetting");
				CPUReset();
			} else {
				if (!CurrentException) // must be an interrupt
					CurrentException = EXCINTERRUPT;

				ControlReg[EPC] = PC;
				PC = evec;

				uint32_t ers = ControlReg[RS];

				ControlReg[RS] &= (~RS_EXC_MASK);
				ControlReg[RS] |= CurrentException<<RS_ECAUSE_SHIFT;
				ControlReg[RS] |= (ers & RS_ERS_MASK)<<RS_ERS_SHIFT;
				ControlReg[RS] |= newstate & RS_ERS_MASK;
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

					switch(funct) {
						case 10: // fwc
							Limn2500Exception(EXCFWCALL);
							break;

						case 11: // rfe
							CPULocked = 0;
							PC = ControlReg[EPC];
							ControlReg[RS] &= (~RS_ERS_MASK);
							ControlReg[RS] |= (ControlReg[RS]>>RS_ERS_SHIFT)&RS_ERS_MASK;
							break;

						case 12: // hlt
							Halted = true;
							break;

						case 13: // ftlb
							asid = Reg[ra];
							vpn = Reg[(ir>>16)&31];

							if (vpn&0x80000000) {
								if (asid&0x80000000) {
									memset(&TLB, 0, sizeof(TLB));
								} else {
									for (int i = 0; i < 64; i++) {
										if (((TLB[i]>>20)&4095) == asid) {
											TLB[i] = 0;
										}
									}
								}
							} else {
								uint32_t base = (((vpn>>15)|(vpn&7))&31)<<1;

								uint32_t tlbe = TLB[base];

								uint32_t tlblo = tlbe&0xFFFFFFFF;

								uint32_t tlbvpn = tlblo&0xFFFFF;
								uint32_t tlbasid = tlblo>>20;

								if ((tlbvpn != vpn) || (tlbasid != asid)) {
									// not a match, check other member of set

									base++;

									tlbe = TLB[base];

									tlblo = tlbe&0xFFFFFFFF;

									tlbvpn = tlblo&0xFFFFF;
									tlbasid = tlblo>>20;

									if ((tlbvpn == vpn) && (tlbasid == asid)) {
										// match, flush

										TLB[base] = 0;
									}
								} else {
									// match, flush

									TLB[base] = 0;
								}
							}

							ILastASID = -1;
							ILastVPN = -1;
							ILastGlobal = false;

							DLastASID = -1;
							DLastVPN = -1;
							DLastGlobal = false;
							DLastVPNWritable = false;

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