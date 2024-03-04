#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#ifndef CONSTS_H
#define CONSTS_H

typedef long long ll;
typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;

static const int CMDBUFSIZE = 256;
static const int DATABLOCKSIZE = 256;
static const int DATABLOCKSLEN = 256;
static const int INODELISTLEN = 128;
static const int MAXDIRDEPTH = 128;

static const bool FT_FILE=0;
static const bool FT_DIR=1;

static const size_t SUPERBLOCKSIZE=DATABLOCKSLEN/8+INODELISTLEN/8;
static const size_t INODESIZE=sizeof(bool)+sizeof(time_t)+sizeof(ll)+(sizeof(ushort)*8)+sizeof(ushort);

#endif 