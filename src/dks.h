#define DKSDISKS 8

void DKSInit();

void DKSReset();

extern bool DKSAsynchronous;
extern bool DKSPrint;

int DKSAttachImage(char *path);

void DKSInterval(uint32_t dt);