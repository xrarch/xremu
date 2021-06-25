#define DKSDISKS 8

void DKSInit();

void DKSReset();

uint32_t DKSBlockBuffer[1024];

int DKSAttachImage(char *path);