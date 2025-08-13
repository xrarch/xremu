#ifndef XR_RTC_H
#define	XR_RTC_H

#include <stdint.h>


void RTCReset();

void RTCInit();

void RTCUpdateRealTime();

extern uint32_t RTCIntervalMS;

#endif