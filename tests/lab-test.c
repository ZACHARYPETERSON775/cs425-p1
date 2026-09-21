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

void setUp(void) {
   printf("Setting up tests...\n");
   smtp = smtp_init(open, close, read, write, stdout, stderr);
   smtp_no_output = smtp_init(open, close, read, write, NULL, NULL);
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
   TEST_ASSERT_FALSE(res);
}

void test_smtp() {
   char* argv[14] = { "", "-f", "test@gmail.com", "-t", "test@gmail.com", "-s", "testing", "-b", "Body", "-p", "2525", "-H", "not_local", "server.txt" };
   int argc = 14;
   int res = smtp_get_opts(smtp, argc, argv);
   TEST_ASSERT_FALSE(res);
   /* Parse opts*/
   int error_code = smtp_get_opts(smtp, argc, argv);
   if (error_code) {
      fprintf(stderr, "Argument Error: %s\n", smtp_error(error_code));
      smtp_free(smtp);
      return;
   }
   /* Connect to SMTP server */
   if(smtp_connect(smtp)) {
      perror("smtp_connect");
      smtp_free(smtp);
      return;
   } 
   /* Send email */
   if(smtp_send_email(smtp)) {
      printf("Failed to send email\n");
      smtp_disconnect(smtp);
      smtp_free(smtp);
      return;
   }
   smtp_disconnect(smtp);
}

int main(void) {
   UNITY_BEGIN();
   RUN_TEST(test_print_usage);
   RUN_TEST(test_print_usage_no_io);
   RUN_TEST(test_print_usage_null_smtp);
   RUN_TEST(test_smtp_get_opts);
   return UNITY_END();
}
