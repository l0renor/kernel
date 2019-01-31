#include "kernel_functions.h"
#include <limits.h>
extern void __save_context(void);
extern void LoadContext(void);
extern void SelectPSP(void);
extern void __get_PSP_from_TCB(void);

void run(void);

void Task1(void);
void Task2(void);

TCB *RunningTask;
bool in_startup;
list *readyList;
list *blockedList; 
list *sleepList;

// void (*taskPointer)(void);

*list createList(){
  list *readyList =  calloc(1,sizeof(list));
  list ->pHead = calloc(1,sizeof(l_obj));
  list->pTail = calloc(1,sizeof(l_obj));
  //Head
  list->pHead->pTask = NULL;
  list->pHead->nTCnt = 0;
  list->pHead->pMessage = NULL;
  list->pHead->pPrevious = NULL;
  list->pHead->pNext = list->pTail;
  //Tail
  list->pTail->pTask = NULL;
  list->pTail->nTCnt = 0;
  list->pTail->pMessage = NULL;
  list->pTail->pPrevious = list->pHead;
  list->pTail->pNext = list->NULL;
  return list;
}

void TimerInt(void);

void run(void)
{
  RunningTask = ptr_tcbForTaskOne;
  LoadContext();
}

exception init_kernel(void){
  extern bool in_startup = TRUE;
  timerTimerInt();
  isr_off()
  extern list *readyList = createList();
  extern list *blockedList =  createList();
  extern list *sleepList =  createList():
  isr_on();
  void idle_function(void){
    while(1){}
  }
  
}


void TimerInt(void)
{
  static unsigned int ticks, first = 1;
  if (first == 1) 
  {
    first = 0; 
    ticks = 1;
  }
  else 
  {
    ticks = ticks + 1;
  }
}
