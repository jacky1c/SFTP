#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define TCP_PORTNO 4259
#define MAX_LINE_SIZE 255
#define OOPS(msg) {perror(msg); exit(1);}
/*char *pname;*/

struct Message{
    char* type;
    char* data;
    int length; // length of data (including '\n' and '\0')
};

void send_message (struct Message *message, int socket_fd);
struct Message * receive_message (int socket_fd);
bool check_message_received(struct Message *message, char* expected_type, char* expected_data);

int read_line (int fd, char *ptr, int line_size);
int write_n (int fd, char *ptr, int n_bytes); // called by send_message and receive_message
int read_n (int fd, char *ptr, int n_bytes); // called by read_line
void print_string(char* str); // display a string including null character

