#define DKSDISKS 8

void DKSInit();

void DKSReset();

extern uint32_t DKSBlockBuffer[1024];

extern bool DKSAsynchronous;

int DKSAttachImage(char *path);

void DKSOperation(uint32_t dt);