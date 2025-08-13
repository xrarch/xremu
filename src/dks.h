#ifndef XR_DKS_H
#define XR_DKS_H

#include <stdbool.h>
#include <stdint.h>

#define DKSDISKS 8

void DKSInit();

void DKSReset();

extern bool DKSAsynchronous;
extern bool DKSPrint;

int DKSAttachImage(char *path);

void DKSInterval(uint32_t dt);

#endif // XR_DKS_H