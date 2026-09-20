#define _GNU_SOURCE // Needed for addrinfo struct
#include "lab.h"
#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* SMTP message terminator */
const char* CRLF = "\r\n";

enum error_code {
   NO_ERROR,
   DUP_FLAG,
   MEM_ERROR,
   BAD_PORT,
   UNKNOWN_FLAG,
   MISSING_FROM,
   MISSING_TO,
   MISSING_SERVER,
   EXTRA_FLAGS
};

char* get_error_str(int error_code) {
   switch (error_code) {
   case NO_ERROR:
      return "no error";
   case DUP_FLAG:
      return "duplicate flag";
   case MEM_ERROR:
      return "out of memory";
   case BAD_PORT:
      return "bad port number";
   case UNKNOWN_FLAG:
      return "unknown flag";
   case MISSING_FROM:
      return "missing sender";
   case MISSING_TO:
      return "missing recipient";
   case MISSING_SERVER:
      return "missing mail server address";
   default:
      return "unknown error";
   }
}

void smtp_message_init(struct smtp_email* opts) {
   opts->from = 0;
   opts->to = 0;
   opts->subject = 0;
   opts->body = 0;
   opts->port = 0;
   opts->helo_host = 0;
   opts->server = 0;
}

void smtp_email_cleanup(struct smtp_email* msg) {
   free(msg->from);
   free(msg->to);
   free(msg->subject);
   free(msg->body);
   free(msg->helo_host);
   free(msg->server);
   memset(msg, 0, sizeof(*msg));
}

int print_usage() {
   return puts("Usage: myapp -f <from> -t <to> [-s subject] [-b body] [-p port]") == EOF || puts("          [-H helo-host] <server>") == EOF || puts("") == EOF || puts("   -f <from>       envelope sender, for example you@example.com") == EOF || puts("   -t <to>         envelope recipient") == EOF || puts("   -s <subject>    subject line (default: empty)") == EOF || puts("  -b <body>       message body (default: read from stdin)") == EOF || puts("   -p <port>       port or service name (default: 25)") == EOF || puts("   -H <helo-host>  host name sent with HELO (default: localhost)") == EOF || puts("   <server>        host name or address of the mail server") == EOF;
}

int parse_cli(int argc, char** argv, struct smtp_email* opts) {
   int arg;
   while ((arg = getopt(argc, argv, "f:t:-s:-b:-p:-H:")) != -1) {
      switch (arg) {
      case 'f':
         if (opts->from) {
            return DUP_FLAG;
         }
         if ((opts->from = strdup(optarg)) == 0) {
            return MEM_ERROR;
         }
         break;
      case 't':
         if (opts->to) {
            return DUP_FLAG;
         }
         if ((opts->to = strdup(optarg)) == 0) {
            return MEM_ERROR;
         }
         break;
      case 's':
         if (opts->subject) {
            return DUP_FLAG;
         }
         if ((opts->subject = strdup(optarg)) == 0) {
            return MEM_ERROR;
         }
         break;
      case 'b':
         if (opts->body) {
            return DUP_FLAG;
         }
         if ((opts->body = strdup(optarg)) == 0) {
            return MEM_ERROR;
         }
         break;
      case 'p':
         if (opts->port) {
            return DUP_FLAG;
         }
         if ((opts->port = atoi(optarg)) == 0) {
            return BAD_PORT;
         }
         break;
      case 'H':
         if (opts->helo_host) {
            return DUP_FLAG;
         }
         if ((opts->helo_host = strdup(optarg)) == 0) {
            return MEM_ERROR;
         }
         break;
      default:
         return UNKNOWN_FLAG;
      }
   }
   if (opts->from == 0) {
      return MISSING_FROM;
   } else if (opts->to == 0) {
      return MISSING_TO;
   } else if (optind >= argc) {
      return MISSING_SERVER;
   } else {
      if ((opts->server = strdup(argv[optind++])) == NULL) {
         return MEM_ERROR;
      }
   }
   if (optind < argc) {
      return EXTRA_FLAGS;
   }
   /* SET DEFAULTS*/
   if (opts->helo_host == 0) {
      if ((opts->helo_host = strdup("localhost")) == NULL) {
         return MEM_ERROR;
      }
   }
   if (opts->port == 0) {
      opts->port = 25;
   }
   /* Read from stdin if body is not set */
   if (opts->body == 0) {
      scanf("%m[^EOF]", &opts->body);
   }
   return 0;
}

char* smtp_sanitize(char* msg) {
   char* san; // Dotted string
   int len = strlen(msg);
   int dotted = msg[0] == '.';
   char* crlf = strstr(msg, CRLF);
   int ret = 0;
   if(crlf != 0) {
      return 0; // string contain CRLF
   }
   if (dotted) {
      ret = asprintf(&san, ".%s%s", msg, CRLF);
   } else {
      ret = asprintf(&san, "%s%s", msg, CRLF);
   }
   if(ret == -1) {
      free(san); // malloc failed
      return 0;
   }
   return san;
}

int check_status(char* line, char* status) {
   int line_len = strlen(line);
   int status_len = strlen(status);
   if(line_len == 0 || status_len == 0 || line_len < status_len) {
      return -1; // line cannot start with status
   }
   return strncmp(line, status, status_len);
}

int is_multi_line(char* line) {
   if(strlen(line) >= 4) {
      return line[3] == '-'; // SMTP: 4th character is `-` indicates multi line
   }
   return 0;
}

int smtp_connect(const char* address, int port, char** error) {
   int error_code;
   int client;
   struct sockaddr_in* saddr;
   struct addrinfo hints, *res, *rp;
   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   error_code = getaddrinfo(address, 0, &hints, &res);
   if (error_code) {
      if (error) {
         asprintf(error, "Get Address Info: %s", gai_strerror(error_code));
      }
      return -1;
   }

   for (rp = res; rp != NULL; rp = rp->ai_next) {
      if ((client = socket(AF_INET, SOCK_STREAM, 0)) == -1) { /* Failed to create socket */
         continue;
      }
      saddr = (struct sockaddr_in*)rp->ai_addr;
      saddr->sin_port = htons(port);
      if (connect(client, (struct sockaddr*)saddr, sizeof(struct sockaddr_in)) != -1) {
         break;
      }
      close(client);
   }
   freeaddrinfo(res); /* Cleanup */
   /* Confirm connection */
   if (rp == 0) {
      if (error) {
         *error = strdup("Could not connect to mail server");
      }
      return -2;
   }
   return client;
}

ssize_t smtp_recv(int socket, void* buffer, size_t length) {
   return recv(socket, buffer, length, 0);
}

ssize_t smtp_send(int socket, void* buffer, size_t length) {
   return send(socket, buffer, length, 0);
}

size_t smtp_read_line(int fd, read_func rf, char** response) {
   int buffer_size = 1024; // Size of buffer
   char buffer[buffer_size]; // Storage buffer
   size_t ret = 0; // Number of bytes read
   size_t size = 0; // Number of bytes in out
   do {
      /* Read message from smtp server */
      if ((ret = rf(fd, buffer, buffer_size)) == -1) {
         return -1; // Read failed
      }
      /* Allocate string with null terminator */
      char* ptr = (char*)realloc(*response, ret + size + 1);
      if (ptr == NULL) { // GCOVR_EXCL_START
         return -1; // realloc failed
      } // GCOVR_EXCL_STOP
      memcpy(ptr + size, buffer, ret); // Append buffer
      size += ret;
      ptr[size] = 0;
      *response = ptr;
      /* Check for \r\n message ending */
   } while (strstr(*response, CRLF) == NULL);
   return size;
}

size_t smtp_send_line(int fd, write_func wf, char* msg) {
   int total =  0; // Total number of bytes written
   int ret = 0; // Number of bytes written
   if(msg == 0) {
      return -1;
   }
   size_t len = strlen(msg); // Size of message to send
   do {
      /* Attemp to send message, message may need multiple writes. */
      if ((ret = wf(fd, msg + total, len - total)) == -1) {
         total = -1;
         break;
      }
      total += ret;
   } while (total < len);
   return total;
}

int smtp_helo(int fd, write_func wf, char* host, char** msg) {
   int ret;
   if ((ret = asprintf(msg, "HELO %s", host)) != -1) {
      char* san = smtp_sanitize(*msg);
      ret = smtp_send_line(fd, wf, san);
      free(san);
   }
   return ret;
}

int smtp_mail_from(int fd, write_func wf, char* sndr, char** msg) {
   int ret;
   if ((ret = asprintf(msg, "MAIL FROM:<%s>", sndr)) != -1){
      char* san = smtp_sanitize(*msg);
      ret = smtp_send_line(fd, wf, san);
      free(san);
   }
   return ret;
}

int smtp_rcpt_to(int fd, write_func wf, char* rcpt, char** msg) {
   int ret;
   if ((ret = asprintf(msg, "RCPT TO:<%s>", rcpt)) != -1) {
      char* san = smtp_sanitize(*msg);
      ret = smtp_send_line(fd, wf, san);
      free(san);
   }
   return ret;
}

int smtp_data(int fd, write_func wf, char** msg) {
   int ret;
   *msg = strdup("DATA");
   if (msg == 0) {
      return -1;
   }
   char* san = smtp_sanitize(*msg);
   ret = smtp_send_line(fd, wf, *msg);
   free(san);
   return ret;
}

int smtp_data_body(int fd, write_func wf, char* body, char** msg) {
   int len = 0;
   int ret = 0;
   char *line, *save_ptr;
   line = strtok_r(body, CRLF, &save_ptr);
   while(line != 0){
      char* san = smtp_sanitize(line);
      if ((ret = smtp_send_line(fd, wf, san)) == -1) {
         free(san);
         break;
      }
      free(san);
      char* ptr;
      if (*msg == 0) {
         asprintf(&ptr, "C: %s", line);
      } else {
         asprintf(&ptr, "%s\nC: %s", *msg, line);
      }
      free(*msg);
      *msg = ptr;
      line = strtok_r(0, CRLF, &save_ptr);
   }
   if(ret != -1) {
      ret = smtp_send_line(fd, wf, ".\r\n");
      char* ptr;
      asprintf(&ptr, "%s\nC: .", *msg);
      free(*msg);
      *msg = ptr;
   }
   return ret;
}

int smtp_quit(int fd, write_func wf, char** msg) {
   *msg = strdup("DATA");
   if (msg) {
      return -1;
   }
   return smtp_send_line(fd, wf, "QUIT");
}

int smtp_listen(int fd, read_func rf, char* status_code, char** msg) {
   int ret = 0;
   char* line;
   /* Get message */
   if ((ret = smtp_read_line(fd, rf, msg)) == -1) {
      return 0;
   }
   /* Check status code */
   if (check_status(*msg, status_code) != 0) {
      return -1;
   }
   return 0;
}

#define MULTI_LISTEN(ret, fd, read, code, msg, cond) \
   do {                                              \
      ret = smtp_listen(fd, read, code, &msg);       \
      printf("S: %s", msg);                          \
      cond = is_multi_line(msg);                     \
      free(msg);                                     \
      msg = 0;                                       \
      if (ret) {                                     \
         return 1;                                   \
      }                                              \
   } while (cond)

int smtp_send_email(int fd, read_func read, write_func write, struct smtp_email* email) {
   char* msg = 0;
   int ret;
   int is_multi = 0;
   MULTI_LISTEN(ret, fd, read, "220", msg, is_multi);
   ret = smtp_helo(fd, write, email->helo_host, &msg);
   printf("C: %s\n", msg);
   free(msg);
   msg = 0;
   if (ret == -1) {
      return -1;
   }
   MULTI_LISTEN(ret, fd, read, "250", msg, is_multi);
   ret = smtp_mail_from(fd, write, email->from, &msg);
   printf("C: %s\n", msg);
   free(msg);
   msg = 0;
   if (ret == -1) {
      return -1;
   }
   MULTI_LISTEN(ret, fd, read, "250", msg, is_multi);
   ret = smtp_rcpt_to(fd, write, email->to, &msg);
   printf("C: %s\n", msg);
   free(msg);
   msg = 0;
   if (ret == -1) {
      return -1;
   }
   MULTI_LISTEN(ret, fd, read, "250", msg, is_multi);
   ret = smtp_data(fd, write, &msg);
   printf("C: %s\n", msg);
   free(msg);
   msg = 0;
   if (ret == -1) {
      return -1;
   }
   MULTI_LISTEN(ret, fd, read, "354", msg, is_multi);
   ret = smtp_data_body(fd, write, email->body, &msg);
   printf("%s\n", msg);
   free(msg);
   msg = 0;
   if (ret == -1) {
      return -1;
   }
   MULTI_LISTEN(ret, fd, read, "250", msg, is_multi);
   ret = smtp_quit(fd, write, &msg);
   printf("C: %s\n", msg);
   free(msg);
   msg = 0;
   if (ret == -1) {
      return -1;
   }
   MULTI_LISTEN(ret, fd, read, "221", msg, is_multi);
   return 0;
}
#undef MULTI_LISTEN