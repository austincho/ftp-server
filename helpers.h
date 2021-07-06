#ifndef _HELPERSH__

#define _HELPERSH__

int cmdUser(char*, int);

int cmdInvalid(int);

int cmdQuit(int);

int cmdInitialize(int);

int cmdType(char*, int);

int cmdStru(char*, int);

int cmdMode(char*, int);

int cmdCWD(char*, int);

int cmdCDUP(char*, int);

int cmdNlst(char*, int);

int cmdPasv(char*, int);

int cmdRetr(char*, int);

void* pasvhelper(void *args);
_Bool startWith(const char*, const char*);

#endif
