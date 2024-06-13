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
	bool Spinning;
	uint8_t TemporaryBuffer[512];
	uint32_t TransferAddress;
	uint32_t TransferSector;
	uint32_t TransferCount;
	uint32_t IoType;
	uint32_t SectorCount;
	uint32_t SectorsPerTrack;
	uint32_t CylinderCount;
	uint32_t PlatterLocation; // selects a sector
	uint32_t HeadLocation; // selects a cylinder
	uint32_t OperationInterval;
	uint32_t SeekTo;
	uint32_t ConsecutiveZeroSeeks;
} DKSDisk;

#define DKS_READ 1
#define DKS_WRITE 2

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

// Accumulated disk interval before paying for the time cost all at once.

#define ACCUMULATED_INTERVAL 50

DKSDisk DKSDisks[DKSDISKS];

int DKSInfoWhat = 0;

DKSDisk *DKSSelectedDrive = 0;

uint32_t DKSStatus = 0;
uint32_t DKSCompleted = 0;

uint32_t DKSPortA = 0;
uint32_t DKSPortB = 0;

uint32_t DKSTransferCount = 0;
uint32_t DKSTransferAddress = 0;

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
	DKSTransferCount = 0;
	DKSTransferAddress = 0;
}

void DKSCompleteTransfer(DKSDisk *disk) {
	// Complete the transfer.

	if (DKSPrint) {
		printf("dks%d: %s %d @ %08x\n", disk->ID, disk->IoType == DKS_READ ? "read" : "write", disk->TransferSector, disk->TransferAddress);
	}

	fseek(disk->DiskImage, disk->TransferSector*512, SEEK_SET);

	for (int i = 0; i < disk->TransferCount; i++) {
		if (disk->IoType == DKS_READ) {
			fread(&disk->TemporaryBuffer[0], 512, 1, disk->DiskImage);

			EBusWrite(disk->TransferAddress, &disk->TemporaryBuffer[0], 512);
		} else {
			EBusRead(disk->TransferAddress, &disk->TemporaryBuffer[0], 512);

			fwrite(&disk->TemporaryBuffer[0], 512, 1, disk->DiskImage);
		}

		disk->TransferAddress += 512;
	}

	// Set parameters and send interrupt.

	DKSStatus &= ~(1 << disk->ID);
	DKSCompleted |= 1 << disk->ID;

	disk->PlatterLocation = LBA_TO_SECTOR(disk, disk->SeekTo);
	disk->HeadLocation = LBA_TO_CYLINDER(disk, disk->SeekTo);

	DKSInfo(0);
}

uint32_t DKSCallback(uint32_t interval, void *param) {
	DKSDisk *disk = param;

	LockIoMutex();

	if ((DKSStatus & (1 << disk->ID)) != 0) {
#ifdef EMSCRIPTEN
		if (interval < disk->OperationInterval) {
			// There's still more left.

			disk->OperationInterval -= interval;

			UnlockIoMutex();

			return 0;
		}
#endif

		DKSCompleteTransfer(disk);

		disk->OperationInterval = 0;
	}

	UnlockIoMutex();

	return 0;
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
#ifdef EMSCRIPTEN
		} else {
			DKSCallback(dt, disk);
#endif
		}
	}
}

void DKSSeek() {
	uint32_t lba = DKSPortA + DKSTransferCount;

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

	disk->OperationInterval += sectorseek + cylseek;
	disk->SeekTo = lba;

	if ((sectorseek + cylseek) == 0) {
		// If the interval rounded down to zero, increment the consecutive zero
		// seeks by the number of sectors the platter had to rotate by.

		disk->ConsecutiveZeroSeeks += sectorseek;

		if (disk->ConsecutiveZeroSeeks > SECTOR_PER_MS(disk)) {
			// If it exceeded the number of sectors the platter can rotate per
			// millisecond, then set the operation interval to 1ms. Otherwise it
			// could keep doing zero-time disk transfers forever due to the
			// rounding down of the interval to the previous millisecond, which
			// would be goofy.

			disk->OperationInterval += 1;
			disk->ConsecutiveZeroSeeks = 0;
		}
	}
}

int DKSDispatchIO(uint32_t type) {
	if (!DKSSelectedDrive) {
		return EBUSERROR;
	}

	if ((DKSPortA + DKSTransferCount) < DKSPortA) {
		// overflow!
		return EBUSERROR;
	}

	if ((DKSPortA + DKSTransferCount) >= DKSSelectedDrive->SectorCount) {
		return EBUSERROR;
	}

	if (DKSStatus & (1 << DKSSelectedDrive->ID)) {
		return EBUSERROR;
	}

	// Mark the disk busy.

	DKSStatus |= 1 << DKSSelectedDrive->ID;

	DKSSelectedDrive->IoType = type;

	if (DKSAsynchronous) {
		// Calculate seek values.

		DKSSeek();
	} else {
		// Zero seek time.

		DKSSelectedDrive->OperationInterval = 0;
	}

	DKSSelectedDrive->TransferSector = DKSPortA;
	DKSSelectedDrive->TransferAddress = DKSTransferAddress;
	DKSSelectedDrive->TransferCount = DKSTransferCount;

#ifndef EMSCRIPTEN
	// Enqueue a timer callback to perform the operation and signal the
	// completion interrupt. If the interval is 0, this should dispatch it
	// to a worker thread for immediate processing. This keeps the CPU threads
	// from having to eat the cost of the read and write syscalls.

	EnqueueCallback(DKSSelectedDrive->OperationInterval, &DKSCallback, DKSSelectedDrive);
#endif

	return EBUSSUCCESS;
}

int DKSWriteCMD(uint32_t port, uint32_t type, uint32_t value) {
	int status;

	switch(value) {
		case 1:
			// select drive

			if ((DKSPortA < DKSDISKS) && (DKSDisks[DKSPortA].Present))
				DKSSelectedDrive = &DKSDisks[DKSPortA];
			else
				DKSSelectedDrive = 0;

			return EBUSSUCCESS;

		case 2:
			// read sectors

			return DKSDispatchIO(DKS_READ);

		case 3:
			// write sectors

			return DKSDispatchIO(DKS_WRITE);

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

		case 8:
			// set transfer count

			if (DKSPortA == 0 || DKSPortA > 8) {
				// Zero length and greater than 8 sectors are both forbidden

				return EBUSERROR;
			}

			DKSTransferCount = DKSPortA;

			return EBUSSUCCESS;

		case 9:
			// set transfer address

			DKSTransferAddress = DKSPortA & ~511;

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
		fprintf(stderr, "%s: couldn't open disk image\n", path);
		return false;
	}

	setvbuf(disk->DiskImage, 0, _IONBF, 0);

	fseek(disk->DiskImage, 0, SEEK_END);
	uint32_t bytes = ftell(disk->DiskImage);

	disk->SectorCount = bytes / 512;

	if (bytes & 511) {
		fprintf(stderr, "Warning: %s: size %d not a multiple of 512, rounding down to %d\n", path, bytes, disk->SectorCount * 512);
		bytes &= ~(511);
	}

	disk->Present = 1;
	disk->Spinning = true;

	// Roughly calculate a disk geometry for use by disk seek simulation.
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