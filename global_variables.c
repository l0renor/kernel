int     Ticks;          /* global sys tick counter */
int     KernelMode;     /* can equal either INIT or RUNNING */
TCB*    RunningTask;
list*   ReadyList;
list*   WaitingList;
list*   TimerList;