#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ebus.h"
#include "pboard.h"
#include "dks.h"

#include "fastmutex.h"
#include "xr.h"

#include "lsic.h"

XrMutex ControllerMutex;

typedef struct _DKSDisk {
	XrSchedulable Schedulable;
	FILE *DiskImage;
	int ID;
	int Present;
	bool Spinning;
	bool Completing;
	uint8_t TemporaryBuffer[4096];
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

static inline void DKSInfo() {
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
		printf("dks%d: %s %d (%d sectors) @ %08x\n", disk->ID, disk->IoType == DKS_READ ? "read" : "write", disk->TransferSector, disk->TransferCount, disk->TransferAddress);
	}

	// Unlock the controller mutex across lengthy IO.

	disk->Completing = 1;

	XrUnlockMutex(&ControllerMutex);

	fseek(disk->DiskImage, disk->TransferSector*512, SEEK_SET);

	if (disk->IoType == DKS_READ) {
		fread(&disk->TemporaryBuffer[0], disk->TransferCount*512, 1, disk->DiskImage);

		EBusWrite(disk->TransferAddress, &disk->TemporaryBuffer[0], disk->TransferCount*512, 0);
	} else {
		EBusRead(disk->TransferAddress, &disk->TemporaryBuffer[0], disk->TransferCount*512, 0);

		fwrite(&disk->TemporaryBuffer[0], disk->TransferCount*512, 1, disk->DiskImage);
	}

	XrLockMutex(&ControllerMutex);

	disk->Completing = 0;

	// Set parameters and send interrupt.

	DKSStatus &= ~(1 << disk->ID);
	DKSCompleted |= 1 << disk->ID;

	disk->PlatterLocation = LBA_TO_SECTOR(disk, disk->SeekTo);
	disk->HeadLocation = LBA_TO_CYLINDER(disk, disk->SeekTo);

	DKSInfo();
}

void DKSStartTimeslice(XrSchedulable *schedulable, int dt) {
	schedulable->Timeslice = dt;
}

int Spins = 0;

void DKSSchedule(XrSchedulable *schedulable) {
	DKSDisk *disk = schedulable->Context;
	uint32_t id = disk->ID;

	XrLockMutex(&ControllerMutex);

	// Note that the conditional disk->Completing avoids a race
	// condition that may otherwise occur because DKSCompleteTransfer releases
	// the controller mutex and is also called from DKSDispatchIO if the
	// operation interval was 0.

	if (((DKSStatus & (1 << disk->ID)) == 0) || disk->Completing) {
		// Soak up all of the time by spinning the disk.

		if (disk->Spinning) {
			disk->PlatterLocation += SECTOR_PER_MS(disk) * schedulable->Timeslice;
			disk->PlatterLocation %= disk->SectorsPerTrack;
		}

		schedulable->Timeslice = 0;
	} else if (schedulable->Timeslice >= disk->OperationInterval) {
		// There is sufficient time to satisfy the current operation.

		// printf("sufficient %d %d\n", disk->OperationInterval, schedulable->Timeslice);

		schedulable->Timeslice -= disk->OperationInterval;

		DKSCompleteTransfer(disk);
	} else {
		// There is not sufficient time to satisfy the current operation.

		// printf("insufficient %d %d\n", disk->OperationInterval, schedulable->Timeslice);

		disk->OperationInterval -= schedulable->Timeslice;
		schedulable->Timeslice = 0;
	}

	XrUnlockMutex(&ControllerMutex);
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
	DKSDisk *disk = DKSSelectedDrive;

	if (!disk) {
		return EBUSERROR;
	}

	if ((DKSPortA + DKSTransferCount) < DKSPortA) {
		// overflow!
		return EBUSERROR;
	}

	if ((DKSPortA + DKSTransferCount) >= disk->SectorCount) {
		return EBUSERROR;
	}

	if (DKSStatus & (1 << disk->ID)) {
		return EBUSERROR;
	}

	disk->IoType = type;

	disk->TransferSector = DKSPortA;
	disk->TransferAddress = DKSTransferAddress;
	disk->TransferCount = DKSTransferCount;

	// Mark the disk busy.

	DKSStatus |= 1 << disk->ID;

	if (DKSAsynchronous) {
		// Calculate seek values.

		DKSSeek();
	} else {
		// Zero seek time.

		disk->OperationInterval = 0;
	}

	if (disk->OperationInterval == 0) {
		// Immediately complete the operation.

		DKSCompleteTransfer(disk);
	}

	return EBUSSUCCESS;
}

int DKSWriteCMD(uint32_t port, uint32_t type, uint32_t value, void *proc) {
	int status;

	DKSDisk *setrunlock = 0;

	XrLockMutex(&ControllerMutex);

	switch(value) {
		case 1:
			// select drive

			if ((DKSPortA < DKSDISKS) && (DKSDisks[DKSPortA].Present)) {
				DKSSelectedDrive = &DKSDisks[DKSPortA];
			} else {
				DKSSelectedDrive = 0;
			}

			status = EBUSSUCCESS;
			break;

		case 2:
			// read sectors

			status = DKSDispatchIO(DKS_READ);

			if (status == EBUSSUCCESS) {
				setrunlock = DKSSelectedDrive;
			}

			break;

		case 3:
			// write sectors

			status = DKSDispatchIO(DKS_WRITE);

			if (status == EBUSSUCCESS) {
				setrunlock = DKSSelectedDrive;
			}

			break;

		case 4:
			// read info

			DKSPortA = 0;

			DKSPortB = DKSCompleted;
			DKSCompleted = 0;

			status = EBUSSUCCESS;
			break;

		case 5:
			// poll drive

			if ((DKSPortA < DKSDISKS) && (DKSDisks[DKSPortA].Present)) {
				DKSPortB = DKSDisks[DKSPortA].SectorCount;
				DKSPortA = 1;
			} else {
				DKSPortA = 0;
				DKSPortB = 0;
			}

			status = EBUSSUCCESS;
			break;

		case 6:
			// enable interrupts

			DKSDoInterrupt = true;

			status = EBUSSUCCESS;
			break;

		case 7:
			// disable interrupts

			DKSDoInterrupt = false;

			status = EBUSSUCCESS;
			break;

		case 8:
			// set transfer count

			if (DKSPortA == 0 || DKSPortA > 8) {
				// Zero length and greater than 8 sectors are both forbidden

				status = EBUSERROR;
				break;
			}

			DKSTransferCount = DKSPortA;

			status = EBUSSUCCESS;
			break;

		case 9:
			// set transfer address

			DKSTransferAddress = DKSPortA & ~511;

			status = EBUSSUCCESS;
			break;
	}

	XrUnlockMutex(&ControllerMutex);

	if (setrunlock &&
		(setrunlock->Schedulable.RunLock != ((XrProcessor *)proc)->Schedulable.RunLock)) {
		// We decided to set the disk's runlock reference to point to that of
		// the current processor. This has the effect of tending to claim the
		// simulation of the disk latency for the thread currently simulating
		// the processor that initiates a request, which improves latency
		// simulation when there are multiple simulation threads.

		((XrProcessor *)proc)->Schedulable.Next = &DKSSelectedDrive->Schedulable;

		XrLockMutex(&setrunlock->Schedulable.InherentRunLock);

		DKSSelectedDrive->Schedulable.RunLock = ((XrProcessor *)proc)->Schedulable.RunLock;

		XrUnlockMutex(&setrunlock->Schedulable.InherentRunLock);
	}

	return status;
}

int DKSReadCMD(uint32_t port, uint32_t type, uint32_t *value, void *proc) {
	XrLockMutex(&ControllerMutex);

	*value = DKSStatus;

	if (DKSStatus != 0) {
		// People pretty much only read from this command port while doing
		// polled operation of the disk controller. So, simulate a bunch of
		// PAUSE instruction executions by the reading processor so that the
		// outermost loop of the emulator yields for disk time scheduling.

		((XrProcessor *)proc)->PauseCalls += 64;
	}

	XrUnlockMutex(&ControllerMutex);

	return EBUSSUCCESS;
}

int DKSWritePortA(uint32_t port, uint32_t type, uint32_t value, void *proc) {
	XrLockMutex(&ControllerMutex);
	DKSPortA = value;
	XrUnlockMutex(&ControllerMutex);

	return EBUSSUCCESS;
}

int DKSReadPortA(uint32_t port, uint32_t type, uint32_t *value, void *proc) {
	*value = DKSPortA;

	return EBUSSUCCESS;
}

int DKSWritePortB(uint32_t port, uint32_t type, uint32_t value, void *proc) {
	XrLockMutex(&ControllerMutex);
	DKSPortB = value;
	XrUnlockMutex(&ControllerMutex);

	return EBUSSUCCESS;
}

int DKSReadPortB(uint32_t port, uint32_t type, uint32_t *value, void *proc) {
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
		bytes &= ~511;
	}

	disk->Present = 1;
	disk->Spinning = true;
	disk->ConsecutiveZeroSeeks = 0;
	disk->PlatterLocation = 0;
	disk->HeadLocation = 0;
	disk->OperationInterval = 0;
	disk->SeekTo = 0;
	disk->Completing = 0;

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
		if (DKSDisks[i].Present && DKSAsynchronous) {
			XrInitializeSchedulable(&DKSDisks[i].Schedulable, &DKSSchedule, &DKSStartTimeslice, &DKSDisks[i]);
		}

		DKSDisks[i].ID = i;
	}

	XrInitializeMutex(&ControllerMutex);

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