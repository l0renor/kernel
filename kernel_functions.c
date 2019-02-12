#include "kernel_functions.h"
#include "globalVariables.c"
#include <limits.h>
extern void __save_context(void);
extern void LoadContext(void);
extern void SelectPSP(void);
extern void __get_PSP_from_TCB(void);

// Task administration

int init_kernel( void )
{
  in_startup = TRUE;
  TimerInt();
  ReadyList = create_list();
  blocked_list = create_list();
  sleep_list = create_list();
  if(sleep_list == NULL || ready_list == NULL || blocked_list == NULL){
    return FAIL;
  }
  
  exception idleTaskException = create_task(idle_function, UINT_MAX);
  //test code list 
  idleTaskException = create_task(idle_function, 1); 
  idleTaskException = create_task(idle_function, 7); 
  idleTaskException = create_task(idle_function, 2);
  //end test code 
  
  return idleTaskException;
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
  if ( in_startup )
  {
    //Create ListObj for TCB
    listobj* o = create_listobj(pTCB);
    //Insert new task in Readylist
    sorted_insert(ready_list, o);
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
      sorted_insert(ready_list, o);
      
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
  listobj *toDelObject = ready_list->pHead->pNext;
  TCB   *toDelTCB = toDelObject->pTask;
  ready_list->pHead->pNext =  ready_list->pHead->pNext->pNext;
  ready_list->pHead->pNext->pNext->pPrevious = ready_list->pHead->pNext;
  //Set running task
  insertion_sort(ready_list);
  RunningTask = ready_list->pHead->pNext->pTask;
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
  in_startup = FALSE;
  //Enable interrupts
  isr_on();
  //Load context
  LoadContext();
}

// Communication

mailbox* create_mailbox( uint nMessages, uint nDataSize)
{
  mailbox *result = (mailbox*)calloc(1,sizeof(mailbox));
  result->nDataSize = nDataSize;
  result->nMessages = nMessages;
  result->nBlockedMsg = 0;
  result->pHead = create_message(NULL,0);
  result->pTail = create_message(NULL,0);
  result->pHead->pNext = result->pTail;
  result->pTail->pPrevious = result->pHead;
  return result;
  
}

exception remove_mailbox( mailbox* mBox )
{
  //IF Mailbox is empty THEN
  if ( mBox->nMessages == 0 )
  {
    //Free the memory for the Mailbox
    free(mBox->pHead);
    free(mBox->pTail);
    free(mBox);
    return OK;
  }
  else
  {
    return NOT_EMPTY;
  }
}

exception receive_wait( mailbox* mBox, void* pData )
{
  static bool is_first_execution = TRUE;
  //Disable interrupt
  isr_off();
  //Save context
  SaveContext();
  //IF first execution THEN
  if ( is_first_execution == TRUE )
  {
    //SET "not first execution any more"
    is_first_execution = FALSE;
    //IF send Message is waiting THEN
    if (mBox->nMessages > 0 && mBox->nBlockedMsg >= 0)
    {
      msg* sender = pop_mailbox_head(mBox);
      mBox->nMessages--;
      mBox->nBlockedMsg--;
      //Copy sender's data to receiving task's data area
      memcpy(pData, sender->pData, mBox->nDataSize);
      //IF Message was of wait type THEN
      if ( sender->pBlock != NULL )
      {
        remove_from_list(blocked_list, sender->pBlock);
        sorted_insert(ready_list, sender->pBlock);
      }
      else 
      {
        void* p = realloc(sender->pData, mBox->nDataSize);
        free(p);
      }
      free(sender);
    }
    else
    { 
      //Allocate a Message structure
      msg* message = ( msg* ) calloc( 1, sizeof( msg ) );
      //Add Message to the Mailbox
      push_mailbox_tail(mBox, message);
      //Move receiving task from Readylist to Waitinglist
      listobj* runningTaskObject = ready_list->pHead->pNext;
      remove_from_list(ready_list, runningTaskObject);
      sorted_insert(blocked_list, runningTaskObject);
    }
    LoadContext();
  }
  else
  {
    if (deadline() <= 0)
    {
      //Disable interrupt
      isr_off();
      //Remove receive Message
      remove_running_task_from_mailbox(mBox);
      //Enable interrupt
      isr_on();
      //Return DEADLINE_REACHED
      return DEADLINE_REACHED;
    }
    else
    {
      //Return OK
      return OK;
    }
  }
  return DEADLINE_REACHED;//make compiler happy
}




exception send_wait( mailbox* mBox, void* pData )
{
  static bool is_first_execution = TRUE;
  //Disable interrupts
  isr_off();
  //Save context
  SaveContext();
  //IF "first execution" THEN
  if ( is_first_execution )
  {
    //Set: "not first execution any more"
    is_first_execution = FALSE;
    if(mBox->nBlockedMsg < 0)
    {
      //receiving tasks waiting
      msg* m = pop_mailbox_head(mBox);
      //copy senders data into reciver
      memcpy(m->pData,pData,mBox->nDataSize);
      
      
      remove_from_list( blocked_list, m->pBlock);
      sorted_insert( ready_list, m->pBlock);
      //todo swich running task somewhere 
      free(m);
      
    }
    else
    {
      msg* newM = (msg *)calloc(1,sizeof(msg));
      newM->pData = pData;
      push_mailbox_tail(mBox,newM);
      remove_from_list(ready_list,ready_list->pHead->pNext);
    }
    LoadContext();
    
    
  }else//not first excecution
  {
    if(deadline()<=0)
    {
      isr_off();
      remove_running_task_from_mailbox(mBox);
      return DEADLINE_REACHED;
    }
    else
    {
      //IS pepsi 
      return OK;
    }
  }
  return DEADLINE_REACHED;//make compiler happy
}

exception receive_no_wait( mailbox* mBox, void* pData )
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
    static exception message_received = FAIL;
    //IF send Message is waiting THEN
    if ( mBox->nMessages > 0 && mBox->nBlockedMsg >= 0 )
    {
      message_received = OK;
      msg* sender = pop_mailbox_head(mBox);
      mBox->nMessages--;
      mBox->nBlockedMsg--;
      //Copy sender's data to receiving task's data area
      memcpy(pData, sender->pData, mBox->nDataSize);
      //IF Message was of wait type THEN
      if ( sender->pBlock != NULL )
      {
        remove_from_list(blocked_list, sender->pBlock);
        sorted_insert(ready_list, sender->pBlock);
      }
      else 
      {
        void* p = realloc(sender->pData, mBox->nDataSize);
        free(p);
      }
      free(sender);
    }
    //Load context
    LoadContext();
  }
  //Return status on received message
  return message_received;
}



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
    insertion_sort(ready_list);
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


