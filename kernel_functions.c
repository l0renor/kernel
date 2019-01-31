#include "kernel_functions.h"
#include <limits.h>
extern void __save_context(void);
extern void LoadContext(void);
extern void SelectPSP(void);
extern void __get_PSP_from_TCB(void);

TCB     *RunningTask;
bool    in_startup;
list*   ready_list, blocked_list, sleep_list;

void run(void);

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
  if ( in_startup )
  {
    //Create ListObj for TCB
    lo = create_listobj(pTCB);
    //Insert new task in Readylist
    sortedInsert(ready
    
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
  }
  //Return status
  return FAIL;
  
  
}

exception init_kernel(void){
  exception result = OK;
  in_startup = TRUE;
  TimerInt();
  isr_off();
  list *ready_list = create_list();
  if(ready_list == NULL){
    result = FAIL;
  }
  list *blocked_list =  create_list();
  if(blocked_list == NULL){
    result = FAIL;
  }
  list *sleep_list =  create_list();
  if(blocked_list == NULL){
    result = FAIL;
  }
  isr_on();
  
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

void insertion_sort(list* l) 
{ 
  list* sorted = create_list(); 
  listobj* current = l->pHead->pNext; 
  l->pHead->pNext = NULL;
  while (current->pTask != NULL)
  {   
    listobj* next = current->pNext; 
    current->pPrevious = current->pNext = NULL;
    sorted_insert(sorted, current); 
    current = next; 
  }  
  //@TODO: Free memory space taken by original l
  l = sorted;
} 

// Only call if l is already sorted!
void sorted_insert(list* l, listobj* o)
{ 
  listobj* current;
  listobj* first = l->pHead->pNext; /* skip head */

  // SPECIAL CASE: Empty List.
  if (first->pTask == NULL) /* check for tail */
  {
    o->pNext = l->pTail;
    o->pPrevious = l->pHead;
    l->pHead->pNext = o;
    l->pTail->pPrevious = o;
  }
  // SPECIAL CASE: New object has lower deadline than first in list.
  else if (o->pTask->DeadLine < first->pTask->DeadLine)
  { 
    o->pNext = first; 
    o->pNext->pPrevious = o;  
  } 
  // All other cases.
  else 
  { 
    current = first;
    // Iterate through list until current followers deadline is smaller than the one of the new object.
    while (current->pNext->pTask->DeadLine <= o->pTask->DeadLine && current->pNext->pTask != NULL)
    {
      current = current->pNext;
    }
    // Changing the pointers.
    o->pNext = current->pNext;
    o->pPrevious = current;
    o->pNext->pPrevious = o;
    current->pNext = o;
  } 
}

static listobj* create_listobj( TCB* t )
{
  listobj* o = ( listobj* ) calloc( 1, sizeof( listobj ) );
  o->pTask = t;
  o->nTCnt = 0;
  o->pMessage = NULL;
  o->pPrevious = NULL;
  o->pNext = NULL;
  return o;
}

static list* create_list( void )
{
  list* l = ( list* ) calloc( 1, sizeof( list ) );
  l->pHead = create_listobj(NULL);
  l->pTail = create_listobj(NULL);
  l->pHead->pNext = l->pTail;
  l->pTail->pPrevious = l->pHead;
  return l;
}

static void idle_function(void)
{
  while(1)
  {
    //SWYM
  }
}


////deprecated
//static void add_to_list( list* l, listobj* o ) {
//  listobj* last = l->pTail->pPrevious;
//  last->pNext = o;
//  o->pPrevious =last;
//  o->pNext = l->pTail;
//  l->pTail->pPrevious = o;
//}


