#define DKSDISKS 8

void DKSInit();

void DKSReset();

extern uint8_t DKSBlockBuffer[512*DKSDISKS];

extern bool DKSAsynchronous;
extern bool DKSPrint;

int DKSAttachImage(char *path);

void DKSInterval(uint32_t dt);