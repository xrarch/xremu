struct AmtsuDevice;

typedef int (*AmtsuActionF)(struct AmtsuDevice *dev, uint32_t value);
typedef int (*AmtsuBusyF)(struct AmtsuDevice *dev, uint32_t *value);

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
};

#define AMTSUDEVICES 16

void AmtsuReset();

void AmtsuInit();

extern struct AmtsuDevice AmtsuDevices[AMTSUDEVICES];