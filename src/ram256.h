#define RAMSLOTSIZE (32 * 1024 * 1024)
#define RAMSLOTCOUNT 8
#define RAMMAXIMUM (RAMSLOTSIZE * RAMSLOTCOUNT)

extern int RAMInit(uint32_t memsize);
extern void RAMDump();