#define DKSDISKS 8

void DKSInit();

void DKSReset();

extern uint32_t DKSBlockBuffer[128];

extern bool DKSAsynchronous;

int DKSAttachImage(char *path);

void DKSOperation(uint32_t dt);