#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ebus.h"
#include "pboard.h"
#include "dks.h"

#include "lsic.h"

struct DKSDisk {
	FILE *DiskImage;
	int ID;
	int Present;
	uint32_t BlockCount;
};

struct DKSDisk DKSDisks[DKSDISKS];

int DKSInfoWhat = 0;
int DKSInfoDetails = 0;

struct DKSDisk *DKSSelectedDrive = 0;

uint32_t DKSPortA = 0;
uint32_t DKSPortB = 0;

bool DKSDoInterrupt = false;

bool DKSAsynchronous = false;

bool DKSSpinning = true;

uint32_t DKSHeadLocation = 0;

void DKSInfo(int what, int details) {
	DKSInfoWhat = what;
	DKSInfoDetails = details;

	if (DKSDoInterrupt)
		LSICInterrupt(0x3);
}

void DKSReset() {
	DKSDoInterrupt = false;
	DKSPortA = 0;
	DKSPortB = 0;
	DKSSelectedDrive = 0;
}

uint32_t DKSOperationInterval = 0;
uint32_t DKSOperationCurrent = 0;
uint32_t DKSSeekTo = 0;

void DKSOperation(uint32_t dt) {
	if (!DKSOperationCurrent) {
		if (DKSSpinning) {
			DKSHeadLocation += BLOCKSPERMS;
			DKSHeadLocation %= LBAPERTRACK;
		}
		return;
	}

	if (dt >= DKSOperationInterval) {
		DKSOperationCurrent = 0;
		DKSHeadLocation = DKSSeekTo;

		DKSInfo(0, DKSPortA);
	} else {
		DKSOperationInterval -= dt;
	}
}

void DKSSeek(uint32_t lba) {
	// set up the disk for seek

	int cylseek = abs((int)(LBA_TO_CYLINDER(DKSHeadLocation)) - (int)(LBA_TO_CYLINDER(lba))) / (CYLPERDISK/FULLSEEKTIMEMS);

	if (LBA_TO_CYLINDER(DKSHeadLocation) != LBA_TO_CYLINDER(lba))
		cylseek += SETTLETIMEMS;

	int blockseek = LBA_TO_BLOCK(lba) - LBA_TO_BLOCK(DKSHeadLocation);

	if (blockseek < 0)
		blockseek += LBAPERTRACK;

	blockseek /= (LBAPERTRACK/ROTATIONTIMEMS);

	DKSOperationInterval = cylseek + blockseek;

	DKSSeekTo = lba;
}

uint32_t DKSBlockBuffer[128];

int DKSWriteCMD(uint32_t port, uint32_t type, uint32_t value) {
	switch(value) {
		case 1:
			// select drive
			if ((DKSPortA < DKSDISKS) && (DKSDisks[DKSPortA].Present))
				DKSSelectedDrive = &DKSDisks[DKSPortA];
			else
				DKSSelectedDrive = 0;

			return EBUSSUCCESS;

		case 2:
			// read block
			if (!DKSSelectedDrive)
				return EBUSERROR;

			if (DKSPortA >= DKSSelectedDrive->BlockCount)
				return EBUSERROR;

			fseek(DKSSelectedDrive->DiskImage, DKSPortA*512, SEEK_SET);

			fread(&DKSBlockBuffer, 512, 1, DKSSelectedDrive->DiskImage);

			// printf("read %d: %d\n", DKSSelectedDrive->ID, DKSPortA);

			if (!DKSAsynchronous) {
				DKSInfo(0, DKSPortA);
			} else {
				DKSOperationCurrent = 2;
				DKSSeek(DKSPortA);
			}

			return EBUSSUCCESS;

		case 3:
			// write block
			if (!DKSSelectedDrive)
				return EBUSERROR;

			if (DKSPortA >= DKSSelectedDrive->BlockCount)
				return EBUSERROR;

			fseek(DKSSelectedDrive->DiskImage, DKSPortA*512, SEEK_SET);

			fwrite(&DKSBlockBuffer, 512, 1, DKSSelectedDrive->DiskImage);

			// printf("write %d: %d\n", DKSSelectedDrive->ID, DKSPortA);

			if (!DKSAsynchronous) {
				DKSInfo(0, DKSPortA);
			} else {
				DKSOperationCurrent = 3;
				DKSSeek(DKSPortA);
			}

			return EBUSSUCCESS;

		case 4:
			// read info
			DKSPortA = DKSInfoWhat;
			DKSPortB = DKSInfoDetails;

			return EBUSSUCCESS;

		case 5:
			// poll drive
			if ((DKSPortA < DKSDISKS) && (DKSDisks[DKSPortA].Present)) {
				DKSPortB = DKSDisks[DKSPortA].BlockCount;
				DKSPortA = 1;
			} else {
				DKSPortA = 0;
				DKSPortB = 0;
			}

			return EBUSSUCCESS;

		case 6:
			// enable interrupts
			DKSDoInterrupt = true;

			return EBUSSUCCESS;

		case 7:
			// disable interrupts
			DKSDoInterrupt = false;

			return EBUSSUCCESS;
	}

	return EBUSERROR;
}

int DKSReadCMD(uint32_t port, uint32_t type, uint32_t *value) {
	if (DKSOperationCurrent)
		*value = DKSOperationCurrent;
	else
		*value = 0;

	return EBUSSUCCESS;
}

int DKSWritePortA(uint32_t port, uint32_t type, uint32_t value) {
	DKSPortA = value;
	return EBUSSUCCESS;
}

int DKSReadPortA(uint32_t port, uint32_t type, uint32_t *value) {
	*value = DKSPortA;
	return EBUSSUCCESS;
}

int DKSWritePortB(uint32_t port, uint32_t type, uint32_t value) {
	DKSPortB = value;
	return EBUSSUCCESS;
}

int DKSReadPortB(uint32_t port, uint32_t type, uint32_t *value) {
	*value = DKSPortB;
	return EBUSSUCCESS;
}

int DKSAttachImage(char *path) {
	struct DKSDisk *disk = 0;

	for (int i = 0; i < DKSDISKS; i++) {
		if (!DKSDisks[i].Present) {
			// found a free disk
			disk = &DKSDisks[i];
			break;
		}
	}

	if (!disk) {
		fprintf(stderr, "maximum disks reached\n");
		return false;
	}

	disk->DiskImage = fopen(path, "r+");

	if (!disk->DiskImage) {
		fprintf(stderr, "couldn't open disk image\n");
		return false;
	}

	fseek(disk->DiskImage, 0, SEEK_END);

	disk->BlockCount = ftell(disk->DiskImage)/512;

	disk->Present = 1;

	printf("%s as dks%d (%d blocks)\n", path, disk->ID, disk->BlockCount);

	return true;
}

void DKSInit() {
	for (int i = 0; i < DKSDISKS; i++) {
		if (DKSDisks[i].Present) {
			fclose(DKSDisks[i].DiskImage);
			DKSDisks[i].Present = 0;
		}
		DKSDisks[i].ID = i;
	}

	CitronPorts[0x19].Present = 1;
	CitronPorts[0x19].ReadPort = DKSReadCMD;
	CitronPorts[0x19].WritePort = DKSWriteCMD;

	CitronPorts[0x1A].Present = 1;
	CitronPorts[0x1A].ReadPort = DKSReadPortA;
	CitronPorts[0x1A].WritePort = DKSWritePortA;

	CitronPorts[0x1B].Present = 1;
	CitronPorts[0x1B].ReadPort = DKSReadPortB;
	CitronPorts[0x1B].WritePort = DKSWritePortB;
}