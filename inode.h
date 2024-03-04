
#include "consts.h"

#ifndef INODE_H
#define INODE_H

typedef struct inode
{
    ushort index; // 0xFFFF <- null
    bool fileType; // 0-file 1-dir
    time_t createdTime;
    ll fileSize;
    ushort *directBlocks;//[8]
    ushort indirectBlock;//
} inode;

inode* inode_new();

void inode_delete(inode *this);

#endif
