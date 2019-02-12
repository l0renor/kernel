// Communication

mailbox* create_mailbox( uint nMessages, uint nDataSize)
{
  isr_off(); /* protection of calloc */
  mailbox *result = (mailbox*)calloc(1,sizeof(mailbox));
  isr_on();
  if ( result != NULL )
  {
    result->nDataSize = nDataSize;
    result->nMessages = nMessages;
    result->nBlockedMsg = 0;
    result->pHead = create_message(NULL,0);
    result->pTail = create_message(NULL,0);
    result->pHead->pNext = result->pTail;
    result->pTail->pPrevious = result->pHead;
  }
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
    
    
  }
  else 
  {
    if( deadline() - ticks() <= 0 )
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
    if (deadline() - ticks() <= 0)
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
  static exception message_received = FAIL;
  //IF first execution THEN
  if ( is_first_execution )
  {
    //Set: "not first execution any more"
    is_first_execution = FALSE;
    
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