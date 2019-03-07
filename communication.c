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
  if ( mBox == NULL )
  {
    return NOT_EMPTY;
  }
  
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
  if ( mBox == NULL ||pData == NULL)
  {
    return DEADLINE_REACHED;
  }
  //Disable interrupts
  isr_off();
  //Update PreviousTask
  PreviousTask = getFirstRL();
  //IF receiving task is waiting THEN
  if ( mBox->nBlockedMsg < 0 )
  {
    //Remove receiving task's Message struct from the Mailbox
    msg* receiver = pop_mailbox_head(mBox);
    //Change mailbox stats
    mBox->nMessages--;
    mBox->nBlockedMsg++;
    //Copy sender's data to the data area of the receivers Message
    memcpy(receiver->pData, pData, mBox->nDataSize);
    //Move receiving task to ReadyList
    remove_from_list(WaitingList, receiver->pBlock);
    sorted_insert(ReadyList, receiver->pBlock);
    //Free receivers memory
    free(receiver);
  }
  else
  {
    //IF Deadline is already reached 
    if ( deadline() <= ticks() ) 
    {
      return DEADLINE_REACHED;
    }
    //Allocate a Message structure +  Set data pointer   
    msg* message =  create_message(pData,0);
    if ( message == NULL )
    {
      return DEADLINE_REACHED;
    }
   
    
    //Get listobj of current task    
    listobj* waiter = ReadyList->pHead->pNext;
    //Set blocked listobj pointer and other way round
    message->pBlock = waiter;
    waiter->pMessage = message;
    //Add Message to the mailbox
    push_mailbox_tail(mBox, message);
    //Move sending task from ReadyList to WaitingList
    remove_from_list(ReadyList, waiter);
    sorted_insert(WaitingList, waiter);
    //IF Mailbox is full
    if ( mBox->nMessages == mBox->nMaxMessages )
    {
      //Pop first mailbox message
      msg* head = pop_mailbox_head(mBox);
      //Set nTCnt to 1 as flag
      head->pBlock->nTCnt = 1;
      //Put its task back in ReadyList
      remove_from_list(WaitingList, head->pBlock);
      sorted_insert(ReadyList, head->pBlock);
      //Free memory
      free(head);
    }
    else
    {
      //Change mailbox stats		
      mBox->nMessages++;
      mBox->nBlockedMsg++;
    }
  }
  
  //Update NextTask
  NextTask = getFirstRL();
  
  //Switch context
  if ( PreviousTask == NextTask ) 
  {
    isr_on();
  }
  else
  {
    SwitchContext();
  }
  
  //IF deadline was reached
  if ( deadline() <= ticks() )
  {
    //Disable interrupt
    isr_off();
    //Remove receive Message
    remove_running_task_from_mailbox(mBox);
    //Enable interrupt
    isr_on();
    return DEADLINE_REACHED;
  }
  //ELSE IF message was thrown out of full mailbox
  else if ( ReadyList->pHead->pNext->nTCnt == 1 ) 
  {
    ReadyList->pHead->pNext->nTCnt = 0;
    return DEADLINE_REACHED;
  }
  else
  {
    return OK;
  }
}

exception receive_wait( mailbox* mBox, void* pData )
{
  if ( mBox == NULL ||pData == NULL)
  {
    return DEADLINE_REACHED;
  }
  //Disable interrupt
  isr_off();
  //Update PreviousTask
  PreviousTask = getFirstRL();
  //IF send Message is waiting THEN
  if ( mBox->nMessages > 0 && mBox->nBlockedMsg >= 0 )
  {
    //Remove sending task's Message struct from the Mailbox
    msg* sender = pop_mailbox_head(mBox);
    //Change mailbox stats
    mBox->nMessages--;
    if ( mBox->nBlockedMsg > 0 ) 
    {
      mBox->nBlockedMsg--;
    }
    //Copy sender's data to receiving task's data area
    memcpy(pData, sender->pData, mBox->nDataSize);
    //IF Message was of wait type THEN
    if ( sender->pBlock != NULL )
    {
      //Move sending task to ReadyList
      remove_from_list(WaitingList, sender->pBlock);
      sorted_insert(ReadyList, sender->pBlock);
    }
    else 
    {
      //Free senders data area
      void* p = realloc(sender->pData, mBox->nDataSize);
      free(p);
    }
    //Free senders memory
    free(sender);
  }
  else
  { 
     if ( deadline() <= ticks() ) 
    {
      return DEADLINE_REACHED;
    }
    //Allocate a Message structure +  Set data pointer   
    msg* message =  create_message(pData,0);
    if( message == NULL )
    {
      return DEADLINE_REACHED;
    }
    //Set data pointer
    //Get listobj of current task    
    listobj* waiter = ReadyList->pHead->pNext;
    //Set blocked listobj pointer and other way round
    message->pBlock = waiter;
    waiter->pMessage = message;
    //Add Message to the Mailbox
    push_mailbox_tail(mBox, message);
    //Move receiving task from Readylist to Waitinglist
    remove_from_list(ReadyList, waiter);
    sorted_insert(WaitingList, waiter);
    //IF Mailbox is full
    if ( mBox->nMaxMessages == mBox->nMessages )
    {
      //Pop first mailbox message
      msg* head = pop_mailbox_head(mBox);
      //Set nTCnt to 1 as flag
      head->pBlock->nTCnt = 1;
      //Put its task back in ReadyList
      remove_from_list(WaitingList, head->pBlock);
      sorted_insert(ReadyList, head->pBlock);
      //Free memory
      free(head);
    }
    else
    {
      //Change the mailbox stats
      mBox->nMessages++;
      mBox->nBlockedMsg--;
    }
  }
  
  //Update NextTask
  NextTask = getFirstRL();
  
  //Switch context
  if ( PreviousTask == NextTask )
  {
    isr_on();
  }
  else
  {
    SwitchContext();
  }
  
  //IF deadline was reached
  if ( deadline() <= ticks() )
  {
    //Disable interrupt
    isr_off();
    //Remove receive Message
    remove_running_task_from_mailbox(mBox);
    //Enable interrupt
    isr_on();
    return DEADLINE_REACHED;
  }
  //ELSE IF message was thrown out of full mailbox
  else if ( ReadyList->pHead->pNext->nTCnt == 1 ) 
  {
    ReadyList->pHead->pNext->nTCnt = 0;
    return DEADLINE_REACHED;
  }
  else
  {
    return OK;
  }
}

exception send_no_wait( mailbox* mBox, void* pData )
{
  if ( mBox == NULL ||pData == NULL)
  {
    return DEADLINE_REACHED;
  }
  //Disable interrupt
  isr_off();
  //Update PreviousTask
  PreviousTask = getFirstRL();
  //IF receiving task is waiting THEN
  if( mBox->nBlockedMsg < 0 )
  {
    //Remove receiving task's Message struct from the Mailbox
    msg* receiver = pop_mailbox_head(mBox);
    //Change mailbox stats
    mBox->nMessages--;
    mBox->nBlockedMsg++;
    //Copy sender's data to the data area of the receivers Message
    memcpy(receiver->pData, pData, mBox->nDataSize);
    //Move receiving task to ReadyList
    remove_from_list( WaitingList, receiver->pBlock);
    sorted_insert( ReadyList, receiver->pBlock);
    //Free receivers memory
    free(receiver);
    
    //Update NextTask
    NextTask = getFirstRL();
    
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
    //Allocate a Message structure +  Set data pointer   
    msg* message =  create_message(NULL,0);
    if ( message == NULL )
    {
      return FAIL;
    }
    //Allocate memory for the data
    message->pData = calloc( mBox->nDataSize, sizeof( char ) );
    if ( message->pData == NULL ) 
    {
      return FAIL;
    }
    //Copy sender's data to the data area of the sender Message
    memcpy(message->pData, pData, mBox->nDataSize);
    //Add Message to the mailbox
    push_mailbox_tail(mBox, message);
    //IF Mailbox is full
    if ( mBox->nMessages == mBox->nMaxMessages )
    {
      //Pop first mailbox message
      msg* head = pop_mailbox_head(mBox);
      //Free memory
      free(head);
    }
    else
    {
      //Change mailbox stats
      mBox->nMessages++;
    }
    //Enable interrupts
    isr_on();
  }
  return OK;
}

exception receive_no_wait( mailbox* mBox, void* pData )
{
  if ( mBox == NULL ||pData == NULL)
  {
    return DEADLINE_REACHED;
  }
  exception message_received = FAIL;
  //IF send Message is waiting THEN
  if ( mBox->nMessages > 0 && mBox->nBlockedMsg >= 0 )
  {
    //Disable interrupt
    isr_off();
    
    message_received = OK;
    //Update PreviousTask
    PreviousTask = getFirstRL();
    //Remove sending task's Message struct from the Mailbox
    msg* sender = pop_mailbox_head(mBox);
    //Change mailbox stats
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
      //Move sending task to ReadyList
      remove_from_list(WaitingList, sender->pBlock);
      sorted_insert(ReadyList, sender->pBlock);
    }
    else 
    {
      //Free senders data area
      void* p = realloc(sender->pData, mBox->nDataSize);
      free(p);
    }
    //Free senders memory
    free(sender);
    
    //Update NextTask
    NextTask = getFirstRL();
    
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