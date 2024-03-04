#include "consts.h"
//스택은 주로 디렉토리 관련 작업에서 사용
typedef struct Stack
{
    uint *stk;
    uint cur;
    uint allclen;
} Stack;

Stack *Stack_new(); //새로운 스택 선언 후 사용 etc) Stack *stk=Stack_new();
void Stack_delete(Stack *this); // 스택 내용을 지움.
void Stack_allocMore(Stack *this); // 스택에 새로운 내용 할당
void Stack_push(Stack *this, uint val); // 현재 스택 제일 위에 값을 추가
uint Stack_pop(Stack *this); // 현재 스택 제일 위 값을 제거
uint Stack_top(Stack *this); // 현재 스택 제일 위 값을 반환만 하고 스택은 그대로