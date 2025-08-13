#ifndef XR_AMTSU_H
#define XR_AMTSU_H

#include <stdint.h>

struct AmtsuDevice;

typedef int (*AmtsuActionF)(struct AmtsuDevice *dev, uint32_t value, void *proc);
typedef int (*AmtsuBusyF)(struct AmtsuDevice *dev, uint32_t *value, void *proc);

typedef void (*AmtsuResetF)(struct AmtsuDevice *dev);

struct AmtsuDevice {
	int Present;
	int InterruptNumber;
	uint32_t MID;
	uint32_t PortAValue;
	uint32_t PortBValue;
	AmtsuActionF Action;
	AmtsuBusyF IsBusy;
	AmtsuResetF Reset;
	void *Mutex;
};

#define AMTSUDEVICES 16

#define AMTSU_CONTROLLER 0
#define AMTSU_KEYBOARD 1
#define AMTSU_MOUSE 2

void AmtsuReset();

void AmtsuInit();

extern struct AmtsuDevice AmtsuDevices[AMTSUDEVICES];

#endif // XR_AMTSU_H