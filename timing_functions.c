exception wait( uint nTicks )
{
  isr_off();
  SaveContext();
  exception status;
  static bool is_first_execution = TRUE;
  if ( is_first_execution == TRUE )
  {
    is_first_execution = FALSE;
    listobj* running = ReadyList->pHead->pNext;
    remove_from_list(ReadyList,running);
    sorted_insert(TimerList,running);
    LoadContext();
    
  } 
  else 
  {
    if ( deadline() <= 0 )
    {
      isr_off();
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
    insertion_sort(ReadyList);
    //Load context
    LoadContext();
  }
}

void TimerInt(void)
{
  Ticks++;
  //check timerlist
  listobj* current = TimerList->pHead->pNext;
  while(current->pTask!=NULL)
  {
    if(current->pTask->DeadLine - ticks() <= 0){//deadline reached 
      remove_from_list( TimerList,current);
      sorted_insert(ReadyList,current);
      current=current->pNext;
    }
    
  }
 //check WaitingList
  current = WaitingList->pHead->pNext;
  while(current->pTask!=NULL)
  {
    if(current->pTask->DeadLine - ticks() <= 0){//deadline reached 
      remove_from_list(WaitingList,current);
      sorted_insert(ReadyList,current);
      current=current->pNext;
      //TODO clean up mailbox
    }
    
  }
}


