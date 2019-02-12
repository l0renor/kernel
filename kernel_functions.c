#include "kernel_functions.h"
#include "globalVariables.c"
#include <limits.h>
#include "helper_functions.c"
#include "timing_functions.c"

extern void __save_context(void);
extern void LoadContext(void);
extern void SelectPSP(void);
extern void __get_PSP_from_TCB(void);

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
  
  exception idleTaskException = create_task(idle_function, UINT_MAX); 
  
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
  if (!KernelMode)
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
        remove_from_list(WaitingList, sender->pBlock);
        sorted_insert(ReadyList, sender->pBlock);
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
      listobj* runningTaskObject = ReadyList->pHead->pNext;
      remove_from_list(ReadyList, runningTaskObject);
      sorted_insert(WaitingList, runningTaskObject);
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
      
      
      remove_from_list( WaitingList, m->pBlock);
      sorted_insert( ReadyList, m->pBlock);
      //@TODO swich running task somewhere 
      free(m);
      mBox->nMessages = mBox->nMessages - 1;
      mBox->nBlockedMsg = mBox->nBlockedMsg + 1; //reciver not waiting anymore
      
      
    }
    else
    {
      msg* newM = (msg *)calloc(1,sizeof(msg));
      newM->pData = pData;
      push_mailbox_tail(mBox,newM);
      remove_from_list(ReadyList,ReadyList->pHead->pNext);
      mBox->nBlockedMsg = mBox->nBlockedMsg + 1;//new sender Task waiting 
      if(mBox->nMessages == mBox->nMaxMessages){//mailbox is full
        pop_mailbox_head(mBox);//remove old msg now nMessages is correct again
      }else{
        mBox->nMessages = mBox->nMessages + 1;
      }
    }
    LoadContext();
    
    
  }else//not first excecution
  {
    if(deadline()<=0)
    {
      isr_off();
      remove_running_task_from_mailbox(mBox);
      mBox->nMessages = mBox->nMessages -1;
      isr_on();
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


exception send_no_wait( mailbox* mBox, void* pData )
{
  static bool is_first_execution = TRUE;
  if ( is_first_execution == TRUE )
  {
    is_first_execution = FALSE;
    if(mBox->nBlockedMsg < 0)
    {
      //receiving tasks waiting
      msg* m = pop_mailbox_head(mBox);
      //copy senders data into reciver
      memcpy(m->pData,pData,mBox->nDataSize);
      
      
      remove_from_list( WaitingList, m->pBlock);
      sorted_insert( ReadyList, m->pBlock);
      //todo swich running task somewhe
      
      free(m);
      LoadContext();
      
    }
    else
    {
      msg* newM = (msg *)calloc(1,sizeof(msg));
      newM->pData = pData;
      if(mBox->nMessages == mBox->nMaxMessages)//Box is full
      {
        msg* oldM = pop_mailbox_head(mBox);
        free(oldM);
      }
      push_mailbox_tail(mBox,newM);
    }
    
  }
  return OK;
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
        remove_from_list(WaitingList, sender->pBlock);
        sorted_insert(ReadyList, sender->pBlock);
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


// Own helper functions




