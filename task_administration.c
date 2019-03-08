#include <limits.h>

// Task administration

int init_kernel( void )
{
  //Set tick counter to zero
  Ticks = 0;
  
  //Create necessary data structures
  ReadyList = create_list();
  WaitingList = create_list();
  TimerList = create_list();
  if ( TimerList == NULL || ReadyList == NULL || WaitingList == NULL)
  {
    free(TimerList);
    free(ReadyList);
    free(WaitingList);
    return FAIL;
  }
  
  //Create an idle task
  if ( create_task(idle_function, UINT_MAX) == FAIL ) 
  {
    free(TimerList);
    free(ReadyList);
    free(WaitingList);
    return FAIL;
  }
  
  //Set the kernel in INIT mode
  KernelMode = INIT;
  return OK;
}

exception create_task( void(* body)(), uint d )
{

  if ( body == NULL || d <= ticks()){
    return FAIL;
  } 

  //Disable interrupts (to protect calloc)
  isr_off();

  //Allocate memory for TCB
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
  
  //Create ListObj for TCB
  listobj* o = create_listobj(pTCB);
  if ( o == NULL )
  {
    free(pTCB);
    return FAIL;
  }
  
  //IF start-up mode THEN
  if (KernelMode == INIT)
  {
    //Insert new task in Readylist
    sorted_insert(ReadyList, o);
    isr_on();
  }
  else
  {
    //Update PreviousTask
    PreviousTask = ReadyList->pHead->pNext->pTask;
    //Insert new task in Readylist
    sorted_insert(ReadyList, o);
    //Update NextTask
    NextTask = ReadyList->pHead->pNext->pTask;
    //Switch context
    if ( PreviousTask == NextTask )
    {
      isr_on();
    }
    else
    {
      SwitchContext();
    }
  }
  //Return status
  return OK;
}

void terminate()
{
  //Disable interrupts
  isr_off();
  //Get terminating task
  listobj* leavingObj = ReadyList->pHead->pNext;
  //Remove terminating task from ReadyList
  remove_from_list(ReadyList, leavingObj);
  //Update NextTask
  NextTask = getFirstRL();
  //Switch to next tasks stack
  switch_to_stack_of_next_task();
  //Free memory
  free(leavingObj->pTask);
  free(leavingObj);
  //Load context
  LoadContext_In_Terminate();
}

void run( void ) 
{
  //Update NextTask
  NextTask = getFirstRL();
  //Set kernel mode
  KernelMode = RUNNING;
  //LoadContext
  LoadContext_In_Run();  
}