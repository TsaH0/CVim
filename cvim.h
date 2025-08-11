#define GNU_SOURCE
#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <termios.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long int u64;
#define $1 (u8*)
#define $2 (u16)
#define $4 (u32)
#define $8 (u64)
#define $c (char*)
#define $i (int)

u8* todotted(in_addr_t);
void enableRawMode();
void disableRawMode();
void zero(u8*,u16);
int main(int, char **);
