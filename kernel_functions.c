#include "kernel_functions.h"
#include <limits.h>
extern void __save_context(void);
extern void LoadContext(void);
extern void SelectPSP(void);
extern void __get_PSP_from_TCB(void);

TCB *RunningTask;
bool in_startup;
list *readyList;
list *blockedList; 
list *sleepList;

exception create_task(void(*task_body)(), uint deadline);

void run(void);

* _list createList(){
  list *list =  calloc(1,sizeof(_list));
  list->pHead = calloc(1,sizeof(l_obj));
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

exception create_task(void(*task_body)(), uint deadline)
{
  //Allocate memory for TCB
  isr_off(); /* protextion of calloc */
  ptr_tcb = (TCB *)calloc(1,sizeof(TCB));
  if ( ptr_tcb == NULL ) {
    //F: perhaps a real solution in the future...
    return FAIL;
  }
  
  //Set deadline in TCB
  ptr_tcb->DeadLine = deadline;
  
  //Set the TCB's PC to point to the task body
  ptr_tcb->PC = task_body;
  
  //Set TCB's SP to point to the stack segment
  //First element on stack is omitted for safety reasons
  ptr_tcb->StackSeg[STACK_SIZE-2] = 0x21000000; /* PSR */
  ptr_tcb->StackSeg[STACK_SIZE-3] = (uint)task_body; /* PC */
  ptr_tcb->StackSeg[STACK_SIZE-4] = 0; /* LR */
  //Skip some Stack elements for r12, r3, r2, r1, r0.
  ptr_tcb->SP = &( ptr_tcb->StackSeg[STACK_SIZE-9] );
    
  //IF start-up mode THEN
  if ( in_startup )
    //Insert new task in Readylist
    
    //Return status
  //ELSE
    //Disable interrupts
    //Save context
    //IF "first execution" THEN
      //Set: "not first execution any more"
      //Insert new task in Readylist
      //Load Context
    //ENDIF
  //ENDIF
  //Return status
  return FAIL;
}

void run(void)
{
  RunningTask = ptr_tcbForTaskOne;
  LoadContext();
}

exception init_kernel(void){
  exception result = OK;
  in_startup = TRUE;
  timerTimerInt();
  isr_off()
  readyList = createList();
  if(readyList == NULL){
  result = FAIL;
  }
  blockedList =  createList();
  if(blockedList == NULL){
  result = FAIL;
  }
  sleepList =  createList():
  if(blockedList == NULL){
  result = FAIL;
  }
  isr_on();
  void idle_function(void){
    while(1){}
  }
  exception idleTaskException = create_task(idle_function, UINT_MAX);
  
  return result && idleTaskException;
  
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
