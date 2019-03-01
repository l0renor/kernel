#include "tests.h"

//General Wrapper for all test which also have to be tasks and need noo special pre-executive operations.
void TestWrapper( void (* test_in_running_mode)() )
{
  init_kernel();
  create_task(test_in_running_mode,1);
  run();
  // Continues in test_in_running_mode
}

void test_init_kernel()
{
  assert(init_kernel() == OK);
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
  run();
  //now the test continues in create_task_test_task
}

void create_task_test_task()
{
  assert(KernelMode == RUNNING);
  assert(ReadyList->pHead->pNext->pTask->DeadLine == 1000);
  assert(ReadyList->pTail->pPrevious->pTask->DeadLine == UINT_MAX);
}