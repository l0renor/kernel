// Communication

mailbox* create_mailbox( uint nMessages, uint nDataSize)
{
  isr_off(); /* protection of calloc */
  mailbox *result = (mailbox*)calloc(1,sizeof(mailbox));
  isr_on();
  if ( result != NULL )
  {
    result->nDataSize = nDataSize;
    result->nMaxMessages = nMessages;
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
  //Disable interrupts
  isr_off();
  if(mBox->nBlockedMsg < 0)
  {
    //receiving tasks waiting
    msg* m = pop_mailbox_head(mBox);
    //copy senders data into reciver
    memcpy(m->pData,pData,mBox->nDataSize);
    remove_from_list( WaitingList, m->pBlock);
    PreviousTask = getFirstRL();
    sorted_insert( ReadyList, m->pBlock);
    NextTask = getFirstRL();

    free(m);
    mBox->nMessages--;
    mBox->nBlockedMsg++; //reciver not waiting anymore
  }
  else
  {
    // sending task will wait
    msg* newM = (msg *)calloc(1,sizeof(msg));
    if(newM == NULL){
      return DEADLINE_REACHED;
    }
    newM->pData = pData;
    newM->pBlock = ReadyList->pHead->pNext;
    push_mailbox_tail(mBox,newM);
    ReadyList->pHead->pNext->pMessage = newM;
    PreviousTask = ReadyList->pHead->pNext->pTask;
    listobj* toMove = ReadyList->pHead->pNext;
    remove_from_list(ReadyList,toMove);
    sorted_insert( WaitingList, toMove);
    NextTask = ReadyList->pHead->pNext->pTask;
    mBox->nBlockedMsg = mBox->nBlockedMsg + 1;//new sender Task waiting 
    if(mBox->nMessages == mBox->nMaxMessages)
    {//mailbox is full
      pop_mailbox_head(mBox);//remove old msg now nMessages is correct again
    }
    else
    {
      mBox->nMessages++;
    }
  }
  
  //Switch context
  if ( PreviousTask == NextTask ) 
  {
    isr_on();
  }
  else
  {
    SwitchContext();
  }
  
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
    return OK;
  }
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
    if (mBox->nBlockedMsg > 0) 
    {
      mBox->nBlockedMsg--;
    }
    //Copy sender's data to receiving task's data area
    memcpy(pData, sender->pData, mBox->nDataSize);
    //IF Message was of wait type THEN
    if ( sender->pBlock != NULL )
    {
      //Update PreviousTask
      PreviousTask = getFirstRL();
      //Move sending task to ReadyList
      remove_from_list(WaitingList, sender->pBlock);
      sorted_insert(ReadyList, sender->pBlock);
      //Update NextTask
      NextTask = getFirstRL();
    }
    else 
    {
      void* p = realloc(sender->pData, mBox->nDataSize);
      free(p);
      PreviousTask = NextTask = getFirstRL();
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
    message->pData = pData;
    //Add Message to the Mailbox
    message->pBlock = ReadyList->pHead->pNext; 
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
      mBox->nBlockedMsg--;
    }
    ReadyList->pHead->pNext->pMessage = message;
    //Update PreviousTask
    PreviousTask = getFirstRL();
    //Move receiving task from Readylist to Waitinglist
    listobj* runningTaskObject = ReadyList->pHead->pNext;
    remove_from_list(ReadyList, runningTaskObject);
    sorted_insert(WaitingList, runningTaskObject);
    //Update NextTask
    NextTask = getFirstRL();
  }
  
  //Switch context
  if ( PreviousTask == NextTask ) 
  {
    isr_on();
  }
  else
  {
    SwitchContext();
  }
  
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
}

exception send_no_wait( mailbox* mBox, void* pData )
{
  //Disable interrupt
  isr_off();
  if(mBox->nBlockedMsg < 0)
  {
    //receiving tasks waiting
    msg* m = pop_mailbox_head(mBox);
    //copy senders data into reciver
    
    memcpy(m->pData,pData,mBox->nDataSize);
    
    //Update PreviousTask
    PreviousTask = getFirstRL();
    //Move receiving task to ReadyList
    remove_from_list( WaitingList, m->pBlock);
    sorted_insert( ReadyList, m->pBlock);
    //Update NextTask
    NextTask = getFirstRL();
    
    //Freeing the memory space of the old message
    free(m);

    //Switch context
    if ( NextTask == PreviousTask )
    {
      isr_on();
    }
    else
    {
      SwitchContext();
    }
  }
  else
  {
    msg* newM = (msg *)calloc(1,sizeof(msg));
    if ( newM == NULL ) 
    {
      return FAIL;
    }
    newM->pData = calloc(mBox->nDataSize,sizeof(char));
    if ( newM->pData == NULL ) 
    {
      return FAIL;
    }
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
  //Disable interrupt
  isr_off();
  exception message_received = FAIL;
  //IF send Message is waiting THEN
  if ( mBox->nMessages > 0 && mBox->nBlockedMsg >= 0 )
  {
    message_received = OK;
    msg* sender = pop_mailbox_head(mBox);
    mBox->nMessages--;
    if (mBox->nBlockedMsg > 0) 
    {
      mBox->nBlockedMsg--;
    }
    //Copy sender's data to receiving task's data area
    memcpy(pData, sender->pData, mBox->nDataSize);
    //IF Message was of wait type THEN
    if ( sender->pBlock != NULL )
    {
      //Update PreviousTask
      PreviousTask = getFirstRL();
      //Move sending task to ReadyList
      remove_from_list(WaitingList, sender->pBlock);
      sorted_insert(ReadyList, sender->pBlock);
      //Update NextTask
      NextTask = getFirstRL();
    }
    else 
    {
      void* p = realloc(sender->pData, mBox->nDataSize);
      free(p);
      NextTask = PreviousTask = getFirstRL();
    }
    
    free(sender);
    
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
  //Return status on received message
  return message_received;
}