#ifndef LAB_H
#define LAB_H

#include <sys/types.h>

/* Structures and typedefs */
typedef ssize_t (*read_func)(int,void*,size_t);
typedef ssize_t (*write_func)(int,void*,size_t);
/* SMTP message contents*/
struct smtp_email {
   char* from; // Sender
   char* to; // Recipient
   char* subject; // Subject line
   char* body; // Message body
   int port; // Server port
   char* helo_host; // Client address
   char* server; // Server address
};

/** @brief Return string associated with error_code.
 *
 * This function returns a string associated with an error code.
 * The string is should not be freed by the caller.
 * @return An error string.
 */
char* get_error_str(int error_code);

/** * @brief Free smtp message struct.
 *
 * This function frees smtp_email internals and sets them to 0.
 * @param options Ptr to app_opts to free
 */
void smtp_email_cleanup(struct smtp_email* msg);

/** * @brief Prints the programs usage message.
 *
 * This function prints the programs usage message to stdout.
 * @return 0 on error
 */
int print_usage();

/** * @brief Parses cli arguments.
 *
 * This funciton uses getopt() to parse cli arguments.
 * Arguments are allocated using strdup() and should be freed by the caller.
 * Error string can be obtained from get_error_str()
 * @param argc Number of arguments
 * @param argv Argument list
 * @param opts Ptr to app_opts to store flags in
 * @return 0 on success
 * @return non-zero on ERROR
 */
int parse_cli(int argc, char** argv, struct smtp_email* opts);

/** * @brief Sanatize messages for smtp.
 *
 * This funciton sanatizes message for sending to smtp servers.
 * If the msg starts with a `.` an additional `.` will be prepended.
 * As smtp lines end with CRLF, the msg must not contain CRLF.
 * The returned string is allocaed using malloc and should be freed by the caller.
 * @param msg null-terminated string
 * @return Sanitized null-terminated string
 * @return 0 on error
*/
char* smtp_sanitize(char* msg);

/* Second Layer: SMTP functions */

/** * @brief Checks the statsu of a line
 *
 * This function checks that a status line starts with specific status code.
 * @param line The line to check
 * @param statsu The code to check for
 * @return 0 if the status codes matches
 */
int check_status(char* line, char* status);

/** * @brief Is the message multi line?
 *
 * This function checks if the current message is a multiline messsage.
 * @param line The line to check
 * @return non-zero if line is multi-line
 */
int is_multi_line(char* line);

/** * @brief Lookup and connect to a server.
 *
 * This function looks for and connects to an IPv4 server using its address.
 * @param address Server address
 * @param port Port number to bind to
 * @param error Prt to string to store errors in
 * @return Non-negative file descriptor
 * @return -1 address lookup failed
 * @return -2 failed to connect
 */
int smtp_connect(const char* address, int port, char** error);

/** * @brief Read a message from the server
 *
 * This function reads a message from a server into the buffer.
 * Due to limits of buffer size, a message may require more than one
 * call to retrive the entire message.
 * On error, errno is set.
 * @param socket Socket File Descriptor
 * @param buffer Ptr to a buffer to store the message
 * @param length Length in bytes of the buffer
 * @return Number of bytes read
 * @return 0 if no messages are ready and the server has shut down.
 * @return -1 on error
 */
ssize_t smtp_recv(int socket, void* buffer, size_t length);

/** * @brief Send a message to the smtp server.
 *
 * This function sends a message to the smtp server.
 * On error, errno is set.
 * @param socket Socket File Descriptor
 * @param buffer Ptr to the message to send
 * @param length Length of the message
 * @return Number of bytes sent
 * @return -1 on error
 */
ssize_t smtp_send(int socket, void* buffer, size_t length);

/** * @brief Read a response line from the smtp server.
 *
 * This function reads a response line from the smtp server.
 * The string is allocated using malloc() and should be freed by the caller.
 * The string is null-terminated which is not included in the returned size.
 * @param fd File Descriptor for the server
 * @param read_func Function used to read from the file descriptor.
 * @param response
 * @return Number of bytes read
 * @return -1 on error
 */
size_t smtp_read_line(int fd, read_func rf, char** response);

/** * @brief Send a message line to the smtp server.
 * 
 * This function sends a sinle line to the smtp server.
 * @param fd SMTP server file descriptor
 * @param wf Ptr to function used to write to the server
 * @param msg Null-terminated string to send
 * @return number of bytes sent on success
 * @return -1 on error
 */
size_t smtp_send_line(int fd, write_func wf, char* msg);

/* Standard SMTP messages */
int smtp_helo(int fd, write_func wf, char* host, char** msg);
int smtp_mail_from(int fd, write_func wf, char* sndr, char** msg);
int smtp_rcpt_to(int fd, write_func wf, char* rcpt, char** msg);
int smtp_data(int fd, write_func wf, char** msg);
int smtp_data_body(int fd, write_func, char* body, char** msg);
int smtp_quit(int fd, write_func wf, char** msg);

/** * @brief Listen for responses from the smtp server
 *
 * This function listens for a response from the smtp server with a specific status code.
 * @param fd SMTP server file descriptor
 * @param rf Ptr to read function
 * @param status_code String containing the expected status code
 * @param msgs Ptr to store read message in
 * @return 0 if the status code matchs
 * @return non-zero on error or if the message was not recived
 */
int smtp_listen(int fd, read_func rf, char* status_code, char** msg);

/** * @brief Send an email to the smtp server
 * 
 * This function send an email to an smtp server.
 * @param fd SMTP file descriptor
 * @param rf Function Ptr used to read from the smtp server
 * @param wf Function Ptr used to write to the smtp server
 * @param email Email structure
 * @return 0 on success
*/
int smtp_send_email(int fd, read_func rf, write_func wf, struct smtp_email* email);

#endif // LAB_H
