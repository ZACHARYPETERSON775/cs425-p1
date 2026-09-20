#include "lab.h"
#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef TEST
#define main main_exclude
#endif

/** * @brief Prints the programs usage message.
 *
 * This function prints the programs usage message to stdout.
 * @return 0 on error
 */



int main(int argc, char** argv) {
   struct smtp_email options = { 0 }; // Initalize smtp_message
   /* Print usage for no opts */
   if (argc == 1) {
      print_usage();
      return 0;
   }
   /* Parse opts*/
   int error_code = parse_cli(argc, argv, &options);
   if (error_code) {
      smtp_email_cleanup(&options);
      fprintf(stderr, "Argument Error: %s\n", get_error_str(error_code));
      return 1;
   }

   /* Connect to mail server*/
   char* error_str = 0;
   int fd = smtp_connect(options.server, options.port, &error_str);
   if(error_str){
      smtp_email_cleanup(&options);
      fprintf(stderr, "%s\n", error_str);
      free(error_str);
      return 2;
   }

   if (smtp_send_email(fd, smtp_recv, smtp_send, &options) == 0) {
      printf("Succeded in sending email\n");
   } else {
      printf("failed to send email\n");
   }

   close(fd);
   smtp_email_cleanup(&options);
   return 0;
}