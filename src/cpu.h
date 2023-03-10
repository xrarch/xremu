void CPUReset();

uint32_t CPUDoCycles(uint32_t cycles, uint32_t dt);

#define CPUHZDEFAULT 25000000

extern int CPUProgress;

extern bool CPUSimulateCaches;
extern bool CPUSimulateCacheStalls;
extern bool CPUPrintCache;