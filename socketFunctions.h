/*
 Read from file descriptor up to [line_size] characters or when '\0' is reached, one at a time.
 Store characters to ptr
 Return the number of characters read in (including '\0')
 ptr is also returned
 */
int read_line (int fd, char *ptr, int line_size){
    int n;
    int rc;
    char c;
    
    for (n = 1; n <= line_size; n++){
        // if we successfully get 1 character, append it to the end of ptr
        // if the character is '\0', return n
        if ((rc = read_n (fd, &c, 1)) == 1){
            *ptr++ = c;
            if (c == '\0'){
                break;
            }
        }
        // if we get 0 character by calling read_n (fd, &c, 1)
        else if (rc == 0){
            // if this is the 1st loop, we get nothing
            if (n == 1){
                return (0);
            }
            // if this is not the 1st loop, we reach the end of string in file descriptor
            // string length is less than line_size and the string doesn't end with '0'
            // append '\0' to the end of the string
            else{
                *ptr++ = '\0';
                break;
            }
        }
        // error
        else{
            return (-1);
        }
    }
    
    return (n);
}

// Write n bytes of characters in ptr to file descriptor.
int write_n (int fd, char *ptr, int n_bytes){
    int n_left;
    int n_written;
    
    n_left = n_bytes;
    
    while (n_left > 0){
        n_written = write (fd, ptr, n_left);
        if (n_written <= 0){
            return (n_written);
        }
        n_left = n_left - n_written;
        ptr = ptr + n_written;
    }
    
    return (n_bytes - n_left);
}

// Called by read_line
// Read n bytes of characters from file descriptor and store in ptr
int read_n (int fd, char *ptr, int n_bytes){
    int n_left;
    int n_read;
    
    n_left = n_bytes;
    
    while (n_left > 0){
        n_read = read (fd, ptr, n_left);
        if (n_read < 0){
            return (n_read);
        }
        else if (n_read == 0){
            break;
        }
        n_left = n_left - n_read;
        ptr = ptr + n_read;
    }
    
    return (n_bytes - n_left);
}

/*
 Function: send_message
 
 Purpose: send message's type and data to file descriptor (including '\n' and '\0')
 */
void send_message (struct Message *message, int socket_fd){
    int i;
    
    // Display send_message info
    printf("--- send_message ---\n");
    
    // Write message type to socket
    if ((i = write_n(socket_fd, message->type, (int)strlen(message->type)+1)) != ((int)strlen(message->type)+1)){
        perror ("ERROR send_message: Unable to send message type to socket\n");
        exit (1);
    }
    print_string(message->type);
    
    // Write message data to socket
    if ((i = write_n(socket_fd, message->data, (int)strlen(message->data)+1)) != ((int)strlen(message->data)+1)){
        perror ("ERROR send_message: Unable to send message data to socket\n");
        exit (1);
    }
    print_string(message->data);
    
    printf("--- end of send_message ---\n");
    
    return;
}

/*
 Function: send_message
 
 Purpose: send message's type and data to file descriptor (including '\n' and '\0')
 */
struct Message * receive_message (int socket_fd){
    int n;
    char line [MAX_LINE_SIZE];
    
    // Display send_message info
    printf("--- receive_message ---\n");
    
    // Allocate memory for message
    struct Message * message = (struct Message*)malloc(sizeof(struct Message));
    if( message == NULL){
        perror("ERROR receive_message: Unable to allocate message\n");
        exit(1);
    }
    
    // Read message type from socket
    n = read_line (socket_fd, line, MAX_LINE_SIZE);
    printf ("receive_message: Reads %d characters for message type\n", n);
    if (n == 0){
        perror ("ERROR receive_message: Error getting message type\n");
        exit (1);
    }
    if (n < 0){
        perror ("ERROR receive_message ERROR\n");
        exit (1);
    }
    message->type = strdup(line);
    print_string(message->type);
    
    // Read message data from socket
    n = read_line (socket_fd, line, MAX_LINE_SIZE);
    printf ("receive_message: Reads %d characters for message data\n", n);
    if (n == 0){
        perror ("ERROR receive_message: Read nothing for message data\n");
        exit (1);
    }
    if (n < 0){
        perror ("ERROR receive_message: Error getting message data\n");
        exit (1);
    }
    message->data = strdup(line);
    print_string(message->data);
    
    // Get the message length (including '\0')
    message->length = (int)strlen(line)+1;
    printf("Message length = %d\n", message->length);
    
    printf("--- end of receive_message ---\n");
    
    return message;
}

/*
 Check if the message received matches what we expect.
 If yes, return true. Otherwise, return false
 */
bool check_message_received(struct Message *message, char* expected_type, char* expected_data){
    if(strcmp(message->type, expected_type) != 0){
        return false;
    }
    if (strcmp(message->data, expected_data) != 0){
        return false;
    }
    return true;
}

/*
 Call strlen() to get the length of string (including '\n' but not '\0')
 Print every single character in the string (including '\n' and '\0')
 */
void print_string(char* str){
    int i;
    if(str != NULL){
        printf("length of str = %d\n", (int)strlen(str));
        for(i=0; i<=(int)strlen(str); i++){
            if (str[i] == '\n'){
                printf("'\\n'");
            }
            else if (str[i] == '\0'){
                printf("'\\0'");
            }
            else{
                printf("%c", str[i]);
            }
        }
        printf("\n");
    }
    else{
        printf("Empty string\n");
    }
}


