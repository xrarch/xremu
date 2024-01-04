#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ebus.h"
#include "pboard.h"
#include "dks.h"

#include "lsic.h"

typedef struct _DKSDisk {
	FILE *DiskImage;
	int ID;
	int Present;
	uint32_t SectorCount;
	uint32_t SectorsPerTrack;
	uint32_t CylinderCount;
	bool Spinning;
	uint32_t PlatterLocation; // selects a sector
	uint32_t HeadLocation; // selects a cylinder
	uint32_t OperationInterval;
	uint32_t SeekTo;
	uint32_t ConsecutiveZeroSeeks;
} DKSDisk;

// fake disk geometry for simple seek time simulation

#define TRACKS_PER_CYLINDER 4

#define RPM 5400.0
#define RPS (RPM/60.0)
#define MS_PER_ROTATION (1000.0/RPS)

#define SETTLE_TIME_MS 3.0

#define SEEK_PER_CYL_MS 0.1

#define SECTOR_PER_MS(_disk) ((_disk)->SectorsPerTrack/MS_PER_ROTATION)

#define LBA_TO_SECTOR(_disk, _lba) ((_lba)%(_disk)->SectorsPerTrack)
#define LBA_TO_TRACK(_disk, _lba) ((_lba)/(_disk)->SectorsPerTrack)
#define LBA_TO_CYLINDER(_disk, _lba) (LBA_TO_TRACK(_disk, _lba)/TRACKS_PER_CYLINDER)

DKSDisk DKSDisks[DKSDISKS];

int DKSInfoWhat = 0;

DKSDisk *DKSSelectedDrive = 0;

uint32_t DKSStatus = 0;
uint32_t DKSCompleted = 0;

uint32_t DKSPortA = 0;
uint32_t DKSPortB = 0;

bool DKSDoInterrupt = false;
bool DKSAsynchronous = false;
bool DKSPrint = false;

void DKSInfo(int what) {
	DKSInfoWhat = what;

	if (DKSDoInterrupt)
		LsicInterrupt(0x3);
}

void DKSReset() {
	DKSDoInterrupt = false;
	DKSPortA = 0;
	DKSPortB = 0;
	DKSSelectedDrive = 0;
}

void DKSInterval(uint32_t dt) {
	for (int i = 0; i < DKSDISKS; i++) {
		DKSDisk *disk = &DKSDisks[i];

		if (!disk->Present)
			break;

		if ((DKSStatus & (1 << i)) == 0) {
			if (disk->Spinning) {
				disk->PlatterLocation += SECTOR_PER_MS(disk);
				disk->PlatterLocation %= disk->SectorsPerTrack;
			}
		} else if (dt >= disk->OperationInterval) {
			DKSStatus &= ~(1 << i);
			DKSCompleted |= 1 << i;

			disk->OperationInterval = 0;
			disk->PlatterLocation = LBA_TO_SECTOR(disk, disk->SeekTo);
			disk->HeadLocation = LBA_TO_CYLINDER(disk, disk->SeekTo);

			DKSInfo(0);
		} else {
			disk->OperationInterval -= dt;
		}
	}
}

void DKSSeek(uint32_t lba) {
	DKSDisk *disk = DKSSelectedDrive;

	// Set up the disk for seek.

	// Calculate how many cylinders the head has to seek across the disk
	// surface.

	double cylseek = ((int)disk->HeadLocation - (int)LBA_TO_CYLINDER(disk, lba));

	if (cylseek < 0) {
		// The head has to seek either back or forth so this should be an
		// absolute value to reflect the cylinder distance.

		cylseek = -cylseek;
	}

	// Multiply by the number of milliseconds it takes to seek one cylinder.

	cylseek *= SEEK_PER_CYL_MS;

	if (disk->HeadLocation != LBA_TO_CYLINDER(disk, lba)) {
		// We moved cylinders, so add a settle time.

		cylseek += SETTLE_TIME_MS;
	}

	// Calculate how many sectors the platter has to rotate by.

	double sectorseek = ((int)LBA_TO_SECTOR(disk, lba) - (int)disk->PlatterLocation);

	if (sectorseek < 0) {
		// The platter is circular, so a negative sector seek means that the
		// sector is "behind". Therefore we have to wait that many sectors less
		// than a full rotation.

		sectorseek += disk->SectorsPerTrack;
	}

	// Divide by the number of sectors that the platter rotates by per
	// millisecond.

	sectorseek /= SECTOR_PER_MS(disk);

	// Set the operation interval to the platter rotation time plus the
	// head seek time.

	disk->OperationInterval = sectorseek + cylseek;
	disk->SeekTo = lba;

	if (disk->OperationInterval == 0) {
		// If the interval rounded down to zero, increment the consecutive zero
		// seeks by the number of sectors the platter had to rotate by.

		disk->ConsecutiveZeroSeeks += sectorseek;

		if (disk->ConsecutiveZeroSeeks > SECTOR_PER_MS(disk)) {
			// If it exceeded the number of sectors the platter can rotate per
			// millisecond, then set the operation interval to 1ms.

			disk->OperationInterval = 1;
			disk->ConsecutiveZeroSeeks = 0;
		}
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

			if (DKSPortA >= DKSSelectedDrive->SectorCount)
				return EBUSERROR;

			if (DKSStatus&(1<<DKSSelectedDrive->ID))
				return EBUSERROR;

			fseek(DKSSelectedDrive->DiskImage, DKSPortA*512, SEEK_SET);

			fread(&DKSBlockBuffer[512 * DKSSelectedDrive->ID], 512, 1, DKSSelectedDrive->DiskImage);

			if (DKSPrint)
				printf("dks%d: read  %d\n", DKSSelectedDrive->ID, DKSPortA);

			DKSSeek(DKSPortA);

			if (!DKSAsynchronous || DKSSelectedDrive->OperationInterval == 0) {
				DKSCompleted |= 1<<DKSSelectedDrive->ID;
				DKSInfo(0);
			} else {
				DKSStatus |= 1<<DKSSelectedDrive->ID;
			}

			return EBUSSUCCESS;

		case 3:
			// write block
			if (!DKSSelectedDrive)
				return EBUSERROR;

			if (DKSPortA >= DKSSelectedDrive->SectorCount)
				return EBUSERROR;

			if (DKSStatus&(1<<DKSSelectedDrive->ID))
				return EBUSERROR;

			fseek(DKSSelectedDrive->DiskImage, DKSPortA*512, SEEK_SET);

			fwrite(&DKSBlockBuffer[512 * DKSSelectedDrive->ID], 512, 1, DKSSelectedDrive->DiskImage);

			if (DKSPrint)
				printf("dks%d: write %d\n", DKSSelectedDrive->ID, DKSPortA);

			DKSSeek(DKSPortA);

			if (!DKSAsynchronous || DKSSelectedDrive->OperationInterval == 0) {
				DKSCompleted |= 1<<DKSSelectedDrive->ID;
				DKSInfo(0);
			} else {
				DKSStatus |= 1<<DKSSelectedDrive->ID;
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
				DKSPortB = DKSDisks[DKSPortA].SectorCount;
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
	DKSDisk *disk = 0;
	int i = 0;

	for (; i < DKSDISKS; i++) {
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

	disk->DiskImage = fopen(path, "r+b");

	if (!disk->DiskImage) {
		fprintf(stderr, "couldn't open disk image\n");
		return false;
	}

	fseek(disk->DiskImage, 0, SEEK_END);
	uint32_t bytes = ftell(disk->DiskImage);

	disk->SectorCount = bytes / 512;

	if (bytes & 511) {
		fprintf(stderr, "Warning: %s: size %d not a multiple of 512, rounding down to %d\n", path, bytes, disk->SectorCount * 512);
		bytes &= ~(511);
	}

	disk->Present = 1;
	disk->Spinning = true;

	// Roughly calculate geometry for disk seek simulation.
	// The code is based on this algorithm from 86Box:
	// https://github.com/86Box/86Box/blob/b5bb35688c19bfe07bc705bce05d9005d9884bb9/src/win/win_settings.c#L3403-L3424

	uint32_t headspercyl;

	if ((disk->SectorCount % 17) || (disk->SectorCount > 0x44000)) {
		// Disk sector count isn't a multiple of 17, or more than 0x44000
		// sectors. ATA-5 suggests the following parameters.

		disk->SectorsPerTrack = 63;
		headspercyl = 16;
	} else {
		// Disk sector count is a multiple of 17, so we can set the sectors per
		// track to 17 and calculate the geometry based on that.

		disk->SectorsPerTrack = 17;

		if (disk->SectorCount <= 0xCC00) {
			headspercyl = 4;
		} else if (((disk->SectorCount % 6) == 0) && (disk->SectorCount <= 0x19800)) {
			headspercyl = 6;
		} else {
			int i;

			for (i = 5; i < 16; i++) {
				if (((disk->SectorCount % i) == 0) && (disk->SectorCount <= (i * 17 * 1024))) {
					break;
				}

				if (i == 5) {
					i++;
				}
			}

			headspercyl = i;
		}
	}

	disk->CylinderCount = disk->SectorCount / headspercyl / disk->SectorsPerTrack;

	printf("%s as dks%d (%d sectors) (cyl=%d, sec=%d)\n", path, i, disk->SectorCount, disk->CylinderCount, disk->SectorsPerTrack);

	return true;
}

void DKSInit() {
	for (int i = 0; i < DKSDISKS; i++) {
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