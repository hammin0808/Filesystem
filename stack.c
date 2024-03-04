#include "stack.h"

Stack *Stack_new(){
    Stack * this=malloc(sizeof(Stack));
    this->stk=calloc(8,sizeof(uint));
    this->allclen=8;
    this->cur=0;
    return this;
}
 
void Stack_delete(Stack *this){
    free(this->stk);
    free(this);
}

void Stack_allocMore(Stack *this){
    uint *newstk = calloc(this->allclen+8,sizeof(uint));
    memcpy(newstk,this->stk,this->cur * sizeof(uint));
    this->stk=newstk;
    this->allclen+=8;
}
 
void Stack_push(Stack *this, uint val){
    while(this->cur>=this->allclen)Stack_allocMore(this);
    *(this->stk+(this->cur++))=val;
}
 
uint Stack_pop(Stack *this){
    return this->cur>0 ? *(this->stk+(--(this->cur))) : -1;
}
 
uint Stack_top(Stack *this){
    return *(this->stk+(this->cur-1));
}