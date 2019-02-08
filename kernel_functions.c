#include "kernel_functions.h"
#include <limits.h>
extern void __save_context(void);
extern void LoadContext(void);
extern void SelectPSP(void);
extern void __get_PSP_from_TCB(void);

TCB             *RunningTask;
static bool     in_startup;
static list*    ready_list;
static list*    blocked_list;
static list*    sleep_list;

// Task administration

int init_kernel( void )
{
  in_startup = TRUE;
  TimerInt();
  ready_list = create_list();
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
  if ( number_of_messages(mBox) == 0 )
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
    if ( mBox->nMessages > 0 && mBox->nBlockedMsg >= 0)
    {
      msg* sender = popHead(mBox);
      //Copy sender's data to receiving task's data area
      memcpy(
      //Remove sending task's message struct from the Mailbox
      //IF Message was of wait type THEN
        //Move sending task to Ready list
      //ELSE
        //Free senders data area
      //ENDIF
    //ELSE 
      //Allocate a Message structure
      //Add Message to the Mailbox
      //Move receiving task from Readylist to Waitinglist
    //ENDIF
    //Load context
  //ELSE
    //IF deadline is reached THEN
      //Disable interrupt
      //Remove receive Message
      //Enable interrupt
      //Return DEADLINE_REACHED
    //ELSE
      //Return OK
    //ENDIF
  //ENDIF
}




exception send_wait( mailbox* mBox, void* pData )
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
      if(mBox->nBlockedMsg < 0){//reciving tasks waiting
        msg* m = popHead(mBox);
        //copy senders data into reciver
        m->pData = (char*)calloc(1,sizeof(mBox->nDataSize));
        //todo free data?;
        //m->pBlock
        free(m);
        
        
        
        
      
      }
      //Copy senders data into data area of recivers message
      
      
    }
  }
  
  


// Timing 

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

static msg* popHead(mailbox* mBox)
{
msg* result =  mBox->pHead->pNext;
//change pointers
mBox->pHead->pNext = mBox->pHead->pNext->pNext;
mBox->pHead->pNext->pPrevious = mBox->pHead;
return result;
}

static void pushtail( msg* m,mailbox* mBox){
  m->pNext = mBox->pTail;
  m->pPrevious = mBox->pTail->pPrevious;
  mBox->pTail->pPrevious->pNext = m;
  mBox->pTail->pPrevious = m;
}

static void     remove_from_list( list* l, listobj* o){
listobj* current = l->pHead;
while(current!= o){
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


