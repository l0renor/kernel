#include <limits.h>
// Task administration

int init_kernel( void )
{
  Ticks = 0;
  ReadyList = create_list();
  WaitingList = create_list();
  TimerList = create_list();
  if(TimerList == NULL || ReadyList == NULL || WaitingList == NULL){
    free(TimerList);
    free(ReadyList);
    free(WaitingList);
    return FAIL;
  }
  create_task(idle_function,UINT_MAX);
  KernelMode = INIT;
  return OK;
}

exception create_task( void(* body)(), uint d )
{
  if ( body == NULL || d <= ticks()){
    return FAIL;
  } 
  //Allocate memory for TCB
  isr_off(); /* protextion of calloc */
  TCB* pTCB = (TCB *)calloc(1,sizeof(TCB));
  if ( pTCB == NULL ) {
    return FAIL;
  }
  
  //Set deadline in TCB
  pTCB->Deadline = d;
  
  //Set the TCB's PC to point to the task body
  pTCB->PC = body;
  pTCB->SPSR = 0x21000000;
  
  //Set TCB's SP to point to the stack segment
  //First element on stack is omitted for safety reasons
  pTCB->StackSeg[STACK_SIZE-2] = 0x21000000; /* PSR */
  pTCB->StackSeg[STACK_SIZE-3] = (uint)body; /* PC */
  //Skip some Stack elements for r12, r3, r2, r1, r0.
  pTCB->SP = &( pTCB->StackSeg[STACK_SIZE-9] );
  
  //IF start-up mode THEN
  if (KernelMode == INIT)
  {
    
    //Create ListObj for TCB
    listobj* o = create_listobj(pTCB);
    if ( o == NULL )
    {
      free(pTCB);
      return FAIL;
    }
    //Insert new task in Readylist
    sorted_insert(ReadyList, o);
  }
  else
  {
    
    //Disable interrupts
    isr_off();
    //Save context
    
    //Create ListObj for TCB
    listobj* o = create_listobj(pTCB);
    if ( o == NULL )
    {
      free(pTCB);
      return FAIL;
    }
    //Insert new task in Readylist
    PreviousTask = ReadyList->pHead->pNext->pTask;
    sorted_insert(ReadyList, o);
    NextTask = ReadyList->pHead->pNext->pTask;
    SwitchContext();
    
  }
  //Return status
  return OK;
}

void terminate()
{
  isr_off();
  listobj* leavingObj = ReadyList->pHead->pNext;
  remove_from_list(ReadyList, ReadyList->pHead->pNext);
  NextTask = getFirstRL();
  switch_to_stack_of_next_task();
  free(leavingObj->pTask);
  free(leavingObj);
  LoadContext_In_Terminate();
}

void run( void ) 
{
  NextTask = getFirstRL();
  KernelMode = RUNNING;
  LoadContext_In_Run();  
}