#include "consts.h"
#include "inode.h"

// Note:
// Object <= dir|file

void MYfs_init(FILE *fp);

bool MYfs_getSuperBlockVal(FILE *fp,uint idx); // 
int MYfs_getUnusedSuperBlockIdx(FILE *fp,bool isInode);
void MYfs_setSuperBlockVal(FILE *fp,uint idx,bool val,bool isInode); // 아이노드 번호(=목차,index)를 통해 슈퍼블록을 셋팅

void MYfs_getInode(FILE *fp,uint idx,inode *ret); // 아이노드 번호(idx)를 입력하면 inode구조체가 *ret값에 저장됨.
void MYfs_getInodeByFileName(FILE *fp,char *filename,inode *parentdir,inode *ret); // 파일이름과 부모디렉토리를 입력하면 inode구조체가 *ret값에 저장됨.
void MYfs_setInode(FILE *fp,inode *inode);//새로운 아이노드를 선언할때 사용 etc) inode node_new; setInode(fp,node_new);

void MYfs_getDataBlock(FILE *fp, uint idx, byte* ret); // 아이노드 번호(=인덱스)를 통해 데이터 블록의 내용을 byte형 포인터에 저장.
void MYfs_setDataBlock(FILE *fp,uint idx, byte* datablk); // 아이노드 번호를 통해 byte형 data를 데이터 블록에 저장.

void MYfs_createObject(FILE *fp,char *dirname,inode *parentdir,inode *newdir,bool fileType); // 파일이나 디렉토리를 생성. 인자중 dirname은 초기에 이 함수가 dir를 만드는 함수였기 때문.
void MYfs_moveObject(FILE *fp, inode *file, inode *parentdir, inode *newparentdir); //파일이나 디렉토리를 자신의 아이노드 , 현재 부모 디렉토리의 아이노드, 옮기려는 디렉토리의 아이노드를 가지고 옮기는 함수. 
void MYfs_renameObject(FILE *fp, inode *file, char *newname); // 파일의 아이노드와 새로운 문자열 포인터만들 가지고 이름을 바꾸는 함수
void MYfs_removeObject(FILE *fp, inode *file, inode *parentdir); // 파일의 아이노드와 부모 디렉토리의 아이노드를 넣으면 해당 파일을 삭제하는 함수.
void MYfs_writeObject(FILE *fp, inode *inode, byte *content, size_t contentlen); // 파일의 아이노드와 작성할 문자열포인터, 그 문자열포인터의 크기를 이용하여 파일을 작성하는 함수
void MYfs_appendObject(FILE *fp, inode *inode, byte *content, size_t contentlen); // 이미 작성된 파일 뒤에 이어서 첨삭하는 함수.
void MYfs_readObject(FILE *fp,inode *inode,byte *out);// 파일의 아이노드를 통해 문자열포인터에 파일의 내용을 받아오는 함수.
void MYfs_getFileName(FILE *fp,inode *inode,byte *out); // 파일의 아이노드를 통해 문자열포인터에 파일의 이름을 받아오는 함수.
ushort MYfs_getParentInodeIdx(FILE *fp,inode *file); //해당 파일의 부모 디렉토리의 아이노드 인덱스를 반환하는 함수.