#include "system_sam3x.h"
#include "at91sam3x8.h"

#include "kernel_functions.c"
#include "tests.c"

void main(void)
{
  test_init_kernel();
}
