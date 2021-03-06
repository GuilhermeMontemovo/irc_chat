#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>

#include <netdb.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>


#include <pthread.h>
#include <ctype.h>
#include <signal.h>

#define MAX 4096 
#define MAX_SEND (MAX+2)
#define PORT 8080 
#define SA struct sockaddr 
#define CONNECT_CMD "/connect"

int is_wspace(char *string){
    while(*string != '\0'){
        if (!isspace((unsigned char)*string))
            return 0;
        string++;
    }
    return 1;
}

int check_commands(char *message){
    if(is_wspace(message))
        return -2;
    if(!strncmp("/quit", message, 5))
        return -1;
    if(!strncmp("/ping", message, 5))
        return -3;
    if(!strncmp("/nickname", message, 1))
        return 0;
    if(!strncmp("/join", message, 1))
        return 0;
    if(!strncmp("/", message, 1))
        return -4;
    return 0;
}      
    
void chat(void *socket_pointer){  
    /**
     * Chat: get user inputs, check if is a specifed command, and send to server
     * 
     * socket_poiter: pointer to socket id
     * client_name: string with client name
     */
    int socket_instance = *((int *)socket_pointer);
    char message_read[MAX]; // raw message input
    
    printf("\nEnter the message:\n");

    while (fgets(message_read,MAX,stdin) > 0) { 
        message_read[strlen(message_read)-1] = '\0';
        // check if the command to exit was entered 
        if(check_commands(message_read) == -1)
            break;
        if(check_commands(message_read) == -2)
            continue;
        if(check_commands(message_read) == -4){
            printf("Invalid command. try again:\n");
            continue;
        }
        if(!check_commands(message_read))
            printf("\nEnter the message:\n"); 
        
        if(write(socket_instance, message_read, strlen(message_read)) < 0)
            printf("\nError: message not sent\n");
        
    } 
    pthread_detach(pthread_self());
} 

void confirm_recv(int socket_instance, int len){
    /**
     * Funciont that send to server a confirm message, the lenth recieved
     * 
     * socket_instance: socket id
     * len: lenth to send
     */
    char str[25];
    sprintf(str, "/confirm:%d", len);
    
    if(write(socket_instance, str, strlen(str)) < 0)
        printf("\nError: server confirm fail.\n");
}

void *messager_receiver(void *socket_pointer){
    /*
        recieve all incoming messages from a client

        socket_pointer: cleint to listen
    */
    int socket_instance = *((int *)socket_pointer);
    char message_received[MAX_SEND];
    int len;

    while((len = recv(socket_instance, message_received, MAX_SEND,0)) > 0) {
        confirm_recv(socket_instance, len); // confim to server 
        message_received[len] = '\0';
        fputs(message_received,stdout);
        printf("\nEnter the message:\n");
    }
}

pthread_t thread;
int socket_master;

void intHandler(int signum) {
    /*
        Signal Handler 
    */

    if(pthread_kill(thread, 0) == 0)
        pthread_detach(thread);
    close(socket_master);  // close socket
    exit(0);
}

int main(int argc, char *argv[]) { 
    pthread_t receiver_token;
    int socket_instance, connfd; 
    struct sockaddr_in serveraddress; 
    
    signal(SIGINT, &intHandler);

    // socket create and varification 
    socket_instance = socket(AF_INET, SOCK_STREAM, 0); 
    if (socket_instance == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&serveraddress, sizeof(serveraddress)); 

    char ip_address[18];
    printf("Please, enter \'IP Address\' to bind\n");
    scanf("%[^\n]%*c", ip_address);
    printf("Type \'/connect\' to stabilsh connection with the server on %s\n", ip_address);

    // assign IP, PORT 
    serveraddress.sin_family = AF_INET; 
    serveraddress.sin_addr.s_addr = inet_addr(ip_address); 
    serveraddress.sin_port = htons(PORT); 
    
    char connect_cmd[10];
    static const char *compare_cmd = CONNECT_CMD;
    
    while(fgets(connect_cmd,9,stdin) > 0 && strcmp(connect_cmd, compare_cmd)){
        printf("Invalid command, please type \'/connect\' to stabilsh connection with the server\n");
    }


    // connect the client socket to server socket 
    if (connect(socket_instance, (SA*)&serveraddress, sizeof(serveraddress)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 
    else
        printf("connected to the server...\n"); 

    socket_master = socket_instance;

    //creating thread to always wait for a message
    pthread_create(&receiver_token,NULL,(void *)messager_receiver,&socket_instance);

    thread = receiver_token;
    chat(&socket_instance); 
    close(socket_instance); 
} 