#include "tests.h"
//Global mailboxes 
mailbox* mailBoxes[10];
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
  assert(ReadyList->pHead->pNext->pTask->Deadline == UINT_MAX);
  assert(ReadyList->pTail->pPrevious->pTask->Deadline == UINT_MAX);
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
  assert(ReadyList->pHead->pNext->pTask->Deadline == UINT_MAX);
  assert(ReadyList->pTail->pPrevious->pTask->Deadline == UINT_MAX);
  create_task(idle_function,100);
  assert(ReadyList->pHead->pNext->pTask->Deadline == 100);
  assert(ReadyList->pHead->pNext->pNext->pTask->Deadline == UINT_MAX);
  assert(ReadyList->pTail->pPrevious->pPrevious->pTask->Deadline == 100);
  assert(ReadyList->pTail->pPrevious->pTask->Deadline == UINT_MAX);
  
}

void test_create_task_running()//this test ist called from the wrapper 
{
  assert(KernelMode == RUNNING);
  create_task(idle_function,777);
  //assert new Task is in ready list
  assert(ReadyList->pHead->pNext->pNext->pTask->Deadline == 777);
  assert(ReadyList->pTail->pPrevious->pPrevious->pTask->Deadline == 777);
  
}

void stage_one_test_case(){
init_kernel();
create_task(stage_one_test_case_task_wait,1);//task to test wait
create_task(stage_one_test_case_task_one,100); //task to test communication and create task
mailBoxes[0]  = create_mailbox(5, sizeof(int));//wait mailbox
mailBoxes[1]  = create_mailbox(5, sizeof(int));//no wait mailbox
run();


}

void stage_one_test_case_task_one(){
  create_task(stage_one_test_case_task_two,500);//other task to test communication
  //send message no one waiting -> this tasks will wait
  int* data = (int*) malloc(sizeof(int));
  *data = 6;
  exception result = send_wait(mailBoxes[0],data);
  //recive wait first 
  exception returncode =  receive_wait( mailBoxes[0], data );
  assert(*data == 7);
  

}
void stage_one_test_case_task_two()
{
  int* data = (int*) malloc(sizeof(int));
  exception result = receive_wait( mailBoxes[0], data );
  // context swich task one
  assert(*data == 6);
  *data = 7;
  exception returncode = send_wait(mailBoxes[0],data);
  
}

void stage_one_test_case_task_wait()
{
  wait(10000);
}

