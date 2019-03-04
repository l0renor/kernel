exception wait( uint nTicks )
{
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
  
  if ( deadline() - ticks() <= 0 )
  {
    status = DEADLINE_REACHED;
  }
  else
  {
    status = OK;
  }
  return status;
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
  return  getFirstRL()->DeadLine;
}

void set_deadline( uint deadline )
{
  //Disable interrupt
  isr_off();
  //Set the deadline field in the calling TCB.
  getFirstRL()->DeadLine = deadline;
  PreviousTask = getFirstRL();
  //Reschedule Readylist
  insertion_sort(ReadyList);
  //Update NextTask
  NextTask = getFirstRL();
  //Switch context
  SwitchContext();
}

void TimerInt( void )
{
  Ticks++;
  
  // check TimerList
  listobj* current = TimerList->pHead->pNext;
  while ( current->pTask != NULL )
  {
    // IF element still has to wait
    if ( current->nTCnt > 1 )
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
  
  // check WaitingList
  current = WaitingList->pHead->pNext;
  while ( current->pTask != NULL )
  {
    // IF Deadline is reached 
    if( current->pTask->Deadline <= ticks() )
    {
      listobj* toMove = current;
      current = toMove->pNext;
      remove_from_list(WaitingList, toMove);      
      //      msg* previous = current->pMessage->pPrevious;
      //      msg* next = current->pMessage->pNext;
      //      next->pPrevious = previous;
      //      previous->pNext = next;
      //      free(current->pMessage);
      sorted_insert(ReadyList, current);
    }
  }
  schedule();
}


