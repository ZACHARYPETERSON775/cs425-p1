#include "../src/lab.h"
#include "harness/unity.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// GCOVR_EXCL_START
// GCOVR_EXCL_STOP
SMTP smtp;
SMTP smtp_no_output;

int oopen(const char* file, int flags) {
   return open(file, flags);
}

void setUp(void) {
   printf("Setting up tests...\n");
   smtp = smtp_init(oopen, close, read, write, stdout, stderr);
   smtp_no_output = smtp_init(oopen, close, read, write, NULL, NULL);
}

void tearDown(void) {
   printf("Tearing down tests...\n");
   smtp_free(smtp);
   smtp_free(smtp_no_output);
}

void test_print_usage() {
   int res = smtp_print_usage(smtp);
   TEST_ASSERT_FALSE(res);
}

void test_print_usage_no_io() {
   int res = smtp_print_usage(smtp_no_output);
   TEST_ASSERT_TRUE(res);
}

void test_print_usage_null_smtp() {
   int res = smtp_print_usage(NULL);
   TEST_ASSERT_TRUE(res);
}

void test_smtp_get_opts() {
   char* argv[14] = { "", "-f", "test@gmail.com", "-t", "test@gmail.com", "-s", "testing", "-b", "Body", "-p", "2525", "-H", "not_local", "server.txt" };
   int argc = 14;
   int res = smtp_get_opts(smtp, argc, argv);
   TEST_ASSERT_EQUAL(0, res);
}

void test_smtp() {
  int res = smtp_set(smtp, "test@gmail.com", "test@gmail.com", "testing",
                     "Body", O_RDONLY, "host", "./tests/test_smtp.txt");
  TEST_ASSERT_EQUAL(0, res);
  res = smtp_connect(smtp);
  if(res) {
   perror("test_smtp");
  }
  TEST_ASSERT_EQUAL(0, res);
  res = smtp_send_email(smtp);
  if (res) {
    smtp_disconnect(smtp);
    TEST_FAIL();
  }
  TEST_ASSERT_EQUAL(0, res);
  res = smtp_disconnect(smtp);
  TEST_ASSERT_EQUAL(0, res);
}

int main(void) {
   UNITY_BEGIN();
   RUN_TEST(test_print_usage);
   RUN_TEST(test_print_usage_no_io);
   RUN_TEST(test_print_usage_null_smtp);
   RUN_TEST(test_smtp_get_opts);
   RUN_TEST(test_smtp);
   return UNITY_END();
}
