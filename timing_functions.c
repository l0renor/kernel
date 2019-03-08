exception wait( uint nTicks )
{
  //IF deadline already reached no need to sleep
  if ( deadline() - ticks() <= 0 )
  {
    return DEADLINE_REACHED;
  }
  isr_off();
  exception status;
  //Update PreviousTask
  PreviousTask = getFirstRL();
  //Place RunningTask in the TimerList
  listobj* running = ReadyList->pHead->pNext;
  remove_from_list(ReadyList,running);
  running->nTCnt = nTicks;
  sorted_insert(TimerList,running);
  //Update NextTask
  NextTask = getFirstRL();
  //Switch context
  SwitchContext();
  
  //Check status
  if ( deadline() - ticks() <= 0 )
  {
    return DEADLINE_REACHED;
  }
  else
  {
    return OK;
  }
}

void set_ticks( uint nTicks )
{
  //Set the tick counter
  Ticks = nTicks;
}

uint ticks( void )
{
  //Return the tick counter
  return Ticks;
}

uint deadline( void )
{
  //Return the deadline of the current task
  return  getFirstRL()->Deadline;
}

void set_deadline( uint deadline )
{
  //Disable interrupt
  isr_off();
  //Update PreviousTask
  PreviousTask = getFirstRL();
  
  //Set the deadline field in the calling TCB.
  getFirstRL()->Deadline = deadline;
  //Reschedule Readylist
  insertion_sort(ReadyList);
  
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

void TimerInt( void )
{
  Ticks++;
  //Update PreviousTask
  PreviousTask = getFirstRL();
  
  //Check TimerList
  listobj* current = TimerList->pHead->pNext;
  while ( current->pTask != NULL )
  {
    // IF element still has to wait
    if ( current->nTCnt > 1 && current->pTask->Deadline > ticks() )
    {
      current->nTCnt--;
      current=current->pNext;
    }
    else 
    {
      listobj* toMove = current;
      current = toMove->pNext;
      remove_from_list(TimerList, toMove);
      toMove->nTCnt = 0;
      sorted_insert(ReadyList, toMove);
    }
  }
  
  //Check WaitingList
  current = WaitingList->pHead->pNext;
  while ( current->pTask != NULL )
  {
    // IF Deadline is reached 
    if( current->pTask->Deadline <= ticks() )
    {
      listobj* toMove = current;
      current = toMove->pNext;
      remove_from_list(WaitingList, toMove);
      sorted_insert(ReadyList, toMove);
    }
    else
    {
      current = current->pNext;
    }
  }
  
  //Update NextTask
  NextTask = getFirstRL();
}


