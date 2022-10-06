bool SerialSetRXFile(char *filename);
bool SerialSetTXFile(char *filename);

int SerialInit(int num);

void SerialReset();
void SerialInterval(uint32_t dt);

extern bool SerialAsynchronous;