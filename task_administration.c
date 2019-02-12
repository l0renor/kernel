#include <limits.h>
// Task administration

int init_kernel( void )
{
  KernelMode = INIT;
  TimerInt();
  ReadyList = create_list();
  WaitingList = create_list();
  TimerList = create_list();
  if(TimerList == NULL || ReadyList == NULL || WaitingList == NULL){
    return FAIL;
  }
  return OK;
}

exception create_task( void(* body)(), uint d )
{
  //Allocate memory for TCB
  isr_off(); /* protextion of calloc */
  TCB* pTCB = (TCB *)calloc(1,sizeof(TCB));
  if ( pTCB == NULL ) {
    //F: perhaps a real solution in the future...
    return FAIL;
  }
  
  //Set deadline in TCB
  pTCB->DeadLine = d;
  
  //Set the TCB's PC to point to the task body
  pTCB->PC = body;
  
  //Set TCB's SP to point to the stack segment
  //First element on stack is omitted for safety reasons
  pTCB->StackSeg[STACK_SIZE-2] = 0x21000000; /* PSR */
  pTCB->StackSeg[STACK_SIZE-3] = (uint)body; /* PC */
  pTCB->StackSeg[STACK_SIZE-4] = 0; /* LR */
  //Skip some Stack elements for r12, r3, r2, r1, r0.
  pTCB->SP = &( pTCB->StackSeg[STACK_SIZE-9] );
  
  //IF start-up mode THEN
  if (KernelMode)
  {
    //Create ListObj for TCB
    listobj* o = create_listobj(pTCB);
    //Insert new task in Readylist
    sorted_insert(ReadyList, o);
    //Return status
    return OK;
  }
  //ELSE
  else
  {
    static bool is_first_execution = TRUE;
    //Disable interrupts
    isr_off();
    //Save context
    SaveContext();
    //IF "first execution" THEN
    if ( is_first_execution == TRUE )
    {
      //Set: "not first execution any more"
      is_first_execution = FALSE;
      //Create ListObj for TCB
      listobj* o = create_listobj(pTCB);
      //Insert new task in Readylist
      sorted_insert(ReadyList, o);
      
      //Load Context
      LoadContext();
    }
  }
  //Return status
  return FAIL;
}

void terminate()
{
  //remove from readyList
  listobj *toDelObject = ReadyList->pHead->pNext;
  TCB   *toDelTCB = toDelObject->pTask;
  ReadyList->pHead->pNext =  ReadyList->pHead->pNext->pNext;
  ReadyList->pHead->pNext->pNext->pPrevious = ReadyList->pHead->pNext;
  //Set running task
  insertion_sort(ReadyList);
  RunningTask = ReadyList->pHead->pNext->pTask;
  //Delete old task
  free(toDelTCB);
  free(toDelObject);
  isr_off();
  LoadContext(); 
}

void run( void ) 
{
  //Initialize interrupt timer
  
  //Set the kernel in running mode
  KernelMode = RUNNING;
  //Enable interrupts
  isr_on();
  //Load context
  LoadContext();
}