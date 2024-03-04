#include "myfs.h"

int min(int a,int b){return a<b?a:b;}
const int DATABLOCKSTART = 2+SUPERBLOCKSIZE+INODESIZE*INODELISTLEN;

void MYfs_init(FILE *fp){
    fseek(fp,0,SEEK_SET);
    for(int i=0;i<2;i++)fputc(0,fp); // boot block
    for(int i=0;i<SUPERBLOCKSIZE;i++)fputc(0,fp); // super block
    for(int i=0;i<INODESIZE*INODELISTLEN;i++)fputc(0,fp); // inode lists
    for(int i=0;i<DATABLOCKSIZE*DATABLOCKSLEN;i++)fputc(0,fp); // data blocks


    // Make root dir
    MYfs_setSuperBlockVal(fp,0,true,true);
    inode *rootinode=inode_new();
    rootinode->index=0;
    rootinode->fileType=FT_DIR;
    rootinode->createdTime=time(NULL);
    *(rootinode->directBlocks+0)=0;
    MYfs_setSuperBlockVal(fp,0,true,false);

    // set filename on 0th datablk (8byte maybe?)
    byte *data=(byte*)calloc(DATABLOCKSIZE,sizeof(byte));
    strcpy((char*)data,"");
    // register it to newObject's directblock
    memcpy(data+8,&(rootinode->index),sizeof(ushort));
    memcpy(data+10,&(rootinode->index),sizeof(ushort));
    MYfs_setDataBlock(fp,0,data);
    free(data);

    MYfs_setInode(fp,rootinode);
    fflush(fp);
}

bool MYfs_getSuperBlockVal(FILE *fp,uint idx){
    fseek(fp,2+(idx/8),SEEK_SET);
    return (fgetc(fp)&(1<<(7-(idx%8))))>0;
}

int MYfs_getUnusedSuperBlockIdx(FILE *fp,bool isInode){
    int i=isInode?0:INODELISTLEN/8;
    int dst=isInode?INODELISTLEN/8:SUPERBLOCKSIZE;

    fseek(fp,2+i,SEEK_SET);
    for(;i<dst;i++){
        byte c;
        if((c = fgetc(fp)) != 0xff)
            for(int j=0;j<8;j++) 
                if((c&((0b10000000)>>j))==0) 
                    return (i*8+j)-(isInode?0:INODELISTLEN);
    }
}

void MYfs_setSuperBlockVal(FILE *fp,uint idx,bool val,bool isInode){
    int realidx=2+(idx/8)+(isInode?0:INODELISTLEN/8);

    fseek(fp,realidx,SEEK_SET);
    byte tmp = fgetc(fp);
    if(val)tmp|=(1<<(7-(idx%8)));
    else tmp^=tmp&(1<<(7-(idx%8)));
    fseek(fp,realidx,SEEK_SET);
    fputc(tmp,fp);
    fflush(fp);
}
 
void MYfs_getInode(FILE *fp,uint idx,inode *ret){
    fseek(fp,2+SUPERBLOCKSIZE+INODESIZE*idx,SEEK_SET);
    ret->index=idx;
    fread(&(ret->fileType),sizeof(ret->fileType),1,fp);
    fread(&(ret->createdTime),sizeof(ret->createdTime),1,fp);
    fread(&(ret->fileSize),sizeof(ret->fileSize),1,fp);
    fread(ret->directBlocks,sizeof(ret->indirectBlock),8,fp);
    fread(&(ret->indirectBlock),sizeof(ret->indirectBlock),1,fp);
}

void MYfs_getInodeByFileName(FILE *fp,char *filename,inode *parentdir,inode *ret){
    ushort *dirz=(ushort*)malloc(parentdir->fileSize);
    MYfs_readObject(fp,parentdir,(byte*)dirz);
    byte *data=(byte*)calloc(8,sizeof(byte));
    for(int i=0;i<parentdir->fileSize/sizeof(ushort);i++){
        MYfs_getInode(fp,*(dirz+i),ret);
        MYfs_getFileName(fp,ret,data);
        if(strcmp((char*)data,filename)==0){
            return;
        }
    }
    // Should not reach here
    ret->index=0xffff;
}

void MYfs_setInode(FILE *fp,inode *inode){
    fseek(fp,2+SUPERBLOCKSIZE+INODESIZE*inode->index,SEEK_SET);
    fwrite(&(inode->fileType),sizeof(inode->fileType),1,fp);
    fwrite(&(inode->createdTime),sizeof(inode->createdTime),1,fp);
    fwrite(&(inode->fileSize),sizeof(inode->fileSize),1,fp);
    fwrite(inode->directBlocks,sizeof(inode->indirectBlock),8,fp);
    fwrite(&(inode->indirectBlock),sizeof(inode->indirectBlock),1,fp);   
    fflush(fp); 
}

void MYfs_getDataBlock(FILE *fp, uint idx, byte* ret){
    fseek(fp,DATABLOCKSTART+DATABLOCKSIZE*idx,SEEK_SET);
    fread(ret,sizeof(byte),DATABLOCKSIZE,fp);
}

void MYfs_setDataBlock(FILE *fp,uint idx, byte* data){
    fseek(fp,DATABLOCKSTART+DATABLOCKSIZE*idx,SEEK_SET);
    fwrite(data,sizeof(byte),DATABLOCKSIZE,fp);
    fflush(fp);
}

void MYfs_createObject(
    FILE *fp,char *dirname,inode *parentdir,inode *newdir,bool fileType){
    // create new dir's inode
    ushort unusedinodeidx = MYfs_getUnusedSuperBlockIdx(fp,true);
    ushort unuseddataidx = MYfs_getUnusedSuperBlockIdx(fp,false);
    newdir->index=unusedinodeidx;
    newdir->fileType=fileType;
    *(newdir->directBlocks+0)=unuseddataidx;
    newdir->createdTime=time(0);

    MYfs_setSuperBlockVal(fp,newdir->index,true,true);
    MYfs_setInode(fp,newdir);
    
    // set filename on 0th datablk (8byte maybe?)
    byte *data=(byte *)calloc(DATABLOCKSIZE,sizeof(byte));
    strcpy((char *)data,dirname);
    // register it to newObject's directblock
    memcpy(data+8,&(newdir->index),sizeof(ushort));
    memcpy(data+10,&(parentdir->index),sizeof(ushort));

    MYfs_setSuperBlockVal(fp,unuseddataidx,true,false);
    MYfs_setDataBlock(fp,unuseddataidx,data);
    free(data);

    // register it to parentDir's directblock
    MYfs_appendObject(fp,parentdir,(byte*)&(newdir->index),sizeof(ushort));
}

void MYfs_moveObject(
    FILE *fp, inode *file, inode *oldparentdir, inode *newparentdir){
    if(oldparentdir->index == newparentdir->index) return;
    ushort *dirz=(ushort *)malloc(oldparentdir->fileSize);
    int newdirzsize=oldparentdir->fileSize-sizeof(ushort);
    ushort *newdirz=(ushort *)malloc(newdirzsize);
    MYfs_readObject(fp,oldparentdir,(byte*)dirz);
    for(int i=0,j=0;i<oldparentdir->fileSize/sizeof(ushort);i++,j++){
        if(*(dirz+i)==file->index) j--;
        else *(newdirz+j)=*(dirz+i);
    }
    MYfs_writeObject(fp,oldparentdir,(byte*)newdirz,newdirzsize);
    MYfs_appendObject(fp,newparentdir,(byte*)&(file->index),sizeof(ushort));
    free(dirz);
    free(newdirz);
}

void MYfs_renameObject(FILE *fp, inode *file, char *newname){
    int firstblockidx = *(file->directBlocks+0);
    fseek(fp,DATABLOCKSTART+DATABLOCKSIZE*firstblockidx,SEEK_SET);
    
    byte *data=(byte *)calloc(8,sizeof(byte));
    memset(data,0,8);
    strcpy((char *)data,newname);
    fwrite(data,sizeof(byte),8,fp);
    free(data);
    fflush(fp);
}

void MYfs_removeObject(FILE *fp, inode *file, inode *parentdir){
    ushort *dirz=(ushort *)malloc(parentdir->fileSize);
    int newdirzsize=parentdir->fileSize-sizeof(ushort);
    ushort *newdirz=(ushort *)malloc(newdirzsize);
    MYfs_readObject(fp,parentdir,(byte*)dirz);
    for(int i=0,j=0;i<parentdir->fileSize/sizeof(ushort);i++,j++){
        if(*(dirz+i)==file->index) j--; 
        else *(newdirz+j)=*(dirz+i);
    }
    MYfs_writeObject(fp,parentdir,(byte*)newdirz,newdirzsize);
    for(int i=0;i<8;i++){
        if(*(file->directBlocks+i)!=0xffff){
            MYfs_setSuperBlockVal(fp,*(file->directBlocks+i),false,false);
        }
    }

    int beforeblk=((file->fileSize+12 - 1)/DATABLOCKSIZE);//+1;

    if(file->indirectBlock!=0xffff){
        ushort *dirz=(ushort *)malloc(DATABLOCKSIZE);
        MYfs_getDataBlock(fp,file->indirectBlock,(byte*)dirz);
        for(int i=0;i<beforeblk-8;i++){
            MYfs_setSuperBlockVal(fp,*(dirz+i),false,false);
        }
        free(dirz);
    }
    MYfs_setSuperBlockVal(fp,file->index,false,true);
    free(dirz);
    free(newdirz);
}

void MYfs_writeObject(
    FILE *fp, inode *inode, byte *content, size_t contentlen){

    int beforeblk=((inode->fileSize+12 - 1)/DATABLOCKSIZE);//+1;
    int afterblk=((contentlen+12 - 1)/DATABLOCKSIZE);//+1;

    //dealloc blocks
    while(beforeblk>afterblk){
        if(beforeblk<8){
            MYfs_setSuperBlockVal(fp,*(inode->directBlocks+beforeblk),false,false);
            *(inode->directBlocks+beforeblk)=0xffff;
        }else{
            ushort *dirz=(ushort *)malloc(DATABLOCKSIZE);
            MYfs_getDataBlock(fp,inode->indirectBlock,(byte*)dirz);
            MYfs_setSuperBlockVal(fp,*(dirz+beforeblk-8),false,false);
            *(dirz+beforeblk-8)=0xffff;
            MYfs_setDataBlock(fp,inode->indirectBlock,(byte*)dirz);
            free(dirz);
            if(beforeblk==8){
                MYfs_setSuperBlockVal(fp,inode->indirectBlock,false,false);
                inode->indirectBlock=0xffff;
            }
        }
        beforeblk--; 
    }

    //alloc blocks
    while(beforeblk<afterblk){
        if(beforeblk<7){
            ushort iiddxx = MYfs_getUnusedSuperBlockIdx(fp,false);
            MYfs_setSuperBlockVal(fp,iiddxx,true,false);
            *(inode->directBlocks+beforeblk+1)=iiddxx;
        }else{
            if(beforeblk==7){
                ushort iiddxx = MYfs_getUnusedSuperBlockIdx(fp,false);
                MYfs_setSuperBlockVal(fp,iiddxx,true,false);
                inode->indirectBlock=iiddxx;
            }
            
            ushort iiddxx = MYfs_getUnusedSuperBlockIdx(fp,false);
            MYfs_setSuperBlockVal(fp,iiddxx,true,false);

            ushort *dirz=(ushort *)malloc(DATABLOCKSIZE);
            MYfs_getDataBlock(fp,inode->indirectBlock,(byte*)dirz);
            *(dirz+beforeblk-8+1)=iiddxx;
            MYfs_setDataBlock(fp,inode->indirectBlock,(byte*)dirz);
            free(dirz);
        }
        beforeblk++;
    }

    int datacursor=0;
    
    byte *zeros = (byte *)malloc(DATABLOCKSIZE);

    for(int i=0;i<=afterblk;i++){
        memset(zeros,0,DATABLOCKSIZE);
        int truesize=min(contentlen-datacursor,DATABLOCKSIZE);
        if(i==0){
            truesize=min(contentlen-datacursor,DATABLOCKSIZE-12);
            MYfs_getDataBlock(fp,*inode->directBlocks,zeros);
            memcpy(zeros+12,content,truesize);
            datacursor+=truesize;
            MYfs_setDataBlock(fp,*inode->directBlocks,zeros);
        }else if(i<8){
            memcpy(zeros,content+datacursor,truesize);
            datacursor+=truesize;
            MYfs_setDataBlock(fp,*(inode->directBlocks+i),zeros);
        }else if(i>=8){
            ushort *dirz=(ushort *)malloc(DATABLOCKSIZE);
            MYfs_getDataBlock(fp,inode->indirectBlock,(byte*)dirz);
            memcpy(zeros,content+datacursor,truesize);
            datacursor+=truesize;
            MYfs_setDataBlock(fp,*(dirz+i-8),zeros);
            free(dirz);
        }
    }
    inode->fileSize=contentlen;
    MYfs_setInode(fp,inode);
}

void MYfs_appendObject(
    FILE *fp, inode *inode, byte *content, size_t contentlen){

    int beforeblk=((inode->fileSize+12 - 1)/DATABLOCKSIZE);//+1;
    int afterblk=((inode->fileSize+contentlen+12 - 1)/DATABLOCKSIZE);//+1;

    //alloc blocks
    for(int bfbtmp=beforeblk;bfbtmp<afterblk;bfbtmp++){
        if(bfbtmp<7){
            ushort iiddxx = MYfs_getUnusedSuperBlockIdx(fp,false);
            MYfs_setSuperBlockVal(fp,iiddxx,true,false);
            *(inode->directBlocks+bfbtmp+1)=iiddxx;
        }else{
            if(bfbtmp==7){
                ushort iiddxx = MYfs_getUnusedSuperBlockIdx(fp,false);
                MYfs_setSuperBlockVal(fp,iiddxx,true,false);
                inode->indirectBlock=iiddxx;
            }
            
            ushort iiddxx = MYfs_getUnusedSuperBlockIdx(fp,false);
            MYfs_setSuperBlockVal(fp,iiddxx,true,false);

            ushort *dirz=(ushort *)malloc(DATABLOCKSIZE);
            MYfs_getDataBlock(fp,inode->indirectBlock,(byte*)dirz);
            *(dirz+bfbtmp-8+1)=iiddxx;
            MYfs_setDataBlock(fp,inode->indirectBlock,(byte*)dirz);
            free(dirz);
        }
    }

    byte *zeros = (byte *)malloc(DATABLOCKSIZE);
    int datacursor=0;
    int lastblkidx=(inode->fileSize+12)%DATABLOCKSIZE;

    for(int i=beforeblk;i<=afterblk;i++){
        memset(zeros,0,DATABLOCKSIZE);
        int truesize=min(contentlen-datacursor,DATABLOCKSIZE);
        if(i==beforeblk){
            truesize=min(contentlen-datacursor,DATABLOCKSIZE-lastblkidx);
            if(i==0)truesize=min(truesize,DATABLOCKSIZE-12);
        }else{
            lastblkidx=0;
        }
        if(i==0){
            MYfs_getDataBlock(fp,*inode->directBlocks,zeros);
            memcpy(zeros+lastblkidx,content,truesize);
            MYfs_setDataBlock(fp,*inode->directBlocks,zeros);
        }else if(i<8){
            MYfs_getDataBlock(fp,*(inode->directBlocks+i),zeros);
            memcpy(zeros+lastblkidx,content+datacursor,truesize);
            MYfs_setDataBlock(fp,*(inode->directBlocks+i),zeros);
        }else if(i>=8){
            ushort *dirz=(ushort *)malloc(DATABLOCKSIZE);
            MYfs_getDataBlock(fp,inode->indirectBlock,(byte*)dirz);
            MYfs_getDataBlock(fp,*(dirz+i-8),zeros);
            memcpy(zeros+lastblkidx,content+datacursor,truesize);
            MYfs_setDataBlock(fp,*(dirz+i-8),zeros);
            free(dirz);
        }
        datacursor+=truesize;
    }


    inode->fileSize+=contentlen;
    MYfs_setInode(fp,inode);
}

void MYfs_readObject(FILE *fp,inode *inode,byte *out){
    int beforeblk=((inode->fileSize+12 - 1)/DATABLOCKSIZE);

    int datacursor=0;
    
    byte *zeros = (byte *)malloc(DATABLOCKSIZE);

    for(int i=0;i<=beforeblk;i++){
        memset(zeros,0,DATABLOCKSIZE);
        int truesize=min(inode->fileSize-datacursor,DATABLOCKSIZE);
        if(i==0){
            truesize=min(inode->fileSize-datacursor,DATABLOCKSIZE-12);
            MYfs_getDataBlock(fp,*inode->directBlocks,zeros);
            memcpy(out,zeros+12,truesize);
        }else if(i<8){
            MYfs_getDataBlock(fp,*(inode->directBlocks+i),zeros);
            memcpy(out+datacursor,zeros,truesize);
        }else if(i>=8){
            ushort *dirz=(ushort *)malloc(DATABLOCKSIZE);
            MYfs_getDataBlock(fp,inode->indirectBlock,(byte*)dirz);
            MYfs_getDataBlock(fp,*(dirz+i-8),zeros);
            memcpy(out+datacursor,zeros,truesize);
            free(dirz);
        }
        datacursor+=truesize;
    }
    free(zeros);
}

void MYfs_getFileName(FILE *fp,inode *inode,byte *out){
    int firstblockidx = *(inode->directBlocks+0);
    fseek(fp,DATABLOCKSTART+DATABLOCKSIZE*firstblockidx,SEEK_SET);
    memset(out,0,8);
    fread(out,sizeof(byte),8,fp);
}

ushort MYfs_getParentInodeIdx(FILE *fp,inode *file){
    int firstblockidx = *(file->directBlocks+0);
    fseek(fp,DATABLOCKSTART+DATABLOCKSIZE*firstblockidx+10,SEEK_SET);
    ushort index;
    fread(&index,sizeof(ushort),1,fp);
    return index;
}