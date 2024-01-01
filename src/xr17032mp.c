#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xr.h"
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

#define SignExt23(n) (((int32_t)(n << 9)) >> 9)
#define SignExt18(n) (((int32_t)(n << 14)) >> 14)
#define SignExt5(n)  (((int32_t)(n << 27)) >> 27)
#define SignExt16(n) (((int32_t)(n << 16)) >> 16)

#define LR 31

#define RS 0
#define WHAMI 1
#define EB 5
#define EPC 6
#define EBADADDR 7
#define TBMISSADDR 9
#define TBPC 10

#define ITBPTE 16
#define ITBTAG 17
#define ITBINDEX 18
#define ITBCTRL 19
#define ICACHECTRL 20
#define ITBADDR 21

#define DTBPTE 24
#define DTBTAG 25
#define DTBINDEX 26
#define DTBCTRL 27
#define DCACHECTRL 28
#define DTBADDR 29

#define NONCACHED 0
#define CACHED 1

#define XR_EXC_INT 1
#define XR_EXC_SYS 2
#define XR_EXC_BUS 4
#define XR_EXC_NMI 5
#define XR_EXC_BRK 6
#define XR_EXC_INV 7
#define XR_EXC_PRV 8
#define XR_EXC_UNA 9
#define XR_EXC_PGF 12
#define XR_EXC_PGW 13
#define XR_EXC_ITB 14
#define XR_EXC_DTB 15

void XrReset(XrProcessor *proc) {
	// Set the program counter to point to the reset vector.

	proc->Pc = 0xFFFE1000;

	// Initialize the control registers that have reset-defined values.

	proc->Cr[RS] = 0;
	proc->Cr[EB] = 0;
	proc->Cr[ICACHECTRL] = (XR_IC_LINE_COUNT_LOG << 16) | (XR_IC_WAY_LOG << 8) | (XR_IC_LINE_SIZE_LOG);
	proc->Cr[DCACHECTRL] = (XR_DC_LINE_COUNT_LOG << 16) | (XR_DC_WAY_LOG << 8) | (XR_DC_LINE_SIZE_LOG);
	proc->Cr[WHAMI] = proc->Id;
}

