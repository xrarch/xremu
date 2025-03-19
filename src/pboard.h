int PBoardInit();

#define PBOARDREGISTERS 32

typedef int (*CitronWriteF)(uint32_t port, uint32_t length, uint32_t value, void *proc);

typedef int (*CitronReadF)(uint32_t port, uint32_t length, uint32_t *value, void *proc);

struct CitronPort {
	int Present;
	CitronWriteF WritePort;
	CitronReadF ReadPort;
};

#define RESETMAGIC 0xAABBCCDD

#define CITRONPORTS 256

#define NVRAMSIZE (4 * 1024)
#define ROMSIZE (128 * 1024)

#define NVRAMRTCOFFSET 124

extern uint8_t NVRAM[NVRAMSIZE];

extern bool NVRAMDirty;

#define NVRAM_GET_RTCOFFSET() NVRAM[NVRAMRTCOFFSET] +         \
                              (NVRAM[NVRAMRTCOFFSET+1] << 8) +  \
                              (NVRAM[NVRAMRTCOFFSET+2] << 16) + \
                              (NVRAM[NVRAMRTCOFFSET+3] << 24)

#define NVRAM_SET_RTCOFFSET(off) NVRAM[NVRAMRTCOFFSET] = off & 0xFF;         \
                                 NVRAM[NVRAMRTCOFFSET+1] = off >> 8 & 0xFF;  \
                                 NVRAM[NVRAMRTCOFFSET+2] = off >> 16 & 0xFF; \
                                 NVRAM[NVRAMRTCOFFSET+3] = off >> 24 & 0xFF; \
                                 NVRAMDirty = true;

extern struct CitronPort CitronPorts[CITRONPORTS];

void NVRAMSave();

bool ROMLoadFile(char *romname);

bool NVRAMLoadFile(char *nvramname);

