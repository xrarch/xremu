#define DKSDISKS 8

void DKSInit();

void DKSReset();

extern uint8_t DKSBlockBuffer[512];

extern bool DKSAsynchronous;

int DKSAttachImage(char *path);

void DKSOperation(uint32_t dt);

// fake disk geometry for simple seek time simulation

#define LBAPERTRACK 63
#define TRACKPERCYL 4
#define CYLPERDISK  1000

#define RPM 3600
#define RPS (RPM/60)
#define ROTATIONTIMEMS (1000/RPS)

#define FULLSEEKTIMEMS 200
#define SETTLETIMEMS   3

#define BLOCKSPERMS    (LBAPERTRACK/ROTATIONTIMEMS)

#define LBA_TO_BLOCK(lba)    ((lba)%LBAPERTRACK)
#define LBA_TO_TRACK(lba)    ((lba)/LBAPERTRACK)
#define LBA_TO_CYLINDER(lba) (LBA_TO_TRACK(lba)/TRACKPERCYL)