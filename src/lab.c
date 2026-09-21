#define _GNU_SOURCE // Needed for addrinfo struct
#include "lab.h"
#include <arpa/inet.h>
#include <errno.h>
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

enum SMTP_ERROR_CODE {
   NO_ERROR,
   NULL_PTR,
   DUP_FLAG,
   MEM_ERROR,
   BAD_PORT,
   UNKNOWN_FLAG,
   MISSING_FROM,
   MISSING_TO,
   MISSING_SERVER,
   EXTRA_FLAGS
};

/* SMTP message contents*/
typedef struct SMTP {
   open_func open; // Function used to open a connection
   close_func close; // Function used to close a connection
   read_func read; // Funciton used to read from the server
   write_func write; // Function used to write to server
   FILE* out; // Output file descriptor
   FILE* err; // Error file descriptor
   int sfd; // Server file descriptor
   char* from; // Sender
   char* to; // Recipient
   char* subject; // Subject line
   char* body; // Message body
   int port; // Server port
   char* helo_host; // Client address
   char* server; // Server address
}* SMTP_ref;

SMTP smtp_init(open_func open, close_func close, read_func read, write_func write, FILE* out, FILE* err) {
   SMTP_ref ref = (SMTP_ref)malloc(sizeof(struct SMTP));
   if (ref == NULL) { // GCOVR_EXCL_START
      return NULL; // malloc failed
   }// GCOVR_EXCL_STOP
   ref->open = open ? open : smtp_open;
   ref->close = close ? close : smtp_close;
   ref->read = read ? read : smtp_read;
   ref->write = write ? write : smtp_write;
   ref->out = out;
   ref->err = err;
   ref->from = 0;
   ref->to = 0;
   ref->subject = 0;
   ref->body = 0;
   ref->port = 0;
   ref->helo_host = 0;
   ref->server = 0;
   return ref;
}

void smtp_free(SMTP smtp) {
   SMTP_ref ref = smtp;
   free(ref->from);
   free(ref->to);
   free(ref->subject);
   free(ref->body);
   free(ref->helo_host);
   free(ref->server);
   ref->from = 0;
   ref->to = 0;
   ref->subject = 0;
   ref->body = 0;
   ref->port = 0;
   ref->helo_host = 0;
   ref->server = 0;
   free(smtp);
}

int smtp_open(const char* host, int port) {
   int error_code;
   int sfd;
   struct sockaddr_in* saddr;
   struct addrinfo hints, *res, *rp;
   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   /* Get list of hosts */
   if ((error_code = getaddrinfo(host, 0, &hints, &res))) { // GCOVR_EXCL_START
      return error_code;
   } // GCOVR_EXCL_STOP
   /* Attempt to connect to hosts */
   for (rp = res; rp != NULL; rp = rp->ai_next) {
      /* Failed to create socket */
      if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // GCOVR_EXCL_START
         continue;
      } // GCOVR_EXCL_STOP
      saddr = (struct sockaddr_in*)rp->ai_addr;
      saddr->sin_port = htons((uint16_t)(port));
      errno = 0;
      if (connect(sfd, (struct sockaddr*)saddr, sizeof(struct sockaddr_in)) != -1) { // GCOVR_EXCL_START
         break;
      }
      close(sfd);
   } // GCOVR_EXCL_STOP
   freeaddrinfo(res); /* Cleanup */
   /* Confirm connection */
   if (rp == 0) { // GCOVR_EXCL_START
      return -1;
   }
   return sfd;
} // GCOVR_EXCL_STOP

int smtp_close(int fd) { // GCOVR_EXCL_START
   errno = 0;
   return close(fd);
} // GCOVR_EXCL_STOP

ssize_t smtp_read(int fd, void* buffer, size_t length) { // GCOVR_EXCL_START
   errno = 0;
   return recv(fd, buffer, length, 0);
} // GCOVR_EXCL_STOP

ssize_t smtp_write(int fd, const void* buffer, size_t length) { // GCOVR_EXCL_START
   errno = 0;
   return send(fd, buffer, length, 0);
} // GCOVR_EXCL_STOP

int smtp_print_usage(SMTP smtp) {
   SMTP_ref ref = (SMTP_ref)smtp;
   if (ref == NULL) {
      return -1;
   }
   if (ref->out == NULL) {
      return -1;
   }
   errno = 0;
   fputs("Usage: myapp -f <from> -t <to> [-s subject] [-b body] [-p port]\n", ref->out);
   fputs("          [-H helo-host] <server>\n", ref->out);
   fputs("   -f <from>       envelope sender, for example you@example.com\n", ref->out);
   fputs("   -t <to>         envelope recipient\n", ref->out);
   fputs("   -s <subject>    subject line (default: empty)\n", ref->out);
   fputs("   -b <body>       message body (default: read from stdin)\n", ref->out);
   fputs("   -p <port>       port or service name (default: 25)\n", ref->out);
   fputs("   -H <helo-host>  host name sent with HELO (default: localhost)\n", ref->out);
   fputs("   <server>        host name or address of the mail server\n", ref->out);
   return 0;
}

int smtp_get_opts(SMTP smtp, int argc, char** argv) {
   if (smtp == NULL) {
      return NULL_PTR;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   int arg;
   while ((arg = getopt(argc, argv, "f:t:-s:-b:-p:-H:")) != -1) {
      switch (arg) {
      case 'f':
         if (ref->from) {
            return DUP_FLAG;
         }
         if ((ref->from = strdup(optarg)) == 0) {
            return MEM_ERROR;
         }
         break;
      case 't':
         if (ref->to) {
            return DUP_FLAG;
         }
         if ((ref->to = strdup(optarg)) == 0) {
            return MEM_ERROR;
         }
         break;
      case 's':
         if (ref->subject) {
            return DUP_FLAG;
         }
         if ((ref->subject = strdup(optarg)) == 0) {
            return MEM_ERROR;
         }
         break;
      case 'b':
         if (ref->body) {
            return DUP_FLAG;
         }
         if ((ref->body = strdup(optarg)) == 0) {
            return MEM_ERROR;
         }
         break;
      case 'p':
         if (ref->port) {
            return DUP_FLAG;
         }
         if ((ref->port = atoi(optarg)) == 0) {
            return BAD_PORT;
         }
         break;
      case 'H':
         if (ref->helo_host) {
            return DUP_FLAG;
         }
         if ((ref->helo_host = strdup(optarg)) == 0) {
            return MEM_ERROR;
         }
         break;
      default:
         return UNKNOWN_FLAG;
      }
   }
   if (ref->from == 0) {
      return MISSING_FROM;
   } else if (ref->to == 0) {
      return MISSING_TO;
   } else if (optind >= argc) {
      return MISSING_SERVER;
   } else {
      if ((ref->server = strdup(argv[optind++])) == NULL) {
         return MEM_ERROR;
      }
   }
   if (optind < argc) {
      return EXTRA_FLAGS;
   }
   /* SET DEFAULTS*/
   if (ref->helo_host == 0) {
      if ((ref->helo_host = strdup("localhost")) == NULL) {
         return MEM_ERROR;
      }
   }
   if (ref->port == 0) {
      ref->port = 25;
   }
   /* Read from stdin if body is not set */
   if (ref->body == 0) {
      scanf("%m[^EOF]", &ref->body);
   }
   return 0;
}

char* smtp_error(int error) {
   switch (error) {
   case NO_ERROR:
      return "no error";
   case NULL_PTR:
      return "Null ptr";
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

int smtp_connect(SMTP smtp) {
   errno = 0;
   if (smtp == 0) {
      return NULL_PTR;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   if ((ref->sfd = ref->open(ref->server, ref->port)) == -1) {
      return -1;
   }
   return 0;
}

int smtp_disconnect(SMTP smtp) {
   errno = 0;
   if (smtp == 0) {
      return NULL_PTR;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   if (ref->sfd == 0) {
      return -1;
   }
   int ret = ref->close(ref->sfd);
   ref->sfd = 0;
   return ret;
}

ssize_t smtp_send(SMTP smtp, void* buffer, size_t length) {
   errno = 0;
   if (smtp == 0) {
      return NULL_PTR;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   if (ref->sfd == 0) {
      return -1;
   }
   return ref->write(ref->sfd, buffer, length);
}

ssize_t smtp_recv(SMTP smtp, void* buffer, size_t length) {
   errno = 0;
   if (smtp == 0) {
      return NULL_PTR;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   if (ref->sfd == 0) {
      return -1;
   }
   return ref->read(ref->sfd, buffer, length);
}

int smtp_read_line(SMTP smtp, char** line) {
   if (smtp == NULL) {
      return -1;
   }
   size_t buffer_size = 1024; // Size of buffer
   char buffer[buffer_size]; // Storage buffer
   int bytes_read = 0; // Number of bytes read
   int line_length = 0; // Number of bytes in out
   do {
      /* Read message from smtp server */
      bytes_read = (int)smtp_recv(smtp, buffer, buffer_size);
      if (bytes_read == -1) {
         return -1; // Read failed
      }
      /* Allocate string with null terminator */
      size_t line_size = (size_t)(line_length + bytes_read + 1); // Number of allocated bytes
      char* ptr = (char*)realloc(*line, line_size);
      if (ptr == NULL) { // GCOVR_EXCL_START
         return -1; // realloc failed
      } // GCOVR_EXCL_STOP
      *line = ptr;
      char* line_end = *line + line_length;
      memcpy(line_end, buffer, (size_t)bytes_read); // Append buffer
      line_length += bytes_read;
      ptr[line_length] = 0;
      /* Check for CRLF message ending */
   } while (strstr(*line, CRLF) == NULL);
   return line_length;
}

int smtp_write_line(SMTP smtp, char* line) {
   if (smtp == NULL) {
      return -1;
   }
   if (line == 0) {
      return -1; // Their is no message to send
   }
   int total_written = 0; // Total number of bytes written
   int bytes_written = 0; // Number of bytes written
   int line_length = (int)strlen(line); // Size of message to send
   do {
      /* Attemp to send message, message may need multiple writes. */
      char* ptr = line + total_written;
      size_t bytes_to_write = (size_t)(line_length - total_written);
      if ((bytes_written = (int)smtp_send(smtp, ptr, bytes_to_write)) == -1) {
         return -1;
      }
      total_written += bytes_written;
   } while (total_written < line_length);
   return total_written;
}

char* smtp_sanitize(char* msg) {
   char* san; // Dotted string
   char* crlf = strstr(msg, CRLF);
   int ret;
   if (crlf != 0) {
      return 0; // string contain CRLF
   }
   if (msg[0] == '.') {
      ret = asprintf(&san, ".%s%s", msg, CRLF);
   } else {
      ret = asprintf(&san, "%s%s", msg, CRLF);
   }
   if (ret == -1) {
      free(san); // malloc failed
      return 0;
   }
   return san;
}

int smtp_helo(SMTP smtp) {
   errno = 0;
   if (smtp == NULL) {
      return -1;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   char* line;
   if (asprintf(&line, "HELO %s%s", ref->helo_host, CRLF) == -1) {
      return -1;
   }
   if (ref->out) {
      fprintf(ref->out, "C: %s", line);
   }
   int ret = smtp_write_line(smtp, line);
   free(line);
   return ret;
}

int smtp_mail_from(SMTP smtp) {
   errno = 0;
   if (smtp == NULL) {
      return -1;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   char* line;
   if (asprintf(&line, "MAIL FROM:<%s>%s", ref->from, CRLF) == -1) {
      return -1;
   }
   if (ref->out) {
      fprintf(ref->out, "C: %s", line);
   }
   int ret = smtp_write_line(smtp, line);
   free(line);
   return ret;
}

int smtp_rcpt_to(SMTP smtp) {
   errno = 0;
   if (smtp == NULL) {
      return -1;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   char* line;
   if (asprintf(&line, "RCPT TO:<%s>%s", ref->to, CRLF) == -1) {
      return -1;
   }
   if (ref->out) {
      fprintf(ref->out, "C: %s", line);
   }
   int ret = smtp_write_line(smtp, line);
   free(line);
   return ret;
}

int smtp_data_start(SMTP smtp) {
   errno = 0;
   if (smtp == NULL) {
      return -1;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   char* line;
   if (asprintf(&line, "DATA%s", CRLF) == -1) {
      return -1;
   }
   if (ref->out) {
      fprintf(ref->out, "C: %s", line);
   }
   int ret = smtp_write_line(smtp, line);
   free(line);
   return ret;
}

int smtp_subject(SMTP smtp) {
   errno = 0;
   if (smtp == NULL) {
      return -1;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   if(ref->subject == NULL) {
      return 0; // No subject line
   }
   char* line;
   char* san;
   if((san = smtp_sanitize(ref->subject)) == NULL) {
      return -1;
   }
   if (asprintf(&line, "Subject: %s%s", san, CRLF) == -1) {
      return -1;
   }
   if(ref->out) {
      fprintf(ref->out, "C: %s", line);
   }
   int ret = smtp_write_line(smtp, line);
   free(san);
   free(line);
   return ret;
}

int smtp_data_body(SMTP smtp) {
   errno = 0;
   if (smtp == NULL) {
      return -1;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   char *line, *san;
   int total = 0;
   line = strtok(ref->body, CRLF);
   while (line != NULL) {
      san = smtp_sanitize(line);
      if (san == NULL) {
         return -1;
      }
      int ret = smtp_write_line(smtp, line);
      if (ret == -1) {
         free(san);
         return -1;
      }
      total += ret;
      if (ref->out) {
         fprintf(ref->out, "C: %s", san);
      }
      line = strtok(NULL, CRLF);
   }
   return total;
}

int smtp_data_end(SMTP smtp) {
   errno = 0;
   if (smtp == NULL) {
      return -1;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   char* line;
   if (asprintf(&line, "%s.%s", CRLF, CRLF) == -1) {
      return -1;
   }
   if (ref->out) {
      fprintf(ref->out, "C: .\r\n");
   }
   int ret = smtp_write_line(smtp, line);
   free(line);
   return ret;
}

int smtp_quit(SMTP smtp) {
   errno = 0;
   if (smtp == NULL) {
      return -1;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   char* line;
   if (asprintf(&line, "QUIT%s", CRLF) == -1) {
      return -1;
   }
   if (ref->out) {
      fprintf(ref->out, "C: %s", line);
   }
   int ret = smtp_write_line(smtp, line);
   free(line);
   return ret;
}

int check_code(char* line, char* code) {
   size_t line_len = strlen(line);
   size_t status_len = strlen(code);
   if (line_len == 0 || status_len == 0 || line_len < status_len) {
      return -1; // line cannot start with status
   }
   return strncmp(line, code, status_len);
}

int is_multi_line(char* line) {
   if (strlen(line) >= 4 && line[3] == '-') {
      return 1; // SMTP: 4th character being a `-` indicates multi line
   }
   return 0;
}

int smtp_listen(SMTP smtp, char* code) {
   if (smtp == NULL) {
      return -1;
   }
   SMTP_ref ref = (SMTP_ref)smtp;
   int ret;
   do {
      char* line = 0;
      ret = smtp_read_line(smtp, &line);
      if (ret == -1) {
         free(line); // Read faild
         fprintf(ref->err, "STMP LISTEN: %s", strerror(errno));
         return -1;
      }
      fprintf(ref->out, "S: %s", line);
      if (check_code(line, code)) {
         return -1; // Code does not match
      }
      ret = is_multi_line(line);
      free(line);
   } while (ret);
   return 0;
}

int smtp_send_email(SMTP smtp) {
   if (smtp == NULL) {
      return -1;
   }
   if (smtp_listen(smtp, "220")) {
      return -1;
   }
   if (smtp_helo(smtp) == -1) { // HELO
      return -1;
   }
   if (smtp_listen(smtp, "250")) {
      return -1;
   }
   if (smtp_mail_from(smtp) == -1) { // MAIL TO
      return -1;
   }
   if (smtp_listen(smtp, "250")) {
      return -1;
   }
   if (smtp_rcpt_to(smtp) == -1) { // RCPT TO
      return -1;
   }
   if (smtp_listen(smtp, "250")) {
      return -1;
   }
   if (smtp_data_start(smtp) == -1) { // DATA START
      return -1;
   }
   if (smtp_listen(smtp, "354")) {
      return -1;
   }
   if (smtp_subject(smtp) == -1) { // SUBJECT LINE
      return -1;
   }
   if (smtp_data_body(smtp) == -1) { // DATA BODY
      return -1;
   }
   if (smtp_data_end(smtp) == -1) { // DATA END
      return -1;
   }
   if (smtp_listen(smtp, "250")) {
      return -1;
   }
   if (smtp_quit(smtp) == -1) { // QUIT
      return -1;
   }
   if (smtp_listen(smtp, "221")) {
      return -1;
   }
   return 0;
}
