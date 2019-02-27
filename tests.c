void init_kernel_test(){
  init_kernel();
  assert(ReadyList != NULL);
  assert(WaitingList != NULL);
  assert(TimerList != NULL);
  assert(KernelMode == INIT);
  //readyListTests
  //Idle task
  assert(ReadyList->pHead-pNext->pTask->DeadLine == UINT_MAX);
  assert(ReadyList->pHead-pNext->pNext-> == ReadyList->pTail);
  assert(ReadyList->pHead-pNext->pPrevious-> == ReadyList->pHead);
  //head
   assert(ReadyList->pHead-pNext->pPrevious-> == ReadyList->pHead);
}