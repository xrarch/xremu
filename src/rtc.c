#include <SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>

#include "ebus.h"
#include "pboard.h"
#include "lsic.h"
#include "rtc.h"
#include "xr.h"

struct timeval RTCCurrentTime;

uint32_t RTCIntervalMS = 0;
uint32_t RTCIntervalCounter = 0;

uint32_t RTCPortA;

void RTCInterval(uint32_t dt) {
	// Currently the thread for CPU 0 does the RTC intervals, so we don't need
	// any synchronization until we need to do an interrupt.

	gettimeofday(&RTCCurrentTime, 0);

	if (RTCIntervalMS) {
		RTCIntervalCounter += dt;

		if (RTCIntervalCounter >= RTCIntervalMS) {
			LockIoMutex();
			LsicInterrupt(0x2);
			UnlockIoMutex();

			RTCIntervalCounter -= RTCIntervalMS;
		}
	}
}

int RTCWriteCMD(uint32_t port, uint32_t type, uint32_t value) {
	switch(value) {
		case 1:
			// set interval
			RTCIntervalMS = RTCPortA;
			RTCIntervalCounter = 0;

			return EBUSSUCCESS;

		case 2:
			// get epoch time
			RTCPortA = RTCCurrentTime.tv_sec + NVRAM_GET_RTCOFFSET();

			return EBUSSUCCESS;

		case 3:
			// get epoch ms
			RTCPortA = RTCCurrentTime.tv_usec/1000;

			return EBUSSUCCESS;

		case 4:
			// set epoch time
			NVRAM_SET_RTCOFFSET((RTCPortA - RTCCurrentTime.tv_sec));

			return EBUSSUCCESS;

		case 5:
			// set epoch ms
			return EBUSSUCCESS;
	}

	return EBUSERROR;
}

int RTCReadCMD(uint32_t port, uint32_t type, uint32_t *value) {
	*value = 0;

	return EBUSSUCCESS;
}

int RTCWritePortA(uint32_t port, uint32_t type, uint32_t value) {
	RTCPortA = value;

	return EBUSSUCCESS;
}

int RTCReadPortA(uint32_t port, uint32_t type, uint32_t *value) {
	// Decrement the progress count on the current processor. Note that
	// the firmware uses this to idle w/o consuming too much host cpu.

	XrIoMutexProcessor->Progress--;

	*value = RTCPortA;

	return EBUSSUCCESS;
}

void RTCInit() {
	gettimeofday(&RTCCurrentTime, 0);

	CitronPorts[0x20].Present = 1;
	CitronPorts[0x20].ReadPort = RTCReadCMD;
	CitronPorts[0x20].WritePort = RTCWriteCMD;

	CitronPorts[0x21].Present = 1;
	CitronPorts[0x21].ReadPort = RTCReadPortA;
	CitronPorts[0x21].WritePort = RTCWritePortA;
}

void RTCReset() {
	RTCIntervalMS = 0;
	RTCIntervalCounter = 0;
	RTCPortA = 0;
}