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

int main(int argc, char** argv) {
   SMTP smtp = smtp_init(smtp_open, smtp_close, smtp_read, smtp_write, stdout, stderr);
   if(smtp == NULL) {
      fprintf(stderr, "Out of memory\n");
   }
   /* Print usage for no opts */
   if (argc == 1) {
      smtp_print_usage(smtp);
      smtp_free(smtp);
      return 0;
   }
   /* Parse opts*/
   int error_code = smtp_get_opts(smtp, argc, argv);
   if (error_code) {
      fprintf(stderr, "Argument Error: %s\n", smtp_error(error_code));
      smtp_free(smtp);
      return 1;
   }
   /* Connect to SMTP server */
   if(smtp_connect(smtp)) {
      perror("smtp_connect");
      smtp_free(smtp);
      return 2;
   } 
   /* Send email */
   if(smtp_send_email(smtp)) {
      printf("Failed to send email\n");
      smtp_disconnect(smtp);
      smtp_free(smtp);
      return 2;
   }
   smtp_disconnect(smtp);
   smtp_free(smtp);
   return 0;
}