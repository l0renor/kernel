#include "system_sam3x.h"
#include "at91sam3x8.h"

#include "kernel_functions.c"
#include "tests.c"

void main(void)
{
  TestWrapper(test_create_task_running);
}
