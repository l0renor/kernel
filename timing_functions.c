exception wait( uint nTicks )
{
  isr_off();
  SaveContext();
  exception status;
  static volatile bool is_first_execution = TRUE;
  if ( is_first_execution )
  {
    is_first_execution = FALSE;
    listobj* running = ReadyList->pHead->pNext;
    remove_from_list(ReadyList,running);
    running->nTCnt = nTicks;
    sorted_insert(TimerList,running);
    schedule();
    LoadContext();
  } 
  else 
  {
    if ( deadline() - ticks() <= 0 )
    {
      status = DEADLINE_REACHED;
    }
    else
    {
      status = OK;
    }
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
  return RunningTask->DeadLine;
}

void set_deadline( uint deadline )
{
  static volatile bool is_first_execution = TRUE;
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
    schedule();
    LoadContext();
  }
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
    if( current->pTask->DeadLine <= ticks() )
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


