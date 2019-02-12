uint ticks( void )
{
  //Return the tick counter
  //TODO
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

exception wait( uint nTicks ){
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
    
  } else {//not first execution
    if(deadline()<=0)
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
