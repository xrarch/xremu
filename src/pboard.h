int PBoardInit();

#define PBOARDREGISTERS 32

typedef int (*CitronWriteF)(uint32_t port, uint32_t type, uint32_t value);

typedef int (*CitronReadF)(uint32_t port, uint32_t type, uint32_t *value);

struct CitronPort {
	int Present;
	CitronWriteF WritePort;
	CitronReadF ReadPort;
};

#define RESETMAGIC 0xAABBCCDD

#define CITRONPORTS 256

struct CitronPort CitronPorts[CITRONPORTS];

#define NVRAMSIZE (64 * 1024)

uint32_t NVRAM[NVRAMSIZE/4];

#define ROMSIZE (128 * 1024)

uint32_t BootROM[ROMSIZE/4];

void NVRAMSave();

bool ROMLoadFile(char *romname);

bool NVRAMLoadFile(char *nvramname);