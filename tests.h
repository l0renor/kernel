#include <assert.h>
#include <limits.h>

void TestWrapper( void (* test_in_running_mode)() );

// Task Administration Tests

void test_init_kernel();

void test_create_task_init();
void test_create_task_running();
void test_create_task_error_memory_full();

void test_run();

void test_terminate();

// Communication Tests

void test_create_mailbox();
void test_remove_mailbox();