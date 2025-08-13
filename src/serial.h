#ifndef XR_SERIAL_H
#define XR_SERIAL_H

#include <stdint.h>
#include <stdbool.h>

bool SerialSetRXFile(char *filename);
bool SerialSetTXFile(char *filename);

int SerialInit(int num);

void SerialReset();
void SerialInterval(uint32_t dt);

extern bool SerialAsynchronous;

#endif // XR_SERIAL_H