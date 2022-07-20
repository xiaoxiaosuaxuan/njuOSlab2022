#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>

#define MAX_NUM 128
#define STACK_SIZE 64

#ifdef DEBUG
  #define debug(fmt, ...) printf(fmt, __VA_ARGS__)
#else
  #define debug(fmt, ...) 
#endif


struct co* _pool[MAX_NUM];              //协程池
struct co  _co_main;                       //main
int _colast = 0;                        //池中最后一个协程的下标，初始为0，即主线程
int _current = 0;

enum co_status{
  CO_START = 1,
  CO_RUNNING,
  CO_WAIT,
  CO_DEAD,
};

struct co {
   char* name;
   void (*func) (void*);
   void* args;

   enum co_status status;
   struct co* co_wait;
   jmp_buf context;
   uint8_t _stack[STACK_SIZE * 1024 + 16];
   uint8_t* stack;                   //用来对齐

   int index;
};

void _init_main(){ 
  if (_pool[0] == NULL){
    srand(time(NULL));                     //顺便初始化以下随机数种子
    _pool[0] = &_co_main;
    _co_main.name = malloc(sizeof(char) * 5); assert(_co_main.name != NULL);
    strcpy(_co_main.name, "main");
    _co_main.status = CO_RUNNING;
    _co_main.co_wait = NULL;
    _co_main.index = 0;
  }
}
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  _init_main();
  struct co* conew = malloc(sizeof(struct co)); assert(conew);
  conew->name = malloc(sizeof(char) * (strlen(name) + 1)); assert(conew->name); 
  strcpy(conew->name, name);
  conew->func = func;
  conew->args = arg;
  conew->status = CO_START;
  conew->stack = conew->_stack + STACK_SIZE * 1024 + 16;
  conew->stack = conew->stack - ((uintptr_t)conew->stack & 15); 
  int flag = 0;
  for (int i = 1; i < MAX_NUM; ++i){
    if (_pool[i] == NULL){
      conew->index = i;
      _pool[i] = conew;
      flag = 1;
      if (i > _colast) _colast = i;
      break;
    }
  }
  assert(flag == 1);
  return conew;
}


void co_wait(struct co *co) {
  struct co* cocur = _pool[_current];
  cocur->status = CO_WAIT;
  cocur->co_wait = co;
  co_yield();
  if (co->index == _colast){
    for(int i = _colast -  1; i>=0 ; --i)
      if (_pool[i] != NULL){
        _colast = i; break;
      }
  }  
  _pool[co->index] = NULL;
  free(co->name);
  free(co); 
  cocur->co_wait = NULL;
  cocur->status = CO_RUNNING;
}

int _find(){                        //找一个可以执行的协程
  int candidate[128];
  int cnt = 0;
  for (int i = 0; i <= _colast; ++i){
    if (_pool[i] && _pool[i]->status != CO_DEAD && !(_pool[i]->status == CO_WAIT && _pool[i]->co_wait->status != CO_DEAD)){
      candidate[cnt++] = i;
    }
  }
  assert(cnt != 0);
  int choice = rand() % cnt;
  debug("\tcnt = %d, choice = %d\t", cnt, choice);
  return candidate[choice];
}

typedef void (*_func)(void);

void _pick_co();

void _pick_co_finish(){                                     //协程结束时执行，所以要把status标记成dead再调用_pick_co;
  _pool[_current]->status = CO_DEAD;
  _pick_co();
}

void _pick_co(){
  int i = _find(); assert(i != -1);
  _current = i;
  struct co* cop = _pool[i];
  if (cop->status == CO_START){
    
#if __x86_64__                                              //把协程结束时的返回地址压到栈里，即_pick_co_finish；因为协程结束后要重新挑选一个协程
  cop->stack = (void*)((uintptr_t)cop->stack - 8);
#else
  cop->stack = (void*)((uintptr_t)cop->stack - 4);
#endif
  *((_func*)(void*)cop->stack) = _pick_co_finish;

    cop->status = CO_RUNNING;

    __asm__ volatile(                                      //设置好pc和sp
    #if __x86_64__
        "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
        ::"b"((uintptr_t)cop->stack), "d"(cop->func),"a"(cop->args):"memory"
    #else
        "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
        ::"b"((uintptr_t)cop->stack - 8), "d"(cop->func),"a"(cop->args):"memory"
    #endif
    );
  }
  else longjmp(cop->context, 1);
}


void co_yield() {
  _init_main();
  int val = setjmp(_pool[_current]->context);
  if (val == 0){
    _pick_co();
  }
}
