#include "socketInclude.h"
#include "socketFunctions.h"

void client_receive_file (char * filename, int socket_fd);
void client_send_file (char* filename, int socket_fd);

int main (int argc, char **argv){
    int socket_fd;
    struct sockaddr_in server_addr;
    struct hostent *hp;
    // buffer for getting input from user
    char *buffer;
    char operation;
    size_t bufsize = 32;
    size_t characters;

    /* Allocate memory for buffer */
    buffer = (char *)malloc(bufsize * sizeof(char));
    if( buffer == NULL)
    {
        perror("ERROR socketClient.c: Unable to allocate buffer\n");
        exit(1);
    }
    
    /* Parameter list handling. */
    if (argc < 2){
        perror ("ERROR socketClient.c: Not enough arguments. Server name is needed.\n");
        exit (1);
    }
    else if (argc > 2){
        perror ("ERROR socketClient.c: Too many arguments\n");
        exit (1);
    }
    
    /* Open up a socket. */
    if ((socket_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0){
        perror ("ERROR socketClient.c: Unable to open a socket\n");
        exit (1);
    }
    
    /* Clear the data structure (server_addr) to 0's. */
    memset (&server_addr, 0, sizeof (server_addr));
    
    /* Tell our socket to be of the IPv4 internet family (AF_INET). */
    server_addr.sin_family = AF_INET;
    
    /* Acquire the ip address of the server */
    hp = gethostbyname (argv [1]);
    if (hp == (struct hostent *) NULL){
        printf ("ERROR socketClient.c: gethostbyname: %s does not exist\n", argv [1]);
        exit (1);
    }
    
    /* Copy the host address to the socket data structure. */
    memcpy (&server_addr.sin_addr, hp->h_addr, hp->h_length);
    
    /* Convert the integer Port Number to the network-short standard
     * required for networking stuff. This accounts for byte order differences.*/
    server_addr.sin_port = htons (TCP_PORTNO);
    
    /* Establish a connection to server. */
    if (connect (socket_fd, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0){
        perror ("ERROR socketClient.c: Unable to establish connection\n");
        exit (1);
    }
    
    /*** Ask user the operation ***/
    printf ("Enter SFTP operation. write(w) or read(r)? \n");
    printf ("Operation: ");
    operation = getchar();
    getchar(); // consume the '\n' after operation
    if(operation == 'w'){
        /*** Client send write request to server ***/
        // Get the file name from user
        printf ("Enter the file name to be written to server: \n");
        characters = getline(&buffer,&bufsize,stdin);
        
        // Replace the '\n' in file name by '\0'
        buffer[(int)strlen(buffer)-1] = '\0';
        
        // Receive file from server
        client_send_file (buffer, socket_fd);
    }
    else if (operation == 'r'){
        /*** Client send read request to server ***/
        // Get the file name from user
        printf ("Enter the file name to be read from server: \n");
        characters = getline(&buffer,&bufsize,stdin);
        
        // Replace the '\n' in file name by '\0'
        buffer[(int)strlen(buffer)-1] = '\0';
        
        // Receive file from server
        client_receive_file (buffer, socket_fd);
    }
    else{
        perror ("ERROR socketClient.c: Invalid operation\n");
        exit (1);
    }
    
    /*** Client close the connection with server ***/
    close (socket_fd);
    
    return 0;
}




/*
 Function: client_receive_file
 
 Called by client main()
 
 Purpose:
 Receive file content from server and write to a local file
 */
void client_receive_file (char* server_filename, int socket_fd){
    struct Message message_to_send;
    struct Message *message_received = NULL;
    int filedes;
    char filename[30];
    int i; // loop counter
    int offset = 0;
    int num_of_chars; // number of characters written to a file descriptor
    
    //*** Send rrq message along with the filename
    printf("Sending rrq message to server...\n");
    message_to_send.type = "rrq  "; // strlen(type) returns 5, type ends with '\0'
    message_to_send.data = strdup(server_filename);
    message_to_send.length = (int)strlen(server_filename)+1;
    send_message(&message_to_send, socket_fd);
    
    // Try to open file
    for(i=0 ; i< strlen(server_filename); i++){
        filename[i] = server_filename[i];
    }
    filename[i] = '\0';
    // If the file name is occupied in client side, use another name
    if((filedes = open(filename, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1){
        printf("File name is occupied on client side. New file name starts with c_copy_\n");
        //*** Create a file named c_copy_***.txt
        filename[offset] = 'c'; offset++;
        filename[offset] = '_'; offset++;
        filename[offset] = 'c'; offset++;
        filename[offset] = 'o'; offset++;
        filename[offset] = 'p'; offset++;
        filename[offset] = 'y'; offset++;
        filename[offset] = '_'; offset++;
        for(i=0 ; i< strlen(server_filename); i++){
            filename[offset+i] = server_filename[i];
        }
        filename[offset+i] = '\0';
        if((filedes = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) == -1){
            perror ("ERROR socketClient.c: Failed to open file\n");
        }
        else{
            printf ("socketClient.c: Open file success\n");
        }
    }
    else{
        printf ("socketClient.c: Open file success\n");
    }
    
    //*** Repeatedly receive message from client until line number gets 10 or client receive an eof message
    for ( ; ; ){
        //** expect to received data/eof/fse message
        printf("Expect to receive data/eof/fse message...\n");
        message_received = receive_message(socket_fd);
        //** if client receive an eof message, break the loop, then close file and terminate connection
        if(strcmp(message_received->type, "eof  ") == 0){
            break;
        }
        //** if client received data message
        else if(strcmp(message_received->type, "data ") == 0){
            // write a string to file
            if ((num_of_chars = write_n(filedes, message_received->data, (int)strlen(message_received->data))) != (int)strlen(message_received->data)){
                perror ("ERROR socketClient.c: Unable to write message data to file\n");
                exit (1);
            }
            // send ack message
            printf("Sending ack message to server...\n");
            message_to_send.type = "ack  "; // strlen(type) returns 5, type ends with '\0'
            message_to_send.data = "data ";
            message_to_send.length = (int)strlen(message_to_send.data)+1;
            send_message(&message_to_send, socket_fd);
        }
        //** if client received fse message, which means number of lines exceeds 10
        else if (strcmp(message_received->type, "fse  ") == 0){
            // ask user to continue or abort
            printf("File size exceeeds 10 lines. Do you want to continue? [y/n]\n");
            // if user want to continue
            if(getchar() == 'y'){
                // send cont message
                printf("Sending cont message to server...\n");
                message_to_send.type = "cont "; // strlen(type) returns 5, type ends with '\0'
                message_to_send.data = "fse  ";
                message_to_send.length = (int)strlen(message_to_send.data)+1;
                send_message(&message_to_send, socket_fd);
                // keep reading message from socket
            }
            // if user doesn't want to continue
            else{
                // send abort message
                printf("Sending abort message to server...\n");
                message_to_send.type = "abort"; // strlen(type) returns 5, type ends with '\0'
                message_to_send.data = "fse  ";
                message_to_send.length = (int)strlen(message_to_send.data)+1;
                send_message(&message_to_send, socket_fd);
                // expect to receive ack message
                printf("Expect to receive ack message...\n");
                message_received = receive_message(socket_fd);
                if(!check_message_received(message_received, "ack  ", "abort")){
                    perror ("ERROR socketClient.c: Unexpected received message. Expecting to receive abort message\n");
                    exit (1);
                }
                // delete all the data read from server
                printf("Discarding file...\n");
                // close the file first
                if(close(filedes) == -1){
                    perror ("ERROR socketClient.c: Unable to close file\n");
                    exit (1);
                }
                // use an empty file to overwrite it
                FILE* fp = fopen(filename, "w");
                if(fp == NULL){ // can't open file
                    perror ("ERROR socketClient.c: Unable to open file for discard purpose\n");
                    exit (1);
                }
                else{
                    fclose(fp);
                }
                // terminate connection
                return;
            }
            
        }
        //** if client received a message not eof, data, nor fse; it must be an error
        else{
            perror ("ERROR socketClient.c: Unexpected received message. Expecting to receive data/eof/fse message\n");
            exit (1);
        }
    }// end of infinite loop either by receiving fse or eof message
    
    //*** Server close the file
    if(close(filedes) == -1){
        perror ("ERROR socketClient.c: Unable to close file\n");
        exit (1);
    }
    
    return;
}


/*
 Function: client_send_file
 
 Purpose: write a line in fp to socket and read recv_line from socket
 
 Precondition: filename doesn't contain '\n' and ends with '\0'
 */
void client_send_file (char* filename, int socket_fd){
    char file_line [MAX_LINE_SIZE];
    struct Message message_to_send;
    struct Message *message_received = NULL;
    FILE* fp;
    
    //*** Try to open the file
    fp = fopen(filename, "rb");
    if(fp == NULL){
        OOPS("ERROR Cannot open file\n");
    }
    
    //*** Send wrq message along with the filename
    printf("Sending wrq message to server...\n");
    message_to_send.type = "wrq  "; // strlen(type) returns 5, type ends with '\0'
    message_to_send.data = strdup(filename);
    message_to_send.length = (int)strlen(filename)+1;
    send_message(&message_to_send, socket_fd);
    
    //*** Expect to receive ack message
    printf("Expect to receive ack message...\n");
    message_received = receive_message(socket_fd);
    if(!check_message_received(message_received, "ack  ", "wrq  ")){
        perror ("ERROR socketClient.c: Unexpected received message. Expecting to receive ack message\n");
        exit (1);
    }
    
    //*** Send data message from file to server
    while (fgets (file_line, MAX_LINE_SIZE, fp) != (char *) NULL){
        //** Send message to socket
        // file_line is terminated by '\0'
        printf("Sending data message to server...\n");
        message_to_send.type = "data ";
        message_to_send.data = strdup(file_line);
        message_to_send.length = (int)strlen(file_line)+1;
        send_message(&message_to_send, socket_fd);
        //** Expect to receive ack/fse message
        printf("Expect to receive ack/fse message...\n");
        message_received = receive_message(socket_fd);
        // if received message is fse
        if(check_message_received(message_received, "fse  ", "data ")){
            // send abort message
            printf("Sending abort message to server...\n");
            message_to_send.type = "abort";
            message_to_send.data = "fse  ";
            message_to_send.length = (int)strlen(message_to_send.data)+1;
            send_message(&message_to_send, socket_fd);
            // expect to receive ack message
            printf("Expect to receive ack message...\n");
            message_received = receive_message(socket_fd);
            if(check_message_received(message_received, "ack  ", "abort")){
                perror ("ERROR socketClient.c: Unexpected received message. Expecting to receive ack message\n");
                exit (1);
            }
            // close file
            if(fclose(fp) == -1){
                perror ("ERROR socketClient.c: Unable to close file\n");
                exit (1);
            }
            // terminate connection
            return;
        }
        else if (check_message_received(message_received, "ack  ", "data ")){
            // keep sending message to socket
        }
        else{
            perror ("ERROR socketClient.c: Unexpected received message. Expecting to receive ack or fse message\n");
            exit (1);
        }
    }
    if (ferror (fp)){
        perror ("ERROR socketClient.c: Message error\n");
        exit (1);
    }
    
    //*** Send eof message to server
    printf("Sending eof message to server...\n");
    message_to_send.type = "eof  ";
    message_to_send.data = strdup(filename);
    message_to_send.length = (int)strlen(filename)+1;
    send_message(&message_to_send, socket_fd);
    
    //*** Close file
    if(fclose(fp) == -1){
        perror ("ERROR socketClient.c: Unable to close file\n");
        exit (1);
    }
    
    //*** Expect to receive ack message
    printf("Expect to receive ack message...\n");
    message_received = receive_message(socket_fd);
    if(!check_message_received(message_received, "ack  ", "eof  ")){
        perror ("ERROR socketClient.c: Unexpected received message. Expecting to receive ack message\n");
        exit (1);
    }
    return;
}
