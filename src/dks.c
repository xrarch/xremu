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
	bool Spinning;
	uint32_t PlatterLocation;
	uint32_t HeadLocation;
	uint32_t OperationInterval;
	uint32_t SeekTo;
	uint32_t ConsecutiveZeroSeeks;
};

struct DKSDisk DKSDisks[DKSDISKS];

int DKSInfoWhat = 0;

struct DKSDisk *DKSSelectedDrive = 0;

uint32_t DKSStatus = 0;
uint32_t DKSCompleted = 0;

uint32_t DKSPortA = 0;
uint32_t DKSPortB = 0;

bool DKSDoInterrupt = false;
bool DKSAsynchronous = false;

void DKSInfo(int what) {
	DKSInfoWhat = what;

	if (DKSDoInterrupt)
		LSICInterrupt(0x3);
}

void DKSReset() {
	DKSDoInterrupt = false;
	DKSPortA = 0;
	DKSPortB = 0;
	DKSSelectedDrive = 0;
}

void DKSOperation(uint32_t dt) {
	for (int i = 0; i < DKSDISKS; i++) {
		if (!DKSDisks[i].Present)
			break;

		if (!(DKSStatus&(1<<i))) {
			if (DKSDisks[i].Spinning) {
				DKSDisks[i].PlatterLocation += BLOCKSPERMS;
				DKSDisks[i].PlatterLocation %= LBAPERTRACK;
			}
		} else if (dt >= DKSDisks[i].OperationInterval) {
			DKSStatus &= ~(1<<i);
			DKSCompleted |= 1<<i;

			DKSDisks[i].OperationInterval = 0;
			DKSDisks[i].PlatterLocation = LBA_TO_BLOCK(DKSDisks[i].SeekTo);
			DKSDisks[i].HeadLocation = LBA_TO_CYLINDER(DKSDisks[i].SeekTo);

			DKSInfo(0);
		} else {
			DKSDisks[i].OperationInterval -= dt;
		}
	}
}

void DKSSeek(uint32_t lba) {
	struct DKSDisk *disk = DKSSelectedDrive;

	// set up the disk for seek

	int cylseek = abs((int)(disk->HeadLocation) - (int)(LBA_TO_CYLINDER(lba))) / (CYLPERDISK/FULLSEEKTIMEMS);

	if (disk->HeadLocation != LBA_TO_CYLINDER(lba))
		cylseek += SETTLETIMEMS;

	int blockseek = LBA_TO_BLOCK(lba) - disk->PlatterLocation;

	if (blockseek < 0)
		blockseek += LBAPERTRACK;

	disk->OperationInterval = cylseek + blockseek/(LBAPERTRACK/ROTATIONTIMEMS);
	disk->SeekTo = lba;

	if (disk->OperationInterval == 0) {
		disk->ConsecutiveZeroSeeks += blockseek;

		if (disk->ConsecutiveZeroSeeks > (LBAPERTRACK/ROTATIONTIMEMS)) {
			disk->OperationInterval = 1;
			disk->ConsecutiveZeroSeeks = 0;
		}
	} else {
		disk->ConsecutiveZeroSeeks = 0;
	}
}

uint8_t DKSBlockBuffer[512*DKSDISKS];

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

			if (DKSStatus&(1<<DKSSelectedDrive->ID))
				return EBUSERROR;

			fseek(DKSSelectedDrive->DiskImage, DKSPortA*512, SEEK_SET);

			fread(&DKSBlockBuffer[512 * DKSSelectedDrive->ID], 512, 1, DKSSelectedDrive->DiskImage);

			// printf("read %d: %d\n", DKSSelectedDrive->ID, DKSPortA);

			if (!DKSAsynchronous) {
				DKSCompleted |= 1<<DKSSelectedDrive->ID;
				DKSInfo(0);
			} else {
				DKSStatus |= 1<<DKSSelectedDrive->ID;
				DKSSeek(DKSPortA);
			}

			return EBUSSUCCESS;

		case 3:
			// write block
			if (!DKSSelectedDrive)
				return EBUSERROR;

			if (DKSPortA >= DKSSelectedDrive->BlockCount)
				return EBUSERROR;

			if (DKSStatus&(1<<DKSSelectedDrive->ID))
				return EBUSERROR;

			fseek(DKSSelectedDrive->DiskImage, DKSPortA*512, SEEK_SET);

			fwrite(&DKSBlockBuffer[512 * DKSSelectedDrive->ID], 512, 1, DKSSelectedDrive->DiskImage);

			// printf("write %d: %d\n", DKSSelectedDrive->ID, DKSPortA);

			if (!DKSAsynchronous) {
				DKSCompleted |= 1<<DKSSelectedDrive->ID;
				DKSInfo(0);
			} else {
				DKSStatus |= 1<<DKSSelectedDrive->ID;
				DKSSeek(DKSPortA);
			}

			return EBUSSUCCESS;

		case 4:
			// read info
			DKSPortA = DKSInfoWhat;
			DKSPortB = DKSCompleted;

			DKSCompleted = 0;

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
	*value = DKSStatus;

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
	disk->Spinning = true;

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