#include <SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cpu.h"

#include "ebus.h"
#include "pboard.h"
#include "lsic.h"
#include "rtc.h"

struct timeval RTCCurrentTime;

uint32_t RTCCurrentTimeSec = 0;
uint32_t RTCCurrentTimeMs = 0;

uint32_t RTCIntervalMS = 0;
uint32_t RTCIntervalCounter = 0;

uint32_t RTCPortA;

bool RTCModified = false;

void RTCInterval(uint32_t dt) {
	if (!RTCModified) {
		gettimeofday(&RTCCurrentTime, 0);
	} else {
		RTCCurrentTimeMs += dt;
		if (RTCCurrentTimeMs >= 1000) {
			RTCCurrentTimeMs -= 1000;
			RTCCurrentTimeSec += 1;
		}
	}

	RTCIntervalCounter += dt;

	if (RTCIntervalCounter >= RTCIntervalMS) {
		LSICInterrupt(0x1);

		RTCIntervalCounter = 0;
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
			if (RTCModified)
				RTCPortA = RTCCurrentTimeSec;
			else
				RTCPortA = RTCCurrentTime.tv_sec;

			return EBUSSUCCESS;

		case 3:
			// get epoch ms
			if (RTCModified)
				RTCPortA = RTCCurrentTimeMs;
			else
				RTCPortA = RTCCurrentTime.tv_usec/1000;

			CPUProgress--;

			return EBUSSUCCESS;

		case 4:
			// set epoch time
			RTCCurrentTimeSec = RTCPortA;
			RTCModified = true;

			return EBUSSUCCESS;

		case 5:
			// set epoch ms
			RTCCurrentTimeMs = RTCPortA;
			RTCModified = true;

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