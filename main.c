
#include "system_sam3x.h"
#include "at91sam3x8.h"

#include "kernel_functions.c"
#include "tests.c"

void main(void)
{
  SystemInit();
  SysTick_Config(10000);
  SCB->SHP[((uint32_t)(SysTick_IRQn) & 0xF)-4] = (0xE0);
  isr_off();
  
  //init_kernel();
  
  // Code here
  test_infinite_tasks_running();
  
  
  // Code end
  
  run();
}
