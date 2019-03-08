        /*  Unit test to check if terminate() works correctly, and if create_task( )
              (1) correctly initializes TCBs, and,
              (2) correctly updates the ReadyList
            assumes that you have used the name ReadyList to point to the Ready list
            and assumes that you have written all of your kernel functions in a single
            C file called kernel_functions.c
            And of course, init_kernel() and run() also need to work correctly
         */

#include "system_sam3x.h"
#include "at91sam3x8.h"

#include "kernel_functions_march_2019.h"
#include "kernel_functions.c"

unsigned int g0=0, g1=0, g2=0, g3=1;  /* gate flags for various stages of unit test */

unsigned int low_deadline  = 1000;    
unsigned int high_deadline = 100000;

#define create_count_from_main_MAX  10
#define create_count_from_task2_MAX 10


void task_body_1(){
  terminate();
}

void task_body_2(){
  exception           return_value;
  unsigned int        recursion_count_upon_entry;
  static unsigned int recursion_depth_left  = create_count_from_task2_MAX;
  static unsigned int count_recursive_calls = 0;

  count_recursive_calls++;
  recursion_depth_left--;
  if (recursion_depth_left > 0)
  {
    recursion_count_upon_entry = count_recursive_calls;
    return_value = create_task( task_body_2, high_deadline - 10 * count_recursive_calls );
                   /* simply a way of creating tasks with tighter deadlines than
                    * the currenty running task
                    */
    if ( return_value == OK)
    {
      if ( count_recursive_calls <= recursion_count_upon_entry )
      {
        g2 = FAIL;
      }
      else
      {
        g2 = OK;
      }
    }
    else
    {
      g2 = FAIL;
    }
  }
  else
  {
    if (  count_recursive_calls == create_count_from_task2_MAX )
    {
      g2 = OK;
    }
    else
    {
      g2 = FAIL;
    }
  }
  g3 = g3 * g2;
  terminate();
}

void task_body_3(){
  if ( g3 == OK )
  {
    while(1) 
    { 
        /* Alles Gut ! This unit test has been passed */
    }
  }
  else
  {
    while(1)
    {  
      /* failed */
    }
  }
}

void main()
{
  SystemInit(); 
  SysTick_Config(100000); 
  SCB->SHP[((uint32_t)(SysTick_IRQn) & 0xF)-4] =  (0xE0);      
  isr_off();
  
  g0 = OK;
  exception retVal = init_kernel(); 
  if ( retVal != OK ) { g0 = FAIL; while(1) { /* no use going further */  } }
  
  if ( ReadyList->pHead->pNext->pNext != ReadyList->pTail ) { g0 = FAIL ;}
  if ( WaitingList->pHead->pNext != WaitingList->pTail )    { g0 = FAIL ;}
  if (   TimerList->pHead->pNext !=   TimerList->pTail )    { g0 = FAIL ;}
    
  if ( g0 != OK ) { while(1) { /* no use going further */  } }

  unsigned int    i, *sPtr, *lowEndPtr, *highEndPtr ;
  listobj        *nextListObj = ReadyList->pHead;

  g1 = OK;
  for ( i = 1 ; i <= create_count_from_main_MAX ; i++ )
  {
      retVal = create_task( task_body_1 , low_deadline + i);
      if ( retVal == OK )
      { 
        // check whether the SP is pointing inside the stack
        sPtr         =     nextListObj->pNext->pTask->SP;   
        lowEndPtr    =  & ( nextListObj->pNext->pTask->StackSeg[0] ); 
        highEndPtr   =  & ( nextListObj->pNext->pTask->StackSeg[STACK_SIZE -1] ); 
        if (  lowEndPtr >= sPtr  ||  sPtr >= highEndPtr   ) { g1 = FAIL; break;} 
        // check whether the stack frame matches the PC and SPSR fields
        lowEndPtr  = (unsigned int*) ( (unsigned int) sPtr + (unsigned int) 24 );   // where PC  should be stored
        highEndPtr = (unsigned int*) ( (unsigned int) sPtr + (unsigned int) 28 );   // where PSR should be stored
        if ( (unsigned int) nextListObj->pNext->pTask->PC   != (unsigned int) (  *lowEndPtr )   ) { g1 = FAIL; break;} 
        if ( (unsigned int) nextListObj->pNext->pTask->SPSR != (unsigned int) ( *highEndPtr )   ) { g1 = FAIL; break;} 
        if ( (unsigned int) nextListObj->pNext->pTask->PC == (unsigned int) 0 )                   { g1 = FAIL; break;}
        // is the newly created task here ?
        if ( (unsigned int) nextListObj->pNext->pTask->Deadline != (unsigned int) ( low_deadline + i ) ) { g1 = FAIL; break;}
        // advance to create the next task and examine its TCB
        nextListObj = nextListObj->pNext;
      }
      else{ g1 = FAIL; break; }
  }
  if ( g1 !=  OK ) { while(1) { /* no use going further */  } }
  
  
  /* now we shall set up a task to test how create_task works when called during run time */
  g3 = 1; g2 = 0;
  retVal = create_task( task_body_2 , high_deadline );
  if ( retVal !=  OK ) { while(1) { /* no use going further */  } }
  
  retVal = create_task( task_body_3 , 8*high_deadline );
           // should have a priority higher than the idle task, but lower than all else
  if ( retVal !=  OK ) { while(1) { /* no use going further */  } }
  
  run();
  
  while(1){ /* something is wrong with run */}
} 