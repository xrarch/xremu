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
	XrProcessor *proc = CpuTable[DbgSelectedCpu];

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
	XrProcessor *proc = CpuTable[DbgSelectedCpu];

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
	XrProcessor *proc = CpuTable[DbgSelectedCpu];

	DbgPutString("ITB Contents\n");
	DbgDumpTb(&proc->Itb[0], XR_ITB_SIZE);

	DbgPutString("\nDTB Contents\n");
	DbgDumpTb(&proc->Dtb[0], XR_DTB_SIZE);
}

void DbgCommandPause() {
	XrProcessor *proc = CpuTable[DbgSelectedCpu];

	if (!proc->Running) {
		DbgPutString("Already paused\n");
		return;
	}

	proc->Running = false;

	DbgPutString("Paused\n");
}

void DbgCommandResume() {
	XrProcessor *proc = CpuTable[DbgSelectedCpu];

	if (proc->Running) {
		DbgPutString("Already resumed\n");
		return;
	}

	proc->Running = true;

	DbgPutString("Resumed\n");
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