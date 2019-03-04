uint    Ticks;          /* global sys tick counter */
uint    KernelMode;     /* can equal either INIT or RUNNING */
TCB*    PreviousTask;
TCB*    NextTask;
list*   ReadyList;
list*   WaitingList;
list*   TimerList;