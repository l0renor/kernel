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
void stage_one_test_case();
void stage_one_test_case_task_one();
void stage_one_test_case_task_two();
void stage_one_test_case_task_wait();

void test_messaging_basic();
void test_messaging_basic_1();
void test_messaging_basic_2();

void test_messaging_exceptions();
void test_1_messaging_exceptions();
void test_2_messaging_exceptions();

void test_messaging_exceptionsRcv();
void test_1_messaging_exceptionsRcv();
void test_2_messaging_exceptionsRcv();

void test_messaging_exceptions_snd_nW();
void test_1_messaging_exceptions_snd_nW();
