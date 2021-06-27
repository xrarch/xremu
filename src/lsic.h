int LSICWrite(int reg, uint32_t value);

int LSICRead(int reg, uint32_t *value);

void LSICReset();

extern bool LSICInterruptPending;

void LSICInterrupt(int intsrc);