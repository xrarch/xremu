#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "ebus.h"
#include "lsic.h"
#include "xr.h"

Lsic LsicTable[XR_PROC_MAX];

uint32_t LsicIplMasks[32] = {
	0x00000001, // 0
	0x00000003, // 1
	0x00000007, // 2
	0x0000000F, // 3
	0x0000001F, // 4
	0x0000003F, // 5
	0x0000007F, // 6
	0x000000FF, // 7
	0x000001FF, // 8
	0x000003FF, // 9
	0x000007FF, // 10
	0x00000FFF, // 11
	0x00001FFF, // 12
	0x00003FFF, // 13
	0x00007FFF, // 14
	0x0000FFFF, // 15
	0x0001FFFF, // 16
	0x0003FFFF, // 17
	0x0007FFFF, // 18
	0x000FFFFF, // 19
	0x001FFFFF, // 20
	0x003FFFFF, // 21
	0x007FFFFF, // 22
	0x00FFFFFF, // 23
	0x01FFFFFF, // 24
	0x03FFFFFF, // 25
	0x07FFFFFF, // 26
	0x0FFFFFFF, // 27
	0x1FFFFFFF, // 28
	0x3FFFFFFF, // 29
	0x7FFFFFFF, // 30
	0xFFFFFFFF  // 31
};

void LsicReset() {
	for (int i = 0; i < XR_PROC_MAX; i++) {
		Lsic *lsic = &LsicTable[i];

		lsic->Registers[LSIC_MASK_0] = 0;
		lsic->Registers[LSIC_MASK_1] = 0;
		lsic->Registers[LSIC_PENDING_0] = 0;
		lsic->Registers[LSIC_PENDING_1] = 0;
		lsic->Registers[LSIC_CLAIM_COMPLETE] = 0;
		lsic->Registers[LSIC_IPL] = 63;

		lsic->LowIplMask = 0xFFFFFFFF;
		lsic->HighIplMask = 0xFFFFFFFF;
		lsic->InterruptPending = 0;
		lsic->Enabled = (CpuTable[i] != 0);
	}
}

void LsicInterrupt(int intsrc) {
	if ((intsrc >= 64) || (intsrc == 0)) {
		fprintf(stderr, "bad interrupt source\n");
		abort();
	}

	int srcbmp = intsrc/32;
	int srcbmpoff = intsrc&31;

	// Broadcast the interrupt to all LSICs.

	for (int i = 0; i < XR_PROC_MAX; i++) {
		Lsic *lsic = &LsicTable[i];

		if (lsic->Enabled == 0) {
			continue;
		}

		XrProcessor *proc = CpuTable[i];

		lsic->Registers[LSIC_PENDING_0 + srcbmp] |= (1 << srcbmpoff);

		uint8_t oldpending = lsic->InterruptPending;

		lsic->InterruptPending =
			((~lsic->Registers[LSIC_MASK_0]) & lsic->Registers[LSIC_PENDING_0] & lsic->LowIplMask) ||
			((~lsic->Registers[LSIC_MASK_1]) & lsic->Registers[LSIC_PENDING_1] & lsic->HighIplMask);

		if (intsrc != 2 || i != 0) { // HACKHACK: Don't poke CPU 0 if this is the RTC interrupt since CPU 0's thread is the one who sends the RTC interrupt.
			if (oldpending != lsic->InterruptPending) {
				// Poke the thread for this cpu to wake it up.

				XrPokeCpu(proc);
			}
		}
	}
}

int LsicWrite(int reg, uint32_t value) {
	int id = reg >> 3;
	reg &= 7;

	Lsic *lsic = &LsicTable[id];

	if (lsic->Enabled == 0) {
		return EBUSERROR;
	}

	XrProcessor *proc = CpuTable[id];

	switch(reg) {
		case LSIC_MASK_0:
		case LSIC_MASK_1:
			lsic->Registers[reg] = value;

			break;

		case LSIC_PENDING_0:
			if (value == 0) {
				// A write of zero into the pending register clears all
				// pending interrupts. Useful for partial reset.

				lsic->Registers[LSIC_PENDING_0] = 0;

				break;
			}

			// Writes to the pending registers atomically OR into the pending
			// interrupt bits. This is useful for IPIs and stuff.

			value &= ~1; // Make sure interrupt zero can't be triggered.
			lsic->Registers[LSIC_PENDING_0] |= value;

			break;

		case LSIC_PENDING_1:
			if (value == 0) {
				// A write of zero into the pending register clears all
				// pending interrupts. Useful for partial reset.

				lsic->Registers[LSIC_PENDING_1] = 0;

				break;
			}

			// Writes to the pending registers atomically OR into the pending
			// interrupt bits. This is useful for IPIs and stuff.

			value &= ~1; // Make sure interrupt zero can't be triggered.
			lsic->Registers[LSIC_PENDING_1] |= value;

			break;

		case LSIC_CLAIM_COMPLETE:
			// Complete. Atomically clear a pending interrupt bit.

			if (value >= 64) {
				return EBUSERROR;
			}

			lsic->Registers[LSIC_PENDING_0 + (value >> 5)] &= ~(1 << (value & 31));

			break;

		case LSIC_IPL:
			// Set some bit masks to mask off interrupts with a number greater
			// than the new IPL, which is a value from 0-63; 0 masks off all, 63
			// enables all.

			if (value >= 64) {
				return EBUSERROR;
			}

			lsic->Registers[LSIC_IPL] = value;

			if (value >= 32) {
				lsic->LowIplMask = 0xFFFFFFFF;
				lsic->HighIplMask = LsicIplMasks[value - 32];
			} else {
				lsic->LowIplMask = LsicIplMasks[value];
				lsic->HighIplMask = 0;
			}

			break;

		default:
			return EBUSERROR;
	}

	uint8_t oldpending = lsic->InterruptPending;

	lsic->InterruptPending =
		((~lsic->Registers[LSIC_MASK_0]) & lsic->Registers[LSIC_PENDING_0] & lsic->LowIplMask) ||
		((~lsic->Registers[LSIC_MASK_1]) & lsic->Registers[LSIC_PENDING_1] & lsic->HighIplMask);

	if (oldpending != lsic->InterruptPending) {
		// Poke the thread for this cpu to wake it up.

		XrPokeCpu(proc);
	}

	return EBUSSUCCESS;
}

int LsicRead(int reg, uint32_t *value) {
	int id = reg >> 3;
	reg &= 7;

	Lsic *lsic = &LsicTable[id];

	if (lsic->Enabled == 0) {
		return EBUSERROR;
	}

	XrProcessor *proc = CpuTable[id];

	switch(reg) {
		case LSIC_MASK_0:
		case LSIC_MASK_1:
		case LSIC_PENDING_0:
		case LSIC_PENDING_1:
		case LSIC_IPL:
			// Reads from most LSIC registers just return their value.

			*value = lsic->Registers[reg];

			return EBUSSUCCESS;

		case LSIC_CLAIM_COMPLETE:
			// Reads from the claim register return the number of the highest
			// priority pending interrupt that is not masked off (which is the
			// one with the lowest number).

			for (int i = 1; i <= lsic->Registers[LSIC_IPL]; i++) {
				int bmp = i/32;
				int bmpoff = i&31;

				if ((((~lsic->Registers[LSIC_MASK_0 + bmp]) & lsic->Registers[LSIC_PENDING_0 + bmp]) >> bmpoff) & 1) {
					*value = i;

					return EBUSSUCCESS;
				}
			}

			*value = 0;

			return EBUSSUCCESS;

		default:
			return EBUSERROR;
	}

	return EBUSERROR;
}