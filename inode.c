#include "inode.h"

inode* inode_new(){
    inode *ptr = (inode *)malloc(sizeof(inode));
    ptr->directBlocks=calloc(8,sizeof(ptr->directBlocks));
    for(int i=0;i<8;i++)*(ptr->directBlocks+i)=0xFFFF;
    ptr->fileSize=0;
    ptr->indirectBlock=0xFFFF;
    ptr->index=0xFFFF;
    return ptr;
}

void inode_delete(inode *this){
    free(this->directBlocks);
    free(this);
}