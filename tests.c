#include "tests.h"

void test_init_kernel()
{
  init_kernel();
  assert(ReadyList != NULL);
  assert(WaitingList != NULL);
  assert(TimerList != NULL);
  assert(KernelMode == INIT);
  //readyListTests
  //Idle task
  assert(ReadyList->pHead->pNext->pTask->DeadLine == UINT_MAX);
  assert(ReadyList->pTail->pPrevious->pTask->DeadLine == UINT_MAX);
  assert(ReadyList->pHead->pNext->pNext == ReadyList->pTail);
  assert(ReadyList->pHead->pNext->pPrevious == ReadyList->pHead);
  assert(Ticks == 0);
  //head
   assert(ReadyList->pHead->pPrevious == NULL);
   //tail
   assert(ReadyList->pTail->pNext == NULL);
}

void test_create_task_init()
{
  init_kernel();
  assert(KernelMode == INIT);
  assert(ReadyList->pHead->pNext->pTask->DeadLine == UINT_MAX);
  assert(ReadyList->pTail->pPrevious->pTask->DeadLine == UINT_MAX);
  create_task(idle_function,100);
  assert(ReadyList->pHead->pNext->pTask->DeadLine == 100);
  assert(ReadyList->pHead->pNext->pNext->pTask->DeadLine == 100);
  assert(ReadyList->pTail->pPrevious->pPrevious->pTask->DeadLine == 100);
  assert(ReadyList->pTail->pPrevious->pTask->DeadLine == UINT_MAX);
  
}

void test_create_task_running()
{
  init_kernel();
  assert(KernelMode == INIT);
  assert(ReadyList->pHead->pNext->pTask->DeadLine == UINT_MAX);
  assert(ReadyList->pTail->pPrevious->pTask->DeadLine == UINT_MAX);
  create_task(create_task_test_task,1000);
  run();
  //now the test continues in create_task_test_task
}

void create_task_test_task()
{
  assert(KernelMode == RUNNING);
  assert(ReadyList->pHead->pNext->pTask->DeadLine == 1000);
  assert(ReadyList->pTail->pPrevious->pTask->DeadLine == UINT_MAX);
}