#ifndef LAB_H
#define LAB_H

#include <stdio.h>
#include <sys/types.h>

/* Structures and typedefs */
typedef int (*open_func)(const char*, int); // Function for opening connection
typedef int (*close_func)(int); // Function for closing connection
typedef ssize_t (*read_func)(int, void*, size_t); // Function for reading
typedef ssize_t (*write_func)(int, const void*, size_t); // Function for writing
typedef void* SMTP; // SMTP handle

/** * @brief Allocate a new SMTP handle.
 * 
 * This function allocates a new SMTP handle that uses the provided function for its lifetime.
 * @param open Function for connecting to a SMTP server.
 * @param close Function for closing a SMTP server connection.
 * @param read Function for reading from a SMTP server.
 * @param write Function for writing to a SMTP server.
 * @param out Output file descriptor, set to NULL to disable.
 * @param err Error file descriptor, set to NULL to disable.
 * @return SMTP handle with the selected functions.
 */
SMTP smtp_init(open_func open, close_func close, read_func read, write_func write, FILE* out, FILE* err);

/** * @brief Free a SMTP handle.
 * 
 * This function frees a SMTP handle and its underlying data.
 * Open connections should be closed before freeing the handle.
 * @param smtp Handle to free.
 */
void smtp_free(SMTP smtp);

/** * @brief Default open method for SMTP.
 * 
 * This function opens a connect to a SMTP server.
 * @param smtp SMTP handle.
 * @return On success, return a non-negative file descriptor.
 * @return On socket error, return error code (use gai_strerror to get error string).
 * @return On connection error, return -1 and set errno
 */
int smtp_open(const char* host, int port);

/** * @brief Default close method for SMTP
 *
 * This function closes the open connection to an SMTP server.
 * @param smtp SMTP handle.
 * @return On success, return 0.
 * @return On error, return -1 and set errno.
*/
int smtp_close(int fd);

/** * @brief Default read method for SMTP.
 *
 * This function reads up to length bytes into buffer from the SMTP server.
 * @param fd Valid file descriptor returned from smtp_open.
 * @param buffer Buffer to store data read from SMTP server.
 * @param length Size of the data buffer.
 * @return On success, the number of bytes read is returned.
 * @return Zero indicates the SMTP server has shutdown.
 * @return Less than length bytes may be read this it not an error.
 * @return On error, -1 is returned, and errno is set to indicate the error.
 */
ssize_t smtp_read(int fd, void* buffer, size_t length);

/** * @brief Default write method for SMTP.
 *
 * This function writes upto length bytes from buffer to the SMTP server.
 * @param fd Valid file descriptor returned from smtp_open.
 * @param buffer Buffer of data to write to the SMTP server.
 * @param length Size of the data buffer.
 * @return On success, the number of bytes written is returned.
 * @return On error, -1 is returned, and errno is set to indicate the error.
*/
ssize_t smtp_write(int fd, const void* buffer, size_t length);

/** * @brief Prints the programs usage message.
 *
 * This function prints a usage message to SMTP handles out files descriptor.
 * @param smtp SMTP handle.
 * @return On success, return 0.
 * @return On error, returns -1.
 */
int smtp_print_usage(SMTP smtp);

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
int smtp_get_opts(SMTP smtp, int argc, char** argv);

/** @brief Return string associated with error_code.
 *
 * This function returns a string associated with an error code.
 * The string is should not be freed by the caller.
 * @return An error string.
 */
char* smtp_error(int error);

/** * @brief Connect to the SMTP server.
 * 
 * This function calls the SMTP handles open_func to open the connection.
 * The connection is stored in the SMTP handle.
 * @param smtp SMTP handle.
 * @return On success, returns 0.
 * @return On error, returns -1.
*/
int smtp_connect(SMTP smtp);

/** * @brief Disconnect from the SMTP server.
 *
 * This function calls the SMTP handles close_func to close the connection.
 * The connection is removed from the SMTP handle.
 * @param smtp SMTP handle.
 * @return On success, returns 0.
 * @return On error, returns -1.
*/
int smtp_disconnect(SMTP smtp);

/** * @brief Send a message to the smtp server.
 *
 * This function sends a message using the SMTP handles write_func.
 * @param smtp SMTP handle.
 * @param buffer Ptr to the message to send.
 * @param length Length of the message.
 * @return On success, returns the number of bytes sent.
 * @return On error, returns.
 */
ssize_t smtp_send(SMTP smtp, void* buffer, size_t length);

/** * @brief Read a message from the server
 *
 * This function reads a message using the SMTP handles read_func
 * Due to buffer size limits, only part of a message may be read.
 * @param SMTP SMTP handle.
 * @param buffer Buffer to store the message in.
 * @param length Size of the buffer.
 * @return On success, returns the number of bytes read.
 * @return If the server has shutdown, reaturns 0.
 * @return On error, returns -1.
 */
ssize_t smtp_recv(SMTP smtp, void* buffer, size_t length);

/** * @brief Read a response line from the SMTP server.
 *
 * This function reads a response line from the smtp server.
 * The string is allocated using malloc() and should be freed by the caller.
 * The string is null-terminated which is not included in the returned size.
 * @param smtp SMTP handle.
 * @param line Ptr to the read line.
 * @return On success, returns the number of bytes read.
 * @return On error, returns -1.
 */
int smtp_read_line(SMTP smtp, char** line);

/** * @brief Send a message line to the SMTP server.
 * 
 * This function sends a line to the SMTP server.
 * @param smtp SMTP handle.
 * @param line CRLF and null-terminated string to send.
 * @return number of bytes sent on success
 * @return -1 on error
 */
int smtp_write_line(SMTP smtp, char* line);

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

/** * @brief Generate HELO message.
 *
 * This function generates the HELO message string and sends it to the server.
 * The string is allocated using asprintf and should be freed by the caller.
 * The string is printed to the SMTP handles out file descriptor.
 * @param smtp SMTP handle.
 * @return On success, returns number of bytes writen.
 * @return On error, returns -1.
 */
int smtp_helo(SMTP smtp);

/** * @brief Generate MAIL FROM message.
 *
 * This function generates the MAIL FROM message string and sends it to the server.
 * The string is allocated using asprintf and should be freed by the caller.
 * The string is printed to the SMTP handles out file descriptor.
 * @param smtp SMTP handle.
 * @return On success, returns number of bytes writen.
 * @return On error, returns -1.
 */
int smtp_mail_from(SMTP smtp);

/** * @brief Generate RCPT TO message.
 *
 * This function generates the RCPT TO message string and sends it to the server.
 * The string is allocated using asprintf and should be freed by the caller.
 * The string is printed to the SMTP handles out file descriptor.
 * @param smtp SMTP handle.
 * @return On success, returns number of bytes writen.
 * @return On error, returns -1.
 */
int smtp_rcpt_to(SMTP smtp);

/** * @brief Generate DATA message.
 *
 * This function generates the DATA message string and sends it to the server.
 * The string is printed to the SMTP handles out file descriptor.
 * @param smtp SMTP handle.
 * @return On success, returns number of bytes writen.
 * @return On error, returns -1.
 */
int smtp_data_start(SMTP smtp);

/** * @brief Generate and send SMTP subject line.
 *
 * This function generate the subject line string and sends it to the server.
 * The string is printed to the SMTP handles out file descriptor.
 * @param smtp SMTP handle
 * @return On success, returns number of bytes writen.
 * @return On error, returns -1.
*/
int smtp_subject(SMTP smtp);

/** * @brief Generate a DATA BODY message.
 *
 * This function generates the DATA BODY message string and sends it to the server.
 * The string is allocated using asprintf and should be freed by the caller.
 * The string is printed to the SMTP handles out file descriptor.
 * @param smtp SMTP handle.
 * @return On success, returns number of bytes writen.
 * @return On error, returns -1.
 */
int smtp_data_body(SMTP smtp);

/** * @brief Generate a DATA END message.
 *
 * This function generates the DATA END message stringi and sends it to the server.
 * The string is allocated using asprintf and should be freed by the caller.
 * The string is printed to the SMTP handles out file descriptor.
 * @param smtp SMTP handle.
 * @return On success, returns number of bytes writen.
 * @return On error, returns -1.
 */
int smtp_data_end(SMTP smtp);

/** * @brief Generate a QUIT message.
 *
 * This function generates the QUIT message string and sends it to the server.
 * The string is allocated using asprintf and should be freed by the caller.
 * The string is printed to the SMTP handles out file descriptor.
 * @param smtp SMTP handle.
 * @return On success, returns number of bytes writen.
 * @return On error, returns -1.
 */
int smtp_quit(SMTP smtp);

/** * @brief Check the status code of a server response.
 *
 * This function check the status code in a server response.
 * @param line Line to check.
 * @param statsu Expected status code.
 * @return On match, return 0.
 * @return On missmatch, return -1.
 */
int check_code(char* line, char* status);

/** * @brief Check if a response is multi-line
 *
 * This function checks if the current message is a multiline messsage.
 * @param line Null-terminated string.
 * @return 1 if line is multi-line.
 * @return 0 if the line is not multi-line.
 */
int is_multi_line(char* line);

/** * @brief Listen for responses from the smtp server.
*
* This function listens for a response from the SMTP server with a specific status code.
* All multi-line message are read at once.
* The line recived is printed if the SMTP handles out file descriptor is set.
* @param smtp SMTP handle.
* @param code String containing the expected status code
* @return If all status codes match, return 0.
* @return If any status code does not match, return -1.
*/
int smtp_listen(SMTP smtp, char* code);

/** * @brief Send an email to the smtp server
 * 
 * This function send an email to an smtp server.
 * @param smtp SMTP handle.
 * @return On success, return 0.
 * @return On error, return -1.
*/
int smtp_send_email(SMTP smtp);

#endif // LAB_H
