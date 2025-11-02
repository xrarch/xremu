#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include "xr.h"

#include "ebus.h"
#include "pboard.h"
#include "lsic.h"
#include "serial.h"

#include "screen.h"
#include "tty.h"

typedef void (*DbgCommandF)();

struct DbgCommand {
	DbgCommandF command;
	char *help;
	char *name;
};

struct TTY *DbgTty;

#define CMD_MAX 256

char *DbgRegNames[32] = {
	"zero", "t0", "t1", "t2", "t3", "t4", "t5", "a0", "a1", "a2", "a3", "s0",
	"s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12",
	"s13", "s14", "s15", "s16", "s17", "tp", "sp", "lr"
};

char *DbgCrNames[32] = {
	"rs", "whami", 0, 0, 0, "eb", "epc", "ebadaddr", 0, "tbmissaddr", "tbpc", "scratch0",
	"scratch1", "scratch2", "scratch3", "scratch4", "itbpte", "itbtag", "itbindex", "itbctrl",
	"icachectrl", "itbaddr", 0, 0, "dtbpte", "dtbtag", "dtbindex", "dtbctrl", "dcachectrl",
	"dtbaddr"
};

char *DbgTbFlags[5] = {
	"V", "W", "K", "N", "G"
};

char cmdline[CMD_MAX];
char tokenbuf[CMD_MAX];
char printbuf[CMD_MAX];
int cmdindex = 0;

int DbgSelectedCpu = 0;

char *cmdnexttoken;

char *RtlTokenize(char *buffer, char *tokenbuffer, int bufsize, char delimiter) {
	while (*buffer == delimiter) {
		buffer++;
	}

	if (buffer[0] == 0) {
		return 0;
	}

	while (bufsize - 1) {
		if (buffer[0] == delimiter || buffer[0] == 0) {
			break;
		}

		tokenbuffer[0] = buffer[0];

		tokenbuffer++;
		buffer++;
		bufsize--;
	}

	tokenbuffer[0] = 0;

	return buffer;
}

int DbgNextToken(char *tokenbuffer, int bufsize) {
	if (!cmdnexttoken) {
		return 0;
	}

	cmdnexttoken = RtlTokenize(cmdnexttoken, tokenbuffer, bufsize, ' ');

	if (cmdnexttoken) {
		return 1;
	}

	return 0;
}

uint32_t DbgParseAddress(char *addr) {
	if (addr[0] == '0') {
		if (addr[1] == 'x') {
			return strtol(addr, 0, 16);
		} else {
			return strtol(addr, 0, 8);
		}
	}

	return strtol(addr, 0, 10);
}

void DbgPutString(char *str) {
	for (; *str; str++) {
		if (*str == '\n') {
			TTYPutCharacter(DbgTty, '\r');
		}

		TTYPutCharacter(DbgTty, *str);
	}
}

void DbgPrompt() {
	DbgPutString("emu>> ");
}

void DbgHeader() {
	DbgPutString("Emulator Level Debugger\n");
}

#define DBG_TRANSLATE_PHYSICAL 1
#define DBG_TRANSLATE_BYPASS_TB 2
#define DBG_TRANSLATE_BYPASS_PT 4
#define DBG_TRANSLATE_PRINT 8
#define DBG_TRANSLATE_ISTREAM 16
#define DBG_TRANSLATE_FIGURE_IT_OUT 32
#define DBG_TRANSLATE_BYPASS_TB_FOR_PGTB 64

int DbgTranslateAddressTb(uint64_t *tb, uint32_t addr, uint32_t asid, uint64_t *tbe, int tbsize, int *index) {
	// Translate a guest virtual address to a guest physical address by looking
	// up the given TB. If there's no matching entry, return 0.
	// The processor's run lock should be held.

	uint32_t vpn = addr >> 12;
	uint32_t matching = (asid << 20) | vpn;

	for (int i = 0; i < tbsize; i++) {
		uint64_t tmp = tb[i];
		uint32_t mask = (tmp & 16) ? 0xFFFFF : 0xFFFFFFFF;

		if (((tmp >> 32) & mask) == (matching & mask)) {
			*tbe = tmp;
			*index = i;

			return 1;
		}
	}

	return 0;
}

int DbgTranslateVirtual(XrProcessor *proc, uint32_t addr, uint32_t *phys, int flags) {
	// Translate a virtual address from the perspective of the given processor.

	uint64_t tbe;
	char *tbname = "DTB";
	uint64_t *tb = &proc->Dtb[0];
	int tbsize = XR_DTB_SIZE;
	int ptbr = 29; // DTBADDR
	uint32_t asid = proc->Cr[25] >> 20; // DTBTAG

	if (flags & DBG_TRANSLATE_ISTREAM) {
		tbname = "ITB";
		tb = &proc->Itb[0];
		tbsize = XR_ITB_SIZE;
		ptbr = 21; // ITBADDR
		asid = proc->Cr[17] >> 20; // ITBTAG
	}

	int index;

	if ((flags & DBG_TRANSLATE_BYPASS_TB) == 0) {
		if (DbgTranslateAddressTb(tb, addr, asid, &tbe, tbsize, &index)) {
			if (flags & DBG_TRANSLATE_PRINT) {
				sprintf(&printbuf[0], "%s[%d]=%016llX ", tbname, index, tbe);
				DbgPutString(&printbuf[0]);
			}

			if (tbe & 1) {
				*phys = ((tbe >> 5) << 12) | (addr & 0xFFF);

				return 1;
			} else {
				return 0;
			}
		}
	}

	uint32_t pgtb = proc->Cr[ptbr] & 0xFFC00000;

	if ((flags & DBG_TRANSLATE_BYPASS_PT) == 0) {
		// Look up the page tables. Assume the preferred page table format.
		// We assume that if the virtual page table base was nonzero, then the
		// preferred paging scheme is probably active. Sort of hacky but very
		// useful for debugging MINTIA and Linux which both use it.

		// We want to imitate the way the real CPU would do it, in order to
		// allow the debugger's user to catch strange problems, so first we
		// look up the containing page table for the PTE in the DTB.

		if (pgtb == 0) {
			if (flags & DBG_TRANSLATE_PRINT) {
				DbgPutString("NoPageTable ");
			}

			return 0;
		}

		uint32_t ptevaddr = pgtb + ((addr >> 12) << 2);
		uint64_t pttbe;
		uint32_t physpteaddr;

		if (((flags & DBG_TRANSLATE_BYPASS_TB_FOR_PGTB) == 0) &&
			DbgTranslateAddressTb(&proc->Dtb[0], ptevaddr, asid, &pttbe, XR_DTB_SIZE, &index)) {
			if (flags & DBG_TRANSLATE_PRINT) {
				sprintf(&printbuf[0], "Pde@DTB[%d]=%016llX ", index, pttbe);
				DbgPutString(&printbuf[0]);
			}

			if ((pttbe & 1) == 0) {
				// The PDE cached in the DTB is invalid.

				return 0;
			}

			physpteaddr = ((pttbe >> 5) << 12) | (ptevaddr & 0xFFF);
		} else {
			if ((addr & 0xFFC00000) == pgtb) {
				// The original virtual address we were meant to translate was
				// inside of the page tables, which means we just failed to look
				// up the page directory inside the TB. That's not supposed to
				// happen if the scheme is implemented correctly, so this must
				// be a bug in the guest kernel.

				if (flags & DBG_TRANSLATE_PRINT) {
					DbgPutString("NoPageDirTbe ");
				}

				return 0;
			}

			// We need to find the page directory inside the DTB.

			uint32_t pdevaddr = pgtb + ((ptevaddr >> 12) << 2);
			uint64_t pdtbe;
			uint32_t physpdeaddr;

			if (DbgTranslateAddressTb(&proc->Dtb[0], pdevaddr, asid, &pdtbe, XR_DTB_SIZE, &index)) {
				if (flags & DBG_TRANSLATE_PRINT) {
					sprintf(&printbuf[0], "PdTbe@DTB[%d]=%016llX ", index, pdtbe);
					DbgPutString(&printbuf[0]);
				}

				if ((pdtbe & 1) == 0) {
					// The entry mapping the page directory is invalid.

					return 0;
				}

				physpdeaddr = ((pdtbe >> 5) << 12) | (pdevaddr & 0xFFF);
			}

			// Load the PDE.

			uint32_t pde;

			int status = EBusRead(physpdeaddr, &pde, 4, proc);

			if (status != EBUSSUCCESS) {
				if (flags & DBG_TRANSLATE_PRINT) {
					sprintf(&printbuf[0], "PdeBusError@Phys[%08x] ", physpdeaddr);
					DbgPutString(&printbuf[0]);
				}

				return 0;
			}

			if (flags & DBG_TRANSLATE_PRINT) {
				sprintf(&printbuf[0], "Pde@Phys[%08X]=%08X ", physpdeaddr, pde);
				DbgPutString(&printbuf[0]);
			}

			if ((pde & 1) == 0) {
				return 0;
			}

			// Calculate the physical PTE address.

			physpteaddr = ((pde >> 5) << 12) | (ptevaddr & 0xFFF);
		}

		// Load the PTE.

		uint32_t pte;

		int status = EBusRead(physpteaddr, &pte, 4, proc);

		if (status != EBUSSUCCESS) {
			if (flags & DBG_TRANSLATE_PRINT) {
				sprintf(&printbuf[0], "PteBusError@Phys[%08x] ", physpteaddr);
				DbgPutString(&printbuf[0]);
			}

			return 0;
		}

		if (flags & DBG_TRANSLATE_PRINT) {
			sprintf(&printbuf[0], "Pte@Phys[%08X]=%08X ", physpteaddr, pte);
			DbgPutString(&printbuf[0]);
		}

		if ((pte & 1) == 0) {
			return 0;
		}

		*phys = ((pte >> 5) << 12) | (addr & 0xFFF);
		return 1;
	}

	return 0;
}

int DbgTranslateAddress(XrProcessor *proc, uint32_t addr, uint32_t *phys, int flags) {
	// Translate a guest address to a guest physical address.
	// Returns 1 on success, 0 on failure.

	XrLockMutex(&proc->RunLock);

	if (flags & DBG_TRANSLATE_FIGURE_IT_OUT) {
		// Set flags based on current perspective of the processor.

		flags = flags & DBG_TRANSLATE_PRINT;

		if ((proc->Cr[0] & 4) == 0) {
			flags |= DBG_TRANSLATE_PHYSICAL;
		}
	}

	int result = 1;

	if ((flags & DBG_TRANSLATE_PHYSICAL) == 0) {
		// Do virtual translation of the address.

		result = DbgTranslateVirtual(proc, addr, phys, flags);
	} else {
		*phys = addr;
	}

	if (flags & DBG_TRANSLATE_PRINT) {
		if (result) {
			sprintf(&printbuf[0], "PHYS=%08x\n", *phys);
			DbgPutString(&printbuf[0]);
		} else {
			DbgPutString("PHYS=Invalid\n");
		}
	}

	XrUnlockMutex(&proc->RunLock);

	return result;
}

int DbgDecodeTranslateFlags(char *flags, uint32_t oldflags) {
	// Skip past - character
	flags++;

	int flag = oldflags;

	for (; *flags; flags++) {
		if (*flags == 'p') {
			flag |= DBG_TRANSLATE_PHYSICAL;
			flag &= ~DBG_TRANSLATE_FIGURE_IT_OUT;
		} else if (*flags == 'v') {
			flag &= ~DBG_TRANSLATE_PHYSICAL;
			flag &= ~DBG_TRANSLATE_FIGURE_IT_OUT;
		} else if (*flags == 'i') {
			flag |= DBG_TRANSLATE_ISTREAM;
		} else if (*flags == 't') {
			flag |= DBG_TRANSLATE_BYPASS_TB;
		} else if (*flags == 'g') {
			flag |= DBG_TRANSLATE_BYPASS_TB_FOR_PGTB;
		} else if (*flags == 'z') {
			flag |= DBG_TRANSLATE_PRINT;
		}
	}

	return flag;
}

struct DbgCommand DbgCommands[];

void DbgCommandHelp() {
	for (int i = 0; DbgCommands[i].name; i++) {
		sprintf(&printbuf[0], " %-10s%s\n", DbgCommands[i].name, DbgCommands[i].help);
		DbgPutString(&printbuf[0]);
	}
}

void DbgCommandClear() {
	DbgPutString("\x1B[0m\x1B[1;1H\x1B[2J");

	DbgHeader();
}

void DbgCommandReg() {
	XrProcessor *proc = XrProcessorTable[DbgSelectedCpu];

	for (int i = 1; i < 32; i++) {
		if (i > 1 && (i - 1) % 8 == 0) {
			DbgPutString("\n");
		}

		sprintf(&printbuf[0], "%3s=%08x ", DbgRegNames[i], proc->Reg[i]);
		DbgPutString(&printbuf[0]);
	}

	sprintf(&printbuf[0], " pc=%08x\n", proc->Pc);
	DbgPutString(&printbuf[0]);
}

void DbgCommandLsic() {
	Lsic *lsic = &LsicTable[DbgSelectedCpu];

	sprintf(&printbuf[0], "PEND0=%08x PEND1=%08x MASK0=%08x MASK1=%08x IPL=%08x LOMASK=%08x HIMASK=%08x\n",
		lsic->Registers[LSIC_PENDING_0],
		lsic->Registers[LSIC_PENDING_1],
		lsic->Registers[LSIC_MASK_0],
		lsic->Registers[LSIC_MASK_1],
		lsic->Registers[LSIC_IPL],
		lsic->LowIplMask,
		lsic->HighIplMask);

	DbgPutString(&printbuf[0]);
}

void DbgCommandCr() {
	XrProcessor *proc = XrProcessorTable[DbgSelectedCpu];

	int printed = 0;

	for (int i = 0; i < 32; i++) {
		if (DbgCrNames[i]) {
			if (printed && printed % 5 == 0) {
				DbgPutString("\n");
			}

			sprintf(&printbuf[0], "%10s=%08x ", DbgCrNames[i], proc->Cr[i]);
			DbgPutString(&printbuf[0]);
			printed++;
		}
	}

	DbgPutString("\n");
}

void DbgCommandCpu() {
	if (!DbgNextToken(&tokenbuf[0], CMD_MAX)) {
		DbgPutString("Usage: cpu [num]\n");
		return;
	}

	long cpunum = strtol(&tokenbuf[0], 0, 10);

	if (cpunum >= XrProcessorCount) {
		DbgPutString("No such processor\n");
		return;
	}

	DbgSelectedCpu = cpunum;
}

void DbgDumpTb(uint64_t *tb, int size) {
	for (int i = 0; i < 4; i++) {
		DbgPutString("VirPN PhyPN ASN FLAG  ");
	}

	for (int i = 0; i < size; i++) {
		if (i % 4 == 0) {
			DbgPutString("\n");
		}

		sprintf(
			&printbuf[0],
			"%05llX %05llX %03llX ",
			(tb[i] >> 32) & 0xFFFFF,
			(tb[i] >> 5) & 0xFFFFF,
			(tb[i] >> 52) & 0xFFF
		);

		DbgPutString(&printbuf[0]);

		int flags = tb[i] & 31;

		for (int j = 0; j < 5; j++) {
			if ((flags >> j) & 1) {
				DbgPutString(DbgTbFlags[j]);
			} else {
				DbgPutString(" ");
			}
		}

		DbgPutString(" ");
	}

	DbgPutString("\n");
}

void DbgCommandTb() {
	XrProcessor *proc = XrProcessorTable[DbgSelectedCpu];

	DbgPutString("ITB Contents\n");
	DbgDumpTb(&proc->Itb[0], XR_ITB_SIZE);

	DbgPutString("\nDTB Contents\n");
	DbgDumpTb(&proc->Dtb[0], XR_DTB_SIZE);
}

void DbgCommandPause() {
	XrProcessor *proc = XrProcessorTable[DbgSelectedCpu];

	if (!proc->Running) {
		DbgPutString("Already paused\n");
		return;
	}

	proc->Running = false;

	DbgPutString("Paused\n");
}

void DbgCommandResume() {
	XrProcessor *proc = XrProcessorTable[DbgSelectedCpu];

	if (proc->Running) {
		DbgPutString("Already resumed\n");
		return;
	}

	proc->Running = true;

	DbgPutString("Resumed\n");
}

void DbgCommandTranslate() {
	if (!DbgNextToken(&tokenbuf[0], CMD_MAX)) {
		DbgPutString("Usage: translate [addr] (-giptvz)\n");
		return;
	}

	uint32_t addr = DbgParseAddress(&tokenbuf[0]);

	int flags = DBG_TRANSLATE_PRINT;

	if (DbgNextToken(&tokenbuf[0], CMD_MAX) && (tokenbuf[0] == '-')) {
		flags = DbgDecodeTranslateFlags(&tokenbuf[0], flags);
	}

	XrProcessor *proc = XrProcessorTable[DbgSelectedCpu];

	uint32_t phys;

	DbgTranslateAddress(proc, addr, &phys, flags);
}

void DbgCommandPoke() {
	if (!DbgNextToken(&tokenbuf[0], CMD_MAX)) {
usage:
		DbgPutString("Usage: poke [addr] [value] (-giptvz)\n");
		return;
	}

	uint32_t addr = DbgParseAddress(&tokenbuf[0]);

	if (!DbgNextToken(&tokenbuf[0], CMD_MAX)) {
		goto usage;
	}

	uint32_t value = DbgParseAddress(&tokenbuf[0]);

	if (addr & 3) {
		DbgPutString("Unaligned address\n");
		return;
	}

	int flags = 0;

	if (DbgNextToken(&tokenbuf[0], CMD_MAX) && (tokenbuf[0] == '-')) {
		flags = DbgDecodeTranslateFlags(&tokenbuf[0], flags);
	}

	XrProcessor *proc = XrProcessorTable[DbgSelectedCpu];

	uint32_t phys;

	if (!DbgTranslateAddress(proc, addr, &phys, flags)) {
		if ((flags & DBG_TRANSLATE_PRINT) == 0) {
			DbgPutString("Failed to translate address, use -z for more info\n");
		}

		return;
	}

	sprintf(&printbuf[0], "Phys[%08x]=%08x ", phys, value);
	DbgPutString(&printbuf[0]);

	if (EBusWrite(phys, &value, 4, proc) != EBUSSUCCESS) {
		DbgPutString("Bus error on write");
	}

	DbgPutString("\n");
}

void DbgCommandPeek() {
	if (!DbgNextToken(&tokenbuf[0], CMD_MAX)) {
		DbgPutString("Usage: peek [addr] (-giptvz)\n");
		return;
	}

	uint32_t addr = DbgParseAddress(&tokenbuf[0]);

	if (addr & 3) {
		DbgPutString("Unaligned address\n");
		return;
	}

	int flags = 0;

	if (DbgNextToken(&tokenbuf[0], CMD_MAX) && (tokenbuf[0] == '-')) {
		flags = DbgDecodeTranslateFlags(&tokenbuf[0], flags);
	}

	XrProcessor *proc = XrProcessorTable[DbgSelectedCpu];

	uint32_t phys;

	if (!DbgTranslateAddress(proc, addr, &phys, flags)) {
		if ((flags & DBG_TRANSLATE_PRINT) == 0) {
			DbgPutString("Failed to translate address, use -z for more info\n");
		}

		return;
	}

	sprintf(&printbuf[0], "Phys[%08x]=", phys);
	DbgPutString(&printbuf[0]);

	uint32_t value;

	if (EBusRead(phys, &value, 4, proc) != EBUSSUCCESS) {
		DbgPutString("Bus error on read\n");
		return;
	}

	sprintf(&printbuf[0], "%08x\n", value);
	DbgPutString(&printbuf[0]);
}

struct DbgCommand DbgCommands[] = {
	{
		.command = &DbgCommandHelp,
		.name = "help",
		.help = "Display this help text.",
	},
	{
		.command = &DbgCommandClear,
		.name = "clear",
		.help = "Clear the console.",
	},
	{
		.command = &DbgCommandReg,
		.name = "reg",
		.help = "Dump register contents.",
	},
	{
		.command = &DbgCommandLsic,
		.name = "lsic",
		.help = "Dump LSIC state for the CPU.",
	},
	{
		.command = &DbgCommandCr,
		.name = "cr",
		.help = "Dump control register contents.",
	},
	{
		.command = &DbgCommandTb,
		.name = "tb",
		.help = "Dump translation buffer contents",
	},
	{
		.command = &DbgCommandPause,
		.name = "pause",
		.help = "Pause CPU execution.",
	},
	{
		.command = &DbgCommandResume,
		.name = "resume",
		.help = "Resume CPU execution.",
	},
	{
		.command = &DbgCommandCpu,
		.name = "cpu",
		.help = "Switch to examining the specified CPU's context.",
	},
	{
		.command = &DbgCommandTranslate,
		.name = "translate",
		.help = "Translate a virtual address from the perspective of the current CPU.",
	},
	{
		.command = &DbgCommandPoke,
		.name = "poke",
		.help = "Poke a new value into the given address.",
	},
	{
		.command = &DbgCommandPeek,
		.name = "peek",
		.help = "Peek the value at the given address.",
	},

	{
		.name = 0,
	},
};

void DbgProcessInput(void) {
	cmdnexttoken = &cmdline[0];

	if (DbgNextToken(&tokenbuf[0], CMD_MAX)) {
		for (int i = 0; ; i++) {
			if (!DbgCommands[i].name) {
				sprintf(&printbuf[0], "%s is not a recognized command.\n", &tokenbuf[0]);
				DbgPutString(&printbuf[0]);
				break;
			}

			if (strcmp(&tokenbuf[0], DbgCommands[i].name) == 0) {
				DbgCommands[i].command();
				break;
			}
		}
	}

	DbgPrompt();
}

void DbgInput(struct TTY *tty, uint16_t c) {
	if (c == '\r') {
		TTYPutCharacter(tty, '\r');
		TTYPutCharacter(tty, '\n');

		cmdline[cmdindex] = 0;
		cmdindex = 0;

		DbgProcessInput();

		return;
	}

	if (c == '\b') {
		if (cmdindex) {
			TTYPutCharacter(tty, '\b');
			TTYPutCharacter(tty, ' ');
			TTYPutCharacter(tty, '\b');

			cmdindex--;
		}

		return;
	}

	if (cmdindex < CMD_MAX - 1) {
		cmdline[cmdindex] = c;

		TTYPutCharacter(tty, c);

		cmdindex++;
	}
}

void DbgInit(void) {
	// Create the debugger tty.

	DbgTty = TTYCreate(132, 24, "dbg", DbgInput);

	DbgHeader();
	DbgPrompt();
}