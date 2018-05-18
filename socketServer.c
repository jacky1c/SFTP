#include "socketInclude.h"
#include "socketFunctions.h"

void server_send_file (char* filename, int socket_fd);
void server_receive_file (int socket_fd, char* filename);

int main (int argc, char **argv){
    int socket_fd;
    int new_socket_fd;
    int addr_length;
    pid_t child_pid;
    struct sockaddr_in client_addr;
    struct sockaddr_in server_addr;
    struct hostent *hp;
    char host_name [256];
    
    /* Open up the socket */
    if ((socket_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0){
        perror ("ERROR socketServer.c: Unable to open a socket\n");
        exit (1);
    }
    
    /* Clear the data structure (server_addr) to 0's. */
    memset (&server_addr, 0, sizeof (server_addr));
    
    /* Tell our socket to be of the IPv4 internet family (AF_INET). */
    server_addr.sin_family = AF_INET;
    
    /* Aquire the name of the current host system (the local machine). */
    gethostname (host_name, sizeof (host_name));
    
    /* Return misc. host information.  Store the results in the
     * hp (hostent) data structure.  */
    hp = gethostbyname (host_name);
    if (hp == (struct hostent *) NULL){
        printf ("ERROR socketServer.c: gethostbyname: %s does not exist\n", host_name);
        exit (1);
    }
    
    /* Copy the host address to the socket data structure. */
    memcpy (&server_addr.sin_addr, hp -> h_addr, hp -> h_length);
    
    /* Convert the integer Port Number to the network-short standard
     * required for networking stuff. This accounts for byte order differences.*/
    server_addr.sin_port = htons (TCP_PORTNO);
    
    /* Register our address with the system. */
    if (bind (socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0){
        perror ("ERROR socketServer.c: Bind error\n");
        exit (1);
    }
    
    /* Tell socket to wait for input.  Queue length is 1. */
    printf ("Parent: Server on port %d\n", ntohs (server_addr.sin_port));
    if (listen (socket_fd, 5) < 0){
        perror ("ERROR socketServer.c: Listen error\n");
        exit (1);
    }
    
    
    for ( ; ; ){
        printf ("Parent: Waiting for client\n");
        addr_length = sizeof (client_addr);
        //*** Wait in the 'accept()' call until a client to make a connection.
        new_socket_fd = accept (socket_fd, (struct sockaddr *) &client_addr, &addr_length);
        
        //*** Connection is established
        printf ("Parent: Client arrived\n");
        if (new_socket_fd < 0){
            perror ("ERROR socketServer.c: Accept error\n");
            exit (1);
        }
        
        //*** Fork a child to communicate with client
        //*** Unable to fork
        if ((child_pid = fork ()) < 0){
            perror ("ERROR socketServer.c: Fork error\n");
            exit (1);
        }
        
        //*** Child process
        // close parent's socket file descriptor
        // read line from socket, print it to screen and write it back to the socket
        else if (child_pid == 0){
            printf ("Child: Fork OK\n");
            close (socket_fd);
            //server_send_file (new_socket_fd);
            
            //--- test ----
            struct Message *message_received = NULL;
            message_received = receive_message(new_socket_fd);
            if(strcmp(message_received->type, "rrq  ") == 0){
                server_send_file(message_received->data, new_socket_fd);
            }
            else if(strcmp(message_received->type, "wrq  ") == 0){
                server_receive_file(new_socket_fd, message_received->data);
            }
            else{
                perror ("ERROR socketServer.c: Unexpected received message. Expecting to receive rrq/wrq message\n");
                exit (1);
            }
            //--- end of test ---
            
            printf ("Child: Done\n\n\n\n\n");
            exit (0);
        }
        
        //*** Parent process stays in 'listen()' status
        else{
            close (new_socket_fd);
        }
    }
}

/*
 Function: server_send_file
 
 Purpose: write a line in fp to socket and read recv_line from socket
 
 Precondition: filename doesn't contain '\n' and ends with '\0'
 */
void server_send_file (char * filename, int socket_fd){
    int n;
    int i; // loop counter. 'for' loop initial declarations are only allowed in C99 mod
    char file_line [MAX_LINE_SIZE];
    struct Message message_to_send;
    struct Message *message_received = NULL;
    int num_of_lines = 0;
    //char * filename;
    FILE* fp;
    
    //*** Try to open the file
    //filename = message_received->data;
    fp = fopen(filename, "rb");
    if(fp == NULL){
        perror ("ERROR socketServer.c: Cannot open file\n");
        exit (1);
    }
    
    //*** Send data message from file to server
    while (fgets (file_line, MAX_LINE_SIZE, fp) != (char *) NULL){
        // if number of lines <= 9 or >= 11
        if(num_of_lines != 10){
            //** Send a string to client
            printf("Sending data message to client...\n");
            message_to_send.type = "data ";
            message_to_send.data = strdup(file_line);
            message_to_send.length = (int)strlen(file_line)+1;
            send_message(&message_to_send, socket_fd);
            //** if the string ends with '\n', increment number of lines
            num_of_lines++;
            //** Expect to receive ack message
            printf("Expect to receive ack message...\n");
            message_received = receive_message(socket_fd);
            if(!check_message_received(message_received, "ack  ", "data ")){
                perror ("ERROR socketServer.c: Unexpected received message. Expecting to receive ack or fse message\n");
                exit (1);
            }
        }
        // if number of lines = 10
        else{
            //** Send fse message to client
            printf("Sending fse message to client...\n");
            message_to_send.type = "fse  ";
            message_to_send.data = "data ";
            message_to_send.length = (int)strlen(message_to_send.data)+1;
            send_message(&message_to_send, socket_fd);
            //** Expect to receive cont/abort message
            printf("Expect to receive cont/abort message...\n");
            message_received = receive_message(socket_fd);
            //** It received cont message, keep sending message to client
            if(strcmp(message_received->type, "cont ") == 0){
                //** Send a string to client
                printf("Sending data message to client...\n");
                message_to_send.type = "data ";
                message_to_send.data = strdup(file_line);
                message_to_send.length = (int)strlen(file_line)+1;
                send_message(&message_to_send, socket_fd);
                //** if the string ends with '\n', increment number of lines
                num_of_lines++;
                //** Expect to receive ack message
                printf("Expect to receive ack message...\n");
                message_received = receive_message(socket_fd);
                if(!check_message_received(message_received, "ack  ", "data ")){
                    perror ("ERROR socketServer.c: Unexpected received message. Expecting to receive ack or fse message\n");
                    exit (1);
                }
            }
            //** It received abort message, send ack message and terminate connection with client
            else if(strcmp(message_received->type, "abort") == 0){
                //** Send ack message to client
                printf("Sending ack message to client...\n");
                message_to_send.type = "ack  ";
                message_to_send.data = "abort";
                message_to_send.length = (int)strlen(message_to_send.data)+1;
                send_message(&message_to_send, socket_fd);
                //** Server close the file
                if(fclose(fp) != 0){
                    perror ("ERROR socketClient.c: Unable to close file\n");
                    exit (1);
                }
                //** Terminate connection
                return;
            }
            else{
                perror ("ERROR socketServer.c: Unexpected received message. Expecting to receive cont or abort message\n");
                exit (1);
            }
        }
        
    } // end of while loop. eof condition or error to read
    if (ferror (fp)){
        perror ("ERROR socketClient.c: Message error\n");
        exit (1);
    }
    
    //*** Send eof message to client
    printf("Sending eof message to server...\n");
    message_to_send.type = "eof  ";
    message_to_send.data = strdup(filename);
    message_to_send.length = (int)strlen(filename)+1;
    send_message(&message_to_send, socket_fd);
    
    //*** Close file
    if(fclose(fp) != 0){
        perror ("ERROR socketClient.c: Unable to close file\n");
        exit (1);
    }
    
    return;
}

/*
 Function: server_receive_file
 Called by server main()
 Purpose:
 Read a line from socket and print to screen. Then write the line back to socket
 If read an empty line, exit function
 */
void server_receive_file (int socket_fd, char * client_filename){
    struct Message message_to_send;
    struct Message *message_received = NULL;
    char filename[30];
    int filedes;
    int i; // loop counter
    int offset = 0;
    int num_of_chars; // number of characters written to a file descriptor
    int num_of_lines = 0; // count how many lines have been received
    
    // Try to open the file
    for(i=0 ; i< strlen(client_filename); i++){
        filename[i] = client_filename[i];
    }
    filename[i] = '\0';
    // If the file name is occupied in client side, use another name
    if((filedes = open(filename, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1){
        filename[offset] = 's'; offset++;
        filename[offset] = '_'; offset++;
        filename[offset] = 'c'; offset++;
        filename[offset] = 'o'; offset++;
        filename[offset] = 'p'; offset++;
        filename[offset] = 'y'; offset++;
        filename[offset] = '_'; offset++;
        for(i=0 ; i< strlen(client_filename); i++){
            filename[offset+i] = client_filename[i];
        }
        filename[offset+i] = '\0';
        if((filedes = open(filename, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) == -1){
            perror ("ERROR socketServer.c: Failed to open file\n");
        }
        else{
            printf ("socketServer.c: Open file success\n");
        }
    }
    else{
        printf ("socketServer.c: Open file success\n");
    }
    
    //*** Send ack message
    printf("Sending ack message to client...\n");
    message_to_send.type = "ack  "; // strlen(type) returns 5, type ends with '\0'
    message_to_send.data = "wrq  ";
    message_to_send.length = (int)strlen(message_to_send.data)+1;
    send_message(&message_to_send, socket_fd);
    
    //*** Repeatedly receive message from client until line number gets 10 or client receive an eof message
    for ( ; ; ){
        // expect to receive data message
        printf("Expect to receive data message...\n");
        message_received = receive_message(socket_fd);
        // if the received message type is not data, it's either an eof or an error
        if(strcmp(message_received->type, "data ") != 0){
            // if server receive an eof message, break the loop
            if(strcmp(message_received->type, "eof  ") == 0){
                break;
            }
            // if server receive a message neither data type nor eof type, it must be an error
            else{
                perror ("ERROR socketServer.c: Unexpected received message. Expecting to receive wrq message\n");
                exit (1);
            }
        }
        // if the received message type is data,
        else{
            // if number of lines <=9
            if(num_of_lines<10){
                // write a string to file
                if ((num_of_chars = write_n(filedes, message_received->data, (int)strlen(message_received->data))) != (int)strlen(message_received->data)){
                    perror ("ERROR socketServer.c: Unable to write message data to file\n");
                    exit (1);
                }
                // if the string ends with "\n\0" increment number of lines
                if(message_received->data[(int)strlen(message_received->data)-1] == '\n'){
                    num_of_lines++;
                }
            }
            // if number of lines >= 10
            else{
                // send fse message
                printf("Sending fse message to client...\n");
                message_to_send.type = "fse  ";
                message_to_send.data = "data ";
                message_to_send.length = (int)strlen(message_to_send.data)+1;
                send_message(&message_to_send, socket_fd);
                // expect to receive abort message
                printf("Expect to receive abort message...\n");
                message_received = receive_message(socket_fd);
                if(!check_message_received(message_received, "abort", "fse  ")){
                    perror ("ERROR socketServer.c: Unexpected received message. Expecting to receive abort message\n");
                    exit (1);
                }
                // discard file
                printf("Discarding file...\n");
                // close the file first
                if(close(filedes) == -1){
                    perror ("ERROR socketServer.c: Unable to close file\n");
                    exit (1);
                }
                // use an empty file to overwrite it
                FILE* fp = fopen(filename, "w");
                if(fp == NULL){ // can't open file
                    perror ("ERROR socketServer.c: Unable to open file for discard purpose\n");
                    exit (1);
                }
                else{
                    fclose(fp);
                }
                return;
            }// end of "if number of lines >= 10"
        }// end of "if the received message type is data"
        
        // send ack message
        printf("Sending ack message to client...\n");
        message_to_send.type = "ack  "; // strlen(type) returns 5, type ends with '\0'
        message_to_send.data = "data ";
        message_to_send.length = (int)strlen(message_to_send.data)+1;
        send_message(&message_to_send, socket_fd);
    } // end of infinite loop because server received eof message
    
    //*** Server received an eof message
    printf("Sending ack message to client...\n");
    message_to_send.type = "ack  "; // strlen(type) returns 5, type ends with '\0'
    message_to_send.data = "eof  ";
    message_to_send.length = (int)strlen(message_to_send.data)+1;
    send_message(&message_to_send, socket_fd);
    
    //*** Server close the file
    if(close(filedes) == -1){
        perror ("ERROR socketServer.c: Unable to close file\n");
        exit (1);
    }
    
    return;
}
