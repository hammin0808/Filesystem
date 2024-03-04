#include "consts.h"
#include "stack.h"
#include "inode.h"
#include "myfs.h"
#include <assert.h>

// GLOBAL STATE
FILE *rawfs=NULL;
Stack *cursorInodeStack;

void mymkfs(bool forced){
    bool agreed=false;
    if(!forced){
        printf("파일시스템이 있습니다. 다시 만들겠습니까?. (y/n)\n");
        char c;scanf("\n%c",&c);
        if(c<'a')c+='a'-'A';
        agreed=(c=='y');
    }
    if(agreed||forced){
        MYfs_init(rawfs);
        if(!forced){
            printf("파일시스템을 다시 만들었습니다.\n");
        }
    }    
}

void pathResolving(char *path,Stack *stk){
    if(*path!='/')
        for(int i=0;i<cursorInodeStack->cur;i++)
            Stack_push(stk,*(cursorInodeStack->stk+i));
    else
        Stack_push(stk,0);

    char *ptr = strtok(path,"/");
    while(ptr!=NULL){
        if(*(ptr+0)=='.'&&*(ptr+1)=='.'&&*(ptr+2)==0){
            Stack_pop(stk);
        }else if(*(ptr+0)=='.'&&*(ptr+1)=='\0'){
            //Do nothing
        }else{
            inode *tmpino = inode_new();
            inode *tmpino2 = inode_new();
            MYfs_getInode(rawfs,Stack_top(stk),tmpino);
            MYfs_getInodeByFileName(rawfs,ptr,tmpino,tmpino2);
            Stack_push(stk,tmpino2->index);
            inode_delete(tmpino);
            inode_delete(tmpino2);
        }
        ptr=strtok(NULL,"/");
    }
} 

void myls(char *path){
    Stack *stk = Stack_new();
    pathResolving(path,stk);
    inode *tmp=inode_new();
    int tmpint = Stack_top(stk);
    MYfs_getInode(rawfs,tmpint,tmp);
    if(tmp->fileType==FT_DIR){
        ushort *objz=malloc(tmp->fileSize);
        MYfs_readObject(rawfs,tmp,(byte*)objz);
        for(int i=-2;i<(int)(tmp->fileSize/sizeof(ushort));i++){
            inode *looptmp=inode_new();
            if(i==-2){
                MYfs_getInode(rawfs,tmp->index,looptmp);                
            }else if(i==-1){
                MYfs_getInode(rawfs,MYfs_getParentInodeIdx(rawfs,tmp),looptmp);
            }else{
                MYfs_getInode(rawfs,*(objz+i),looptmp);
            }
            char *buff=malloc(20);
            strftime(buff, 20, "%Y/%m/%d %H:%M:%S", localtime(&(looptmp->createdTime)));
            char *filename=malloc(10);
            memset(filename,0,10);
            if(i==-2){
                strcpy(filename,".");
            }else if(i==-1){
                strcpy(filename,"..");
            }else{
                MYfs_getFileName(rawfs,looptmp,filename);
            }
            printf("%s  %-10s %5d %6lld byte  %s\n",
                buff,
                looptmp->fileType==FT_DIR?"directory":"file",
                looptmp->index+1, // as it should start from 1
                looptmp->fileSize,
                filename);
            inode_delete(looptmp);
            free(buff);
            free(filename);
        }
    }else{
        char *buff=malloc(20);
        strftime(buff, 20, "%Y/%m/%d %H:%M:%S", localtime(&(tmp->createdTime)));
        char *filename=malloc(10);
        memset(filename,0,10);
        MYfs_getFileName(rawfs,tmp,filename);
        printf("%s  %-10s %5d %6lld byte  %s\n",
        buff,
        tmp->fileType==FT_DIR?"directory":"file",
        tmp->index,
        tmp->fileSize,
        filename);
        free(buff);
        free(filename);
    }
}

void mycat(char *filename){ 
    Stack *dir = Stack_new();
    pathResolving(filename,dir);
    uint now_dir_index = Stack_top(dir);
    if(now_dir_index==0xffff){
        printf("mycat: %s: no such file\n",filename);
        return;
    }
    inode* nowfile=inode_new();
    MYfs_getInode(rawfs, now_dir_index, nowfile);//현재 이동하려는 파일의 부모 dir
    char* str = malloc(nowfile->fileSize+1);
    *(str+nowfile->fileSize)='\0';
    MYfs_readObject(rawfs, nowfile, str);
    
    for(int i=0;i<nowfile->fileSize;i++){
        putchar(*(str+i));
    }

    free(str);
    inode_delete(nowfile);
    Stack_delete(dir);
}
 
void myshowfile(char *start, char *end, char *filename){
    int start_i=atoi(start);
    int end_i=atoi(end);
    Stack *stk = Stack_new();
    pathResolving(filename,stk);
    
    inode* nowdir=inode_new();
    MYfs_getInode(rawfs, Stack_top(stk), nowdir);

    char* str = malloc(nowdir->fileSize+1);
    MYfs_readObject(rawfs,nowdir, str);
    *(str+nowdir->fileSize)=0;

    for(int i=start_i-1;i<=end_i;i++){
        putchar(*(str+i));
    }
    printf("\n");
    free(str);
}

void mypwd(){    
    inode *tmp=inode_new();
    for(int i=0;i<cursorInodeStack->cur;i++) {
        MYfs_getInode(rawfs,*(cursorInodeStack->stk + i),tmp);
        byte *name=malloc(8);
        memset(name,0,8);
        MYfs_getFileName(rawfs,tmp,name);
        if(i==0)printf("/");
        else if(i==1)printf("%s", name);
        else printf("/%s", name);
    }
    inode_delete(tmp);
}

void mycd(char *path) { //상대경로나 절대경로 디렉토리명이 cmd
    Stack *dir = Stack_new();
    if(*path==0)Stack_push(dir,0);
    else pathResolving(path,dir);
    if(Stack_top(dir)==0xffff){
        printf("mycd: %s: no such directory\n",path);
        return;
    }
    Stack_delete(cursorInodeStack);
    cursorInodeStack = dir; 
}

void mycp(char *srcpath, char *destpath){
    char *destname=strrchr(destpath,'/');
    if(destname==NULL){
        destname=destpath;
        destpath=".";
    }
    else {
        *destname='\0';
        destname+=1;
    }
    Stack *dirsrc  = Stack_new();
    Stack *dirdst  = Stack_new();
    pathResolving(srcpath,dirsrc);
    pathResolving(destpath,dirdst);
    inode *srcnode=inode_new();
    MYfs_getInode(rawfs, Stack_top(dirsrc), srcnode);//현재 이동하려는 파일
    inode *dstparent=inode_new();
    MYfs_getInode(rawfs, Stack_top(dirdst), dstparent);//현재 이동하려는 목적 부모 디렉토리

    inode *dstnode = inode_new();
    MYfs_createObject(rawfs,destname,dstparent,dstnode,FT_FILE);
    char* str = malloc(srcnode->fileSize);
    MYfs_readObject(rawfs, srcnode, str);
    MYfs_writeObject(rawfs, dstnode, str, (srcnode->fileSize));
}

void mycpto(char*myfile ,char *hostfile){ //myfile을 hostfile로 파일을 보내야함.
    Stack *dir = Stack_new();
    pathResolving(myfile,dir);

    inode *mynode = inode_new();
    MYfs_getInode(rawfs, Stack_top(dir), mynode);
    
    char* str = malloc(mynode->fileSize); //myfile의 내용이 들어갈 문자열.
    MYfs_readObject(rawfs, mynode, str);

    FILE* hostfp = fopen(hostfile, "w+b");
    fwrite(str, mynode->fileSize, 1, hostfp);
    fclose(hostfp);
    free(str);
}

void mycpfrom(char *hostfile,char *myfile) { 
    char *destname=strrchr(myfile,'/');
    if(destname==NULL){
        destname=myfile;
        myfile=".";
    }
    else {
        *destname='\0';
        destname+=1;
    }
    Stack *path=Stack_new();
    pathResolving(myfile,path);
    
    inode* newdir = inode_new();
    MYfs_getInode(rawfs, Stack_top(path), newdir);
    inode* newnode = inode_new();
    MYfs_createObject(rawfs, destname, newdir, newnode, FT_FILE);

    FILE* fp = fopen(hostfile, "rb"); //쓰기+읽기 pc에서 읽어오는 파일.
    char c;
    char* str = malloc(256); //원본 파일로부터 받아올 내용을 담을 문자열
    int filesize = 0; //while문
    while ((c = fgetc(fp)) != EOF) {
        *(str + (filesize%256)) = c;// str 변수에 원본 파일 문자를 하나하나 받아와서 저장.
        if((filesize+1)%256==0){
            MYfs_appendObject(rawfs,newnode,str,256);
        }
        filesize++;
    }fclose(fp);
    if((filesize+1)%256!=0){
        MYfs_appendObject(rawfs,newnode,str,filesize%256);
    }
    free(str);
}

void mymkdir(char *dirpath){
    char *destname=strrchr(dirpath,'/');
    if(destname==NULL){
        destname=dirpath;
        dirpath=".";
    }
    else {
        *destname='\0';
        destname+=1;
    }
    Stack *path=Stack_new();
    pathResolving(dirpath,path);
    inode *dirparentnode=inode_new();
    MYfs_getInode(rawfs,Stack_top(path),dirparentnode);
    inode *tmpinode=inode_new();
    MYfs_createObject(rawfs,destname,dirparentnode,tmpinode,FT_DIR);

    inode_delete(dirparentnode);
    inode_delete(tmpinode);
    Stack_delete(path);
}

void myrmdir(char *dirpath){
    Stack *dir = Stack_new();
    pathResolving(dirpath,dir);
    inode *dirnode=inode_new();
    inode *dirparentnode=inode_new();
    MYfs_getInode(rawfs,*(dir->stk+(dir->cur-2)),dirparentnode);
    MYfs_getInode(rawfs,Stack_top(dir),dirnode);
    MYfs_removeObject(rawfs,dirnode,dirparentnode);
    //TODO 오류 메세지 출력?
}

void myrm(char *filepath){
    Stack *dir = Stack_new();
    pathResolving(filepath,dir);
    inode *dirnode=inode_new();
    inode *dirparentnode=inode_new();
    MYfs_getInode(rawfs,*(dir->stk+(dir->cur-2)),dirparentnode);
    MYfs_getInode(rawfs,Stack_top(dir),dirnode);
    MYfs_removeObject(rawfs,dirnode,dirparentnode);
}

void mymv(char *srcpath, char *destpath) { // 현재 dir에 둘다 있다고 가정.
    Stack *srcstk = Stack_new();
    pathResolving(srcpath,srcstk);
    Stack *dststk = Stack_new();
    pathResolving(destpath,dststk);

    inode *srcnode=inode_new();
    inode *srcparentnode=inode_new();

    MYfs_getInode(rawfs,*(srcstk->stk+(srcstk->cur-2)),srcparentnode);
    MYfs_getInode(rawfs,Stack_top(srcstk),srcnode);

    if(Stack_top(dststk)==0xffff){ // <- file file
        inode *dstdir=inode_new();
        MYfs_getInode(rawfs,*(dststk->stk+(dststk->cur-2)),dstdir);
        MYfs_moveObject(rawfs,srcnode,srcparentnode,dstdir);
        char *newname = strrchr(destpath,'/');
        newname=(newname==NULL)?destpath:newname+1;
        MYfs_renameObject(rawfs,srcnode,newname);
    }else{ // <- file directory
        inode *dstdir=inode_new();
        MYfs_getInode(rawfs,Stack_top(dststk),dstdir);
        MYfs_moveObject(rawfs,srcnode,srcparentnode,dstdir);
    }
    
}

void mytouch(char* myfile) {
    Stack* dir_stk = Stack_new();
    pathResolving(myfile, dir_stk);
    
    inode* nowparentdir=inode_new();
    MYfs_getInode(rawfs,*(dir_stk->stk+(dir_stk->cur-2)),nowparentdir);

    if (Stack_top(dir_stk)==0xFFFF) { //현재 dir에 myfile이 없는 경우, myfile을 그냥 0바이트로 만듦.
        inode *tmpnode=inode_new();
        MYfs_createObject(rawfs,myfile,nowparentdir,tmpnode,FT_FILE);
    }
    else { //현재 dir에 myfile이 있는 경우
        inode* nowdir=inode_new();
        MYfs_getInode(rawfs, Stack_top(dir_stk), nowdir);//현재 이동하려는 파일의 부모 di
        nowdir->createdTime = time(0);
        MYfs_setInode(rawfs, nowdir);
    }
}

void myinode(char *inode_number) { /* 피드백 언제든지 대 환영 여기 위치에 피드백 적어주세요 :
11/05일 내로 직접 데이터 블록이 8개가 아니면 출력 그만 하는 분기문 추가할 예정.*/
    int a = atoi(inode_number)-1;
    inode *node = inode_new();
    MYfs_getInode(rawfs, a, node);
    if (node->fileType == 0)
        printf("종류 : 파일 \n");
    else
        printf("종류 : 디렉토리\n");

    char *buff=malloc(20);
    strftime(buff, 20, "%Y/%m/%d %H:%M:%S", localtime(&(node->createdTime)));
    printf("생성 일자 : %s\n", buff);
    printf("크기 : %lld\n", node->fileSize);
    printf("직접 블록 목록 : \n");
    for (int i = 0; i < 8; i++) {
        if (*(node->directBlocks + i) == 0xffff) //NULL로 제어를 하는게 맞는지?
            break;
        printf("\t#%d 직접 데이터 블록 : %d\n", i, *(node->directBlocks + i)+1);
    }
    printf("간접 블록 번호 : %d\n", node->indirectBlock+1); // WIP
    free(buff);
}

void mydatablock(char *inode_number){
    
    int a = atoi(inode_number)-1;
    
    byte *buf=malloc(DATABLOCKSIZE+1);
    MYfs_getDataBlock(rawfs,a,buf);
    *(buf+256)=0;
    for(int i=0;i<DATABLOCKSIZE;i++)putchar(*(buf+i));
    putchar('\n');
}


void mystate(){

    int usedInode=0;
    for(int i=0;i<INODELISTLEN;i++)usedInode += MYfs_getSuperBlockVal(rawfs,i);

    int usedDataBlock=0;
    for(int i=INODELISTLEN;i<INODELISTLEN+DATABLOCKSLEN;i++)usedDataBlock+=MYfs_getSuperBlockVal(rawfs,i);

    printf("Inode state : \n");
    printf("Total : %d\n", INODELISTLEN);
    printf("Used : %d\n", usedInode);
    printf("Available : %d\n", INODELISTLEN-usedInode);
    printf("Inode Map : \n");
    for(int i=0;i<INODELISTLEN;i++){
        putchar('0'+MYfs_getSuperBlockVal(rawfs,i));
        if((i+1)%4==0)putchar(' ');
        if((i+1)%(4*16)==0)putchar('\n');
    }
    putchar('\n');

    printf("Data Block state : \n");
    printf("Total : %d blocks / %d byte \n", DATABLOCKSIZE,DATABLOCKSLEN*DATABLOCKSLEN);
    printf("Used : %d blocks / %d byte \n",usedDataBlock,usedDataBlock*DATABLOCKSIZE);
    printf("Available : %d blocks / %d byte \n",
        DATABLOCKSIZE-usedDataBlock,(DATABLOCKSLEN-usedDataBlock)*DATABLOCKSLEN);
    printf("Data Block Map : \n");
    for(int i=INODELISTLEN;i<INODELISTLEN+DATABLOCKSLEN;i++){
        putchar('0'+MYfs_getSuperBlockVal(rawfs,i));
        if((i+1)%4==0)putchar(' ');
        if((i+1)%(4*16)==0)putchar('\n');
    }
    putchar('\n');
}


void mytree(char *path){
    Stack *stk = Stack_new();
    pathResolving(path,stk);
    inode *tmp=inode_new();
    int tmpint = Stack_top(stk);
    MYfs_getInode(rawfs,tmpint,tmp);


    if(tmp->fileType==FT_DIR){
        // 탐색
        Stack *searchStk=Stack_new();
        Stack *depthStk=Stack_new();
        char *tmpfilename = malloc(8);


        MYfs_getFileName(rawfs,tmp,tmpfilename);
        if(tmp->index==0)strcpy(tmpfilename,"/");
        ushort *objz=malloc(tmp->fileSize);
        MYfs_readObject(rawfs,tmp,(byte*)objz);
        for(int i=0;i<(int)(tmp->fileSize/sizeof(ushort));i++){
            Stack_push(searchStk,*(objz+i));
            Stack_push(depthStk,1);
        }
        free(objz);
        printf("%s\n",tmpfilename);

        while(searchStk->cur>0){
            uint searchNum = Stack_pop(searchStk);
            uint depth = Stack_pop(depthStk);

            MYfs_getInode(rawfs,searchNum,tmp);
            memset(tmpfilename,0,8);
            MYfs_getFileName(rawfs,tmp,tmpfilename);
            for(int i=0;i<depth;i++)printf("\t\t");
            printf("->%s\n",tmpfilename);

            if(tmp->fileType==FT_DIR){
                objz=malloc(tmp->fileSize);
                MYfs_readObject(rawfs,tmp,(byte*)objz);
                for(int i=0;i<(int)(tmp->fileSize/sizeof(ushort));i++){
                    Stack_push(searchStk,*(objz+i));
                    Stack_push(depthStk,depth+1);
                }
                free(objz);
            }
        }
    }else{
        printf("Not directory\n");
    }
}

//------------------------------------

void printState(){
    char *path=calloc(cursorInodeStack->cur*8,sizeof(char));
    *(path+0)='/';  
    *(path+1)='\0';
    
    printf("[");
    mypwd();
    printf(" ]$ ");
    free(path);              
}

void processCmdInput(){
    char *ipt=calloc(CMDBUFSIZE,sizeof(char));
    char *originalipt=calloc(CMDBUFSIZE,sizeof(char));
    char *command=calloc(CMDBUFSIZE,sizeof(char));
    char *filename=calloc(CMDBUFSIZE,sizeof(char));
    //------------------------------------
    memset(filename,0,CMDBUFSIZE*sizeof(char));
    memset(command,0,CMDBUFSIZE*sizeof(char));
    memset(ipt,0,CMDBUFSIZE*sizeof(char));
    memset(originalipt,0,CMDBUFSIZE*sizeof(char));
    int i;
    for(i=0; (*(originalipt+i)=getchar())!='\n'||i==0;i++)if(*(originalipt+i)=='\n')i--;
    *(originalipt+i)='\0';
    strcpy(ipt,originalipt);
    // fflush(stdin);

    char *a1p,*a2p,*a3p;
    int cursor=0;
    
    while(*(ipt+cursor)!='\0'&&*(ipt+cursor)!=' ')cursor++;
    while(*(ipt+cursor)==' '){*(ipt+cursor)='\0';cursor++;}
    a1p=ipt+cursor;
    while(*(ipt+cursor)!='\0'&&*(ipt+cursor)!=' ')cursor++;
    while(*(ipt+cursor)==' '){*(ipt+cursor)='\0';cursor++;}
    a2p=ipt+cursor;
    while(*(ipt+cursor)!='\0'&&*(ipt+cursor)!=' ')cursor++;
    while(*(ipt+cursor)==' '){*(ipt+cursor)='\0';cursor++;}
    a3p=ipt+cursor;
    // mycat >> file1 file2- ipt가 mycat a1p =>> a2p=file1 a3p=file2 
    // myInode 4 ipt가 myIonde a1p = 4(char타입)
    

    // should crop
    if(strcmp(ipt,"mymkfs")==0)mymkfs(false);
    else if(strcmp(ipt,"myls")==0)myls(a1p);
    else if(strcmp(ipt,"mycat")==0)mycat(a1p);
    else if(strcmp(ipt,"myshowfile")==0)myshowfile(a1p,a2p,a3p);
    else if(strcmp(ipt,"mypwd")==0){mypwd();putchar('\n');}
    else if(strcmp(ipt,"mycd")==0)mycd(a1p);
    else if(strcmp(ipt,"mycp")==0)mycp(a1p,a2p);
    else if(strcmp(ipt,"mycpto")==0)mycpto(a1p,a2p);
    else if(strcmp(ipt,"mycpfrom")==0)mycpfrom(a1p,a2p);
    else if(strcmp(ipt,"mymkdir")==0)mymkdir(a1p);
    else if(strcmp(ipt,"myrmdir")==0)myrmdir(a1p);
    else if(strcmp(ipt,"myrm")==0)myrm(a1p);
    else if(strcmp(ipt,"mymv")==0)mymv(a1p,a2p);
    else if(strcmp(ipt,"mytouch")==0)mytouch(a1p);
    else if(strcmp(ipt,"myinode")==0)myinode(a1p);
    else if(strcmp(ipt,"mydatablock")==0)mydatablock(a1p);
    else if(strcmp(ipt,"mystate")==0)mystate();
    else if(strcmp(ipt,"mytree")==0)mytree(a1p);
    else if(strcmp(ipt,"exit")==0){printf("Bye~~~\n");exit(0);}
    else system(originalipt);//printf("Unknown Command\n");
    free(ipt);
    printf("\n");
}

int main(void){
    cursorInodeStack = Stack_new();
    Stack_push(cursorInodeStack, 0);
    if((rawfs=fopen("myfs","r"))==NULL){
        printf("파일시스템이 없습니다. 파일시스템을 만듭니다.\n");
        rawfs=fopen("myfs","w+b");
        mymkfs(true);
        fclose(rawfs);
        rawfs=fopen("myfs","r+b");
    }else{
        rawfs=fopen("myfs","r+b");
    }
    while(true){
        printState();
        processCmdInput();
    }
}
