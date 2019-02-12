#include "kernel_functions.h"
#include "global_variables.c"
#include "task_administration.c"
#include "communication.c"

extern void __save_context(void);
extern void LoadContext(void);
extern void SelectPSP(void);
extern void __get_PSP_from_TCB(void);






// Timing 

uint ticks( void )
{
  //Return the tick counter
  //TODO
}

uint deadline( void )
{
  //Return the deadline of the current task
  return RunningTask->Deadline;
}


void set_deadline( uint deadline )
{
  static bool is_first_execution = TRUE;
  //Disable interrupt
  isr_off();
  //Save context
  SaveContext();
  //IF first execution THEN
  if ( is_first_execution )
  {
    //Set: "not first execution any more"
    is_first_execution = FALSE;
    //Set the deadline field in the calling TCB.
    RunningTask->DeadLine = deadline;
    //Reschedule Readylist
    insertion_sort(ReadyList);
    //Load context
    LoadContext();
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

exception wait( uint nTicks ){
  isr_off();
  SaveContext();
  exception status;
  static bool is_first_execution = TRUE;
  if ( is_first_execution == TRUE )
  {
    is_first_execution = FALSE;
    listobj* running = ReadyList->pHead->pNext;
    remove_from_list(ReadyList,running);
    sorted_insert(TimerList,running);
    LoadContext();
    
  } else {//not first execution
    if(deadline()<=0)
    {
      isr_off();
      status = DEADLINE_REACHED;
    }
    else
    {
      status = OK;
    }
  }
  return status;
}

// Own helper functions

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
  listobj* first = l->pHead->pNext; /* skip head */
  
  // SPECIAL CASE: Empty List.
  if (first->pTask == NULL) /* check for tail */
  {
    o->pNext = l->pTail;
    o->pPrevious = l->pHead;
    l->pHead->pNext = o;
    l->pTail->pPrevious = o;
  }
  // SPECIAL CASE: New object has lower deadline than in list.
  else if (o->pTask->DeadLine < first->pTask->DeadLine)
  { 
    o->pNext = first;
    o->pPrevious = l->pHead;
    l->pHead->pNext = o;
    first->pPrevious = o;
  } 
  // All other cases.
  else 
  { 
    listobj* current = first;
    // Iterate through list until current followers deadline is smaller than the one of the new object.
    while (current->pNext->pTask->DeadLine <= o->pTask->DeadLine && current->pNext->pTask != NULL)
    {
      current = current->pNext;
    }
    // Changing the pointers.
    listobj* follower = current->pNext;
    o->pNext = follower;
    o->pPrevious = current;
    follower->pPrevious = o;
    current->pNext = o;
  } 
}

static listobj* create_listobj( TCB* t )
{       
  listobj* o = ( listobj* ) calloc( 1, sizeof( listobj ) );
  if(o != NULL){
    o->pTask = t;
    o->nTCnt = 0;
    o->pMessage = NULL;
    o->pPrevious = NULL;
    o->pNext = NULL;
  }
  return o;
}

static list* create_list( void )
{
  isr_off();
  list* l = ( list* ) calloc( 1, sizeof( list ) );
  if(l != NULL)
  {
    l->pHead = create_listobj(NULL);
    l->pTail = create_listobj(NULL);
    l->pHead->pNext = l->pTail;
    l->pTail->pPrevious = l->pHead;
  }
  isr_on();
  return l;
}

static void idle_function( void )
{
  while(1)
  {
    //SWYM
  }
}
static msg* create_message(char *pData,exception Status)
{
  msg *m = (msg*)calloc(1,sizeof(msg));
  m->pData = pData;
  m->Status = Status;
  m->pBlock = NULL;
  m->pPrevious = NULL;
  m->pNext = NULL;
  return m;
}

static msg* pop_mailbox_head(mailbox* mBox)
{
  msg* result =  mBox->pHead->pNext;
  //change pointers
  mBox->pHead->pNext = result->pNext;
  result->pPrevious = mBox->pHead;
  return result;
}


static void push_mailbox_tail( mailbox* mBox,msg* m)
{
  m->pNext = mBox->pTail;
  m->pPrevious = mBox->pTail->pPrevious;
  mBox->pTail->pPrevious->pNext = m;
  mBox->pTail->pPrevious = m;
}


static void remove_from_list( list* l, listobj* o)
{
  listobj* current = l->pHead;
  while(current!= o){
    current = current->pNext;
  }
  current->pPrevious->pNext = current->pNext;
  current->pNext->pPrevious= current->pPrevious;
}

static void remove_running_task_from_mailbox( mailbox* mBox )
{
  msg* current = mBox->pHead->pNext;
  while ( current->pBlock->pTask != RunningTask )
  {
    current = current->pNext;
  }
  current->pNext->pPrevious = current->pPrevious;
  current->pPrevious->pNext = current->pNext;
  free(current);
}

////deprecated
//static void add_to_list( list* l, listobj* o ) {
//  listobj* last = l->pTail->pPrevious;
//  last->pNext = o;
//  o->pPrevious =last;
//  o->pNext = l->pTail;
//  l->pTail->pPrevious = o;
//}


