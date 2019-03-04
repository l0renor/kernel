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
  static volatile bool is_first_execution = TRUE;
  //Disable interrupts
  isr_off();
  //Save context
  SaveContext();
  if(mBox->nBlockedMsg < 0)
  {
    //receiving tasks waiting
    msg* m = pop_mailbox_head(mBox);
    //copy senders data into reciver
    memcpy(m->pData,pData,mBox->nDataSize);
    remove_from_list( WaitingList, m->pBlock);
    PreviousTask = ReadyList->pHead->pNext->pTask;
    sorted_insert( ReadyList, m->pBlock);
    NextTask = ReadyList->pHead->pNext->pTask;

    free(m);
    mBox->nMessages = mBox->nMessages - 1;
    mBox->nBlockedMsg = mBox->nBlockedMsg + 1; //reciver not waiting anymore
  }
  else
  {
    msg* newM = (msg *)calloc(1,sizeof(msg));
    if(newM == NULL){
      return DEADLINE_REACHED;
    }
    newM->pData = pData;
    push_mailbox_tail(mBox,newM);
    ReadyList->pHead->pNext->pMessage = newM;
    PreviousTask = ReadyList->pHead->pNext->pTask;
    listobj* toMove = ReadyList->pHead->pNext;
    remove_from_list(ReadyList,toMove);
    sorted_insert( WaitingList, toMove);
    NextTask = ReadyList->pHead->pNext->pTask;
    mBox->nBlockedMsg = mBox->nBlockedMsg + 1;//new sender Task waiting 
    if(mBox->nMessages == mBox->nMaxMessages){//mailbox is full
      pop_mailbox_head(mBox);//remove old msg now nMessages is correct again
    }else{
      mBox->nMessages = mBox->nMessages + 1;
    }
  }
  SwitchContext();
  
  
  
  if(deadline() <= ticks() )
  {
    isr_off();
    remove_running_task_from_mailbox(mBox);
    mBox->nMessages = mBox->nMessages - 1;
    isr_on();
    return DEADLINE_REACHED;
  }
  else
  {
    //IS pepsi 
    return OK;
  }
  
  return DEADLINE_REACHED;//make compiler happy
}

exception receive_wait( mailbox* mBox, void* pData )
{
  //Disable interrupt
  isr_off();
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
      PreviousTask = getFirstRL();
      remove_from_list(WaitingList, sender->pBlock);
      sorted_insert(ReadyList, sender->pBlock);
      NextTask = getFirstRL();
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
    if(message == NULL){
      return DEADLINE_REACHED;
    }
    //Add Message to the Mailbox
    push_mailbox_tail(mBox, message);
    //IF Mailbox is full
    if ( mBox->nMaxMessages == mBox->nMessages )
    {
      msg* head = pop_mailbox_head(mBox);
      remove_from_list(WaitingList, head->pBlock);
      head->pBlock->nTCnt = 1;
      sorted_insert(ReadyList, head->pBlock);
      free(head);
    }
    else
    {
      mBox->nMessages++;
    }
    ReadyList->pHead->pNext->pMessage = message;
    //Move receiving task from Readylist to Waitinglist
    listobj* runningTaskObject = ReadyList->pHead->pNext;
    remove_from_list(ReadyList, runningTaskObject);
    sorted_insert(WaitingList, runningTaskObject);
  }
  SwitchContext();
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
  else if ( ReadyList->pHead->pNext->nTCnt == 1 ) 
  {
    //FLAG IF TASK WAS THROWN OUT OF FULL MAILBOX 
    return DEADLINE_REACHED;
  }
  else
  {
    //Return OK
    return OK;
  }
  return DEADLINE_REACHED;//make compiler happy
}

exception send_no_wait( mailbox* mBox, void* pData )
{
    if(mBox->nBlockedMsg < 0)
    {
      //receiving tasks waiting
      msg* m = pop_mailbox_head(mBox);
      //copy senders data into reciver
      
      memcpy(m->pData,pData,mBox->nDataSize);
      
      PreviousTask = getFirstRL();
      remove_from_list( WaitingList, m->pBlock); //move task
      sorted_insert( ReadyList, m->pBlock);
      free(m); //delete old msg
      NextTask = getFirstRL();
      SwitchContext();  
    
      
    }
    else
    {
      msg* newM = (msg *)calloc(1,sizeof(msg));
      newM->pData = calloc(mBox->nDataSize,sizeof(char));
      memcpy(newM->pData,pData,mBox->nDataSize);
      if(mBox->nMessages == mBox->nMaxMessages)//Box is full
      {
        msg* oldM = pop_mailbox_head(mBox);
        free(oldM);
      }
      push_mailbox_tail(mBox,newM);
    }
    
  return OK;
}

exception receive_no_wait( mailbox* mBox, void* pData )
{
  static volatile bool is_first_execution = TRUE;
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
    schedule();
    LoadContext();
  }
  //Return status on received message
  return message_received;
}