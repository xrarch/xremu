int LSICWrite(int reg, uint32_t value);

int LSICRead(int reg, uint32_t *value);

void LSICReset();

bool LSICInterruptPending;

void LSICInterrupt(int intsrc);