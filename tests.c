#include "tests.h"
#include <string.h>

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
void test_sort(){
  init_kernel();
  create_task(idle_function,100);
  create_task(idle_function,200);
  create_task(idle_function,300);
  ReadyList->pHead->pNext->pTask->Deadline = 500;
  insertion_sort(ReadyList);
  
  
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
  terminate();
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
  terminate();
  
}

void stage_one_test_case_task_two()
{
  int* data = (int*) malloc(sizeof(int));
  exception result = receive_wait( mailBoxes[0], data );
  // context swich task one
  assert(*data == 6);
  *data = 7;
  exception returncode = send_wait(mailBoxes[0],data);
  terminate();
}

void test_create_remove_mailbox(){
  init_kernel();
  create_task(test_create_remove_mailbox_task,200);
  create_task(test_create_remove_mailbox_task2,500);
  mailBoxes[0]  = create_mailbox(5, sizeof(int));//wait mailbox
  mailBoxes[1]  = create_mailbox(5, sizeof(int));//no wait mailbox
  run();

}
void test_create_remove_mailbox_task(){
  int* data = (int*) malloc(sizeof(int));
  *data = 5;
  assert(remove_mailbox(mailBoxes[0])==OK);
  assert(send_wait(mailBoxes[1],data)== OK);
  terminate();
}
void test_create_remove_mailbox_task2(){
  int* data = (int*) malloc(sizeof(int));
  assert(remove_mailbox(mailBoxes[1])==NOT_EMPTY);
  assert(receive_no_wait(mailBoxes[1],data) == OK);
  assert(*data == 5);
  assert(remove_mailbox(mailBoxes[1])==OK);
  terminate();
}

void test_messaging_basic()
{
  init_kernel();
  create_task(test_messaging_basic_1,1000);
  create_task(test_messaging_basic_2,1500);
  mailBoxes[0] = create_mailbox(1, sizeof(int));
  run();
}

void test_messaging_basic_1()
{
  int* data = (int*) malloc(sizeof(int));
  
  // STEP 1: send_wait -> receive_wait
  *data = 1;
  exception result = send_wait(mailBoxes[0],data);
  assert(result == OK);
  
  // STEP 2: receive_wait -> send_wait
  result =  receive_wait( mailBoxes[0], data );
  assert(result == OK);
  assert(*data == 2);
  
  // STEP 3: send_no_wait -> receive_no_wait
  *data = 3;
  result = send_no_wait(mailBoxes[0],data);
  assert(result == OK);
  set_deadline(deadline()+1000);
  
  // STEP 4: receive_no_wait -> send_no_wait => FAILURE
  result = receive_no_wait(mailBoxes[0],data);
  assert(result == FAIL);
  assert(*data == 3);
  set_deadline(deadline()+1000);
  
  // Receive to clear mBox
  receive_no_wait(mailBoxes[0],data);
  assert(*data == 4);
  
  // STEP 5: send_wait -> receive_no_wait
  *data = 5;
  result = send_wait(mailBoxes[0],data);
  assert(result == OK);
  
  // STEP 6: receive_no_wait -> send_wait => FAILURE
  result = receive_no_wait(mailBoxes[0],data);
  assert(result == FAIL);
  assert(*data == 5);
  set_deadline(deadline()+1000);
  
  // Receive to clear mBox
  receive_no_wait(mailBoxes[0],data);
  assert(*data == 6);
  
  // STEP 7: send_no_wait -> receive_wait
  *data = 7;
  result = send_no_wait(mailBoxes[0],data);
  assert(result == OK);
  set_deadline(deadline()+1000);
  
  // STEP 8: receive_wait -> send_no_wait
  result = receive_wait(mailBoxes[0],data);
  assert(result == OK);
  assert(*data == 8);
  
  // TERMINATION AFTER ALL TESTS
  terminate();
}

void test_messaging_basic_2()
{ 
  int* data = (int*) malloc(sizeof(int));
  
  // STEP 1: send_wait -> receive_wait
  exception result = receive_wait( mailBoxes[0], data );
  assert(result == OK);
  assert(*data == 1);
  
  // STEP 2: receive_wait -> send_wait
  *data = 2;
  result = send_wait(mailBoxes[0],data); 
  assert(result == OK);
  
  // STEP 3: send_no_wait -> receive_no_wait
  result = receive_no_wait(mailBoxes[0],data);
  assert(result == OK);
  assert(*data == 3);
  set_deadline(deadline()+1000);
  
  // STEP 4: receive_no_wait -> send_no_wait => FAILURE
  *data = 4;
  result = send_no_wait(mailBoxes[0],data);
  assert(result == OK);
  set_deadline(deadline()+1000);
  
  // STEP 5: send_wait -> receive_no_wait
  result = receive_no_wait(mailBoxes[0],data);
  assert(result == OK);
  assert(*data = 5);
  
  // STEP 6: receive_no_wait -> send_wait => FAILURE
  *data = 6;
  result = send_wait(mailBoxes[0],data);
  assert(result == OK);
  set_deadline(deadline()+1000);
  
  // STEP 7: send_no_wait -> receive_wait
  result = receive_wait(mailBoxes[0],data);
  assert(result == OK);
  assert(*data == 7);
  
  // STEP 8: receive_wait -> send_no_wait
  *data = 8;
  result = send_no_wait(mailBoxes[0],data);
  assert(result == OK);
  set_deadline(deadline()+1000);
  
  // TERMINATION
  terminate();
}

void stage_one_test_case_task_wait()
{
  wait(500);
  terminate();
}

void test_messaging_exceptions(){
  init_kernel();
  create_task(test_1_messaging_exceptions,1000);
  create_task(test_2_messaging_exceptions,1500);
  //create_task(test_3_messaging_exceptions,2000);
  mailBoxes[0] = create_mailbox(1, sizeof(int)); //size 1 mBox
  //mailBoxes[0] = create_mailbox(6, sizeof(int)); //size 6 mBox
  run();
}  

void test_1_messaging_exceptions(){
  int* data = (int*) malloc(sizeof(int));
  //STEP 1: 2 tasks send_wait in mailbox size 1
  *data = 1;
  exception result = send_wait(mailBoxes[0],data); 
  int* mailData = (int*)mailBoxes[0]->pHead->pNext->pData;
  assert(*mailData == 2);
  assert(mailBoxes[0]->nMessages == 1);
  assert(mailBoxes[0]->nBlockedMsg == 1);
  
  terminate();
}

void test_2_messaging_exceptions()
{
  int* data = (int*) malloc(sizeof(int));
  *data = 2;
  int* mailData = (int*)mailBoxes[0]->pHead->pNext->pData;
  assert(*mailData == 1);
  assert(mailBoxes[0]->nMessages == 1);
  assert(mailBoxes[0]->nBlockedMsg == 1);
  exception result = send_wait(mailBoxes[0],data);  
  terminate();
}


void test_messaging_exceptionsRcv(){
  init_kernel();
  create_task(test_1_messaging_exceptionsRcv,1000);
  create_task(test_2_messaging_exceptionsRcv,1500);
  mailBoxes[0] = create_mailbox(1, sizeof(int)); //size 1 mBox
  //mailBoxes[0] = create_mailbox(6, sizeof(int)); //size 6 mBox
  run();
}  

void test_1_messaging_exceptionsRcv(){
  int* data = (int*) malloc(sizeof(int));
  //STEP 1: 2 tasks send_wait in mailbox size 1
  *data = 1;
  exception result = receive_wait(mailBoxes[0],data); 
  assert(mailBoxes[0]->nMessages == 1);
  assert(mailBoxes[0]->nBlockedMsg == -1);
  
  terminate();
}

void test_2_messaging_exceptionsRcv()
{
  int* data = (int*) malloc(sizeof(int));
  assert(mailBoxes[0]->nMessages == 1);
  assert(mailBoxes[0]->nBlockedMsg == -1);  
  exception result = receive_wait(mailBoxes[0],data); 
  
  terminate();
}

void test_messaging_exceptions_snd_nW(){
  init_kernel();
  create_task(test_1_messaging_exceptions_snd_nW,1000);
  mailBoxes[0] = create_mailbox(3, sizeof(int)); //size 3 mBox
  run();

}
void test_1_messaging_exceptions_snd_nW(){
   int* data = (int*) malloc(sizeof(int));
  //first msg 
  *data = 1;
  exception result = send_no_wait(mailBoxes[0],data);
  int* mailData = (int*)mailBoxes[0]->pHead->pNext->pData;
  assert(*mailData==1);
  assert(mailBoxes[0]->nBlockedMsg == 0);
  assert(mailBoxes[0]->nMessages == 1);
  //second msg
  *data = 2;
  result = send_no_wait(mailBoxes[0],data);
  mailData = (int*)mailBoxes[0]->pHead->pNext->pNext->pData;
  assert(*mailData==2);
  assert(mailBoxes[0]->nBlockedMsg == 0);
  assert(mailBoxes[0]->nMessages == 2);
  //third msg
  *data = 3;
  result = send_no_wait(mailBoxes[0],data);
  mailData = (int*)mailBoxes[0]->pHead->pNext->pNext->pNext->pData;
  assert(*mailData==3);
  assert(mailBoxes[0]->nBlockedMsg == 0);
  assert(mailBoxes[0]->nMessages == 3);
  //forth  msg -> throw out
  *data = 4;
  result = send_no_wait(mailBoxes[0],data);
  mailData = (int*)mailBoxes[0]->pHead->pNext->pNext->pNext->pData;
  assert(*mailData==4);
  mailData = (int*)mailBoxes[0]->pHead->pNext->pNext->pData;
  assert(*mailData==3);
  mailData = (int*)mailBoxes[0]->pHead->pNext->pData;
  assert(*mailData==2);
  assert(mailBoxes[0]->nBlockedMsg == 0);
  assert(mailBoxes[0]->nMessages == 3);
  terminate();
  


}

void test_messaging_exceptions_full_sw()
{
  init_kernel();
  create_task(test_1_messaging_exceptions_full_sw,1000);
  create_task(test_2_messaging_exceptions_full_sw,1000);
  create_task(test_3_messaging_exceptions_full_sw,1000);
  create_task(test_4_messaging_exceptions_full_sw,1000);
  mailBoxes[0] = create_mailbox(3, sizeof(int)); //size 3 mBox
  run();
}

void test_1_messaging_exceptions_full_sw()
{
  int* data = (int*) malloc(sizeof(int));
  *data = 1;
  assert(mailBoxes[0]->nBlockedMsg == 0);
  assert(mailBoxes[0]->nMessages == 0);
  exception result = send_wait(mailBoxes[0],data);
  //this task wakes up because his message was thrown out
  assert(result == FAIL);//this message was thrown out 
  assert(mailBoxes[0]->nBlockedMsg == 3);
  assert(mailBoxes[0]->nMessages == 3);
  receive_no_wait(mailBoxes[0],data);
  assert(mailBoxes[0]->nBlockedMsg == 2);
  assert(mailBoxes[0]->nMessages == 2);
  assert(*data==2);
  terminate();
}

void test_2_messaging_exceptions_full_sw()
{
  int* data = (int*) malloc(sizeof(int));
  *data = 2;
  assert(mailBoxes[0]->nBlockedMsg == 1);
  assert(mailBoxes[0]->nMessages == 1);
  exception result = send_wait(mailBoxes[0],data);
  //this task wakes up because his message was recived
  assert(result == OK);
  assert(mailBoxes[0]->nBlockedMsg == 2);
  assert(mailBoxes[0]->nMessages == 2);
  assert(receive_wait(mailBoxes[0],data)==OK);
  assert(mailBoxes[0]->nBlockedMsg == 1);
  assert(mailBoxes[0]->nMessages == 1);
  assert(*data==3);
  terminate();
}

void test_3_messaging_exceptions_full_sw()
{
  assert(mailBoxes[0]->nBlockedMsg == 2);
  assert(mailBoxes[0]->nMessages == 2);
  int* data = (int*) malloc(sizeof(int));
  *data = 3;
  exception result = send_wait(mailBoxes[0],data);
  //this task wakes up because his message was recived
  assert(result == OK);
  assert(mailBoxes[0]->nBlockedMsg == 1);
  assert(mailBoxes[0]->nMessages == 1);
  assert(receive_wait(mailBoxes[0],data)==OK);
  assert(mailBoxes[0]->nBlockedMsg == 0);
  assert(mailBoxes[0]->nMessages == 0);
  assert(*data==4);
  terminate();
  
}

void test_4_messaging_exceptions_full_sw()
{
  assert(mailBoxes[0]->nBlockedMsg == 3);
  assert(mailBoxes[0]->nMessages == 3);
  int* data = (int*) malloc(sizeof(int));
  *data = 4;
  exception result = send_wait(mailBoxes[0],data);
  assert(mailBoxes[0]->nBlockedMsg == 0);
  assert(mailBoxes[0]->nMessages == 0);
  terminate();
}

void test_mass_msg(){
  init_kernel();
  create_task(test_mass_msg_send,10);
  create_task(test_mass_msg_rcv,15);
  mailBoxes[0] = create_mailbox(1, 592*sizeof(char)); //size 1 mBox for text stringlengh + 1
  run();

}

void test_mass_msg_send(){
  char *data = (char*) malloc(592*sizeof(char)); //stringlengh + 1
  strcpy(data, "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.");
  while(1){
  exception result = send_wait(mailBoxes[0],data);
  assert(result == OK);
  }

}

void test_mass_msg_rcv(){
  char *data = (char*) malloc(592*sizeof(char)); //stringlengh + 1
  while(1){
  strcpy(data, "Not Lorem ipsum");  
  exception result = receive_no_wait(mailBoxes[0],data);
  assert(result == OK);
  assert(strcmp(data, "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.") == 0);
  }
}

void test_infinite_tasks_init()
{
  init_kernel();
  while (1)
  {
    create_task(idle_function,100);
  }
}

void test_infinite_tasks_running(){
  
  init_kernel();
  assert(OK == create_task(test_infinite_tasks_running_task,100));
  run();

}

void test_infinite_tasks_running_task()
{
  while (1)
  {
    create_task(idle_function,100);
  }
  terminate();
}


void test_infinite_tasks_create_terminate()
{
  init_kernel();
  assert(OK == create_task(test_infinite_tasks_create_terminate_task_cre,2000));//high DL
  run(); 
}
void test_infinite_tasks_create_terminate_task_cre()
{
  while(1)
  {
    create_task(test_infinite_tasks_create_terminate_task_ter,100); // low DL
  }
  terminate();  
}

void test_infinite_tasks_create_terminate_task_ter()
{
  terminate();
}
 
test_send_wait_deadlinereached(){


}





