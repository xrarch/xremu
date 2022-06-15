int PBoardInit();

#define PBOARDREGISTERS 32

typedef int (*CitronWriteF)(uint32_t port, uint32_t length, uint32_t value);

typedef int (*CitronReadF)(uint32_t port, uint32_t length, uint32_t *value);

struct CitronPort {
	int Present;
	CitronWriteF WritePort;
	CitronReadF ReadPort;
};

#define RESETMAGIC 0xAABBCCDD

#define CITRONPORTS 256

extern struct CitronPort CitronPorts[CITRONPORTS];

void NVRAMSave();

bool ROMLoadFile(char *romname);

bool NVRAMLoadFile(char *nvramname);