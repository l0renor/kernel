#include "kernel_functions.h"

extern void __save_context(void);
extern void LoadContext(void);
extern void SelectPSP(void);
extern void __get_PSP_from_TCB(void);

void run(void);

void Task1(void);
void Task2(void);

TCB *ptr_tcbForTaskOne, *ptr_tcbForTaskTwo, *RunningTask;

// void (*taskPointer)(void);

void TimerInt(void);

void Task1(void)
{
    unsigned int j;
    j = 0;

    while(1)
    {
        j++;
        j--;
        //SaveContext();
    }
}

void Task2(void)
{
    unsigned int k;
    k = 0;
    while(1)
    {
        k++;
        //SaveContext();
        k--;
    }
}

void run(void)
{
  RunningTask = ptr_tcbForTaskOne;
  LoadContext();
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
  if ( ticks % 2 == 0 )
  {  
    RunningTask = ptr_tcbForTaskOne; 
  }
  else
  {  
    RunningTask = ptr_tcbForTaskTwo; 
  }
}
