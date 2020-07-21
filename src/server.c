#include <stdio.h> 

#include <stdlib.h>
#include <string.h> 
#include <time.h>

#include <netdb.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

#include <pthread.h>
#include <signal.h>
#include <sys/types.h> 

#include "users.h"
#include "channels.h"

#define EXIT_CMD "/quit"

#define PORT 8080 
#define SA struct sockaddr 

#define MAX 4096 // max size of a message
#define MAX_CLIENT_NAME 100  // max size of a client name
#define MAX_SEND (MAX+MAX_CLIENT_NAME+2)
  
volatile sig_atomic_t keepRunning; 

pthread_mutex_t mutex, mutex_confirm[20]; // mutex: used to send messages; mutex_confirm: used to accces message confirmed value of a client 
pthread_cond_t condition[20]; // condition: used with <mutex_confirm> to tell when a confirm message is ready
pthread_t threads[20]; // thread ids for each client connected
struct timespec max_wait; 
int TIMEOUT = 4; // max time to wait a confirm message

int clients[20]; // connection id for each client
int clients_confirm[20]; // confirm message for each client

int n = 0, socket_master; // n: number of clients connected counter; socket_master: socket id

USERS *users;
CHANNELS *channels;

int send_ping(char *message, int current){
    /*
        check if a message is equal to "/ping" and, if true, send "pong" back to the client

        message: string to check equals "/ping"
        current: client id

    */
    if(!strncmp("/ping", message, 5)){
        printf("Pinging %d", current);
        if(send(current, "pong\n", strlen("pong\n"), 0) < 0) 
            printf("Sending pong to %d failure \n", current);
        return -1;
    }
    return 0;
}

int close_connection(int id){
    /*
        terminate thread and connection with a client

        id: client id
    */
    int sock = get_socket(id, users);
    printf("Connection timeout, closing %d: %d\n", id, sock);
    pthread_detach(threads[id]);
    
    close(sock);
    return -1;
}

int confirm_send(int id, int len_checker){
    /*
        confirm if a client recived the same amout of bytes that was send

        id: client id
        len_chekcer: amout of bytes sent
    */
    char message_received[MAX_SEND];

    pthread_mutex_lock(&(mutex_confirm[id]));

    max_wait.tv_sec = time(NULL) + TIMEOUT;
    max_wait.tv_nsec = 0;
    // await client answer confirm or TIMEOUT pass.
    pthread_cond_timedwait(&(condition[id]), &(mutex_confirm[id]), &max_wait); 

    if(clients_confirm[id] != len_checker){
        printf("Error: client confirm fail.\n(client received) %d != (sent) %d\n\n", clients_confirm[id], len_checker);
        pthread_mutex_unlock(&(mutex_confirm[id]));
        return 0;
    }
    
    clients_confirm[id] = -1;
    pthread_mutex_unlock(&(mutex_confirm[id]));
    return 1;    
}

int sendtoall(int user_id, char *message_to_send){
    /*
        send a message to all clients connected excepted the sender

        message_to_send: message to be send
        current: client sender
    */
   
    int i;
    pthread_mutex_lock(&mutex);
    int tries = 0, status = -1, confirmed = 0;
    int err = 0, error;
    socklen_t size = sizeof(err);

    int current = get_socket(user_id, users);
    int channel_id = get_channel(user_id, users);
    int n_members = get_n_members(channel_id, channels);
    int *users_ids = get_users_id(get_members(channel_id, channels), n_members);


    for(i = 0; i < n_members; i++) {
        int sock = get_socket(users_ids[i], users);
        if((users_ids[i] != user_id) && (sock != -1)) { // skip sender and check if connection dind't close 
            
            tries = 0;
            
            error = getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &size);
            
            if(!error){ // check if connection is open
                status = send(sock, message_to_send, strlen(message_to_send), 0);
                confirmed = confirm_send(users_ids[i], strlen(message_to_send));
            }
            // if can't send or client can't confirm, try more 4 times
            while((status < 0 || !confirmed) && (tries < 4)) {
                printf("Sending failure to <%d>... Trying again (%d).\n", sock, tries);
                
                error = getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &size);
                if(!error){
                    status = send(sock, message_to_send, strlen(message_to_send), 0);               
                    confirmed = confirm_send(users_ids[i], strlen(message_to_send));
                }
                tries++;
            }
            if(status < 0 || !confirmed){
                // if client connection is broken, close connection
                set_socket(users_ids[i], users, close_connection(users_ids[i]));
            } else {
                printf("Sending success to <%d>\n", sock);
            }
        }
    }

    free(users_ids);
    pthread_mutex_unlock(&mutex);
}

void change_cofirm(char *value, int socket_instance){
    /*
        change message confirm value of a client
        
        value: int as string
        socket: connection id of this client
    */
    int id = -1;
    for(int i = 0; i < 20; i++) // get de id by the connection id
        if(clients[i] == socket_instance)
            id = i;
    
    pthread_mutex_lock(&(mutex_confirm[id]));
    
    clients_confirm[id] = atoi(value);
    pthread_cond_signal(&(condition[id])); // alerting that confirm was updated
    pthread_mutex_unlock(&(mutex_confirm[id]));
}

int check_command(int user_id, char *message){
    /*
        check if a messagem is a internal command, such as:
        /ping
        /confirm (confirme message)

        message: message to check
        socket_instance: current connection id
    */
    char buffer[strlen(message)+1];
    strcpy(buffer, message);
    int socket_instance = get_socket(user_id, users);

    printf("\nChecking command from %d: <%s>", user_id, message);
    if(send_ping(message, socket_instance))
        return -1;
    printf("\n1\n");
    if(!strncmp("/confirm", buffer, 8)){
        change_cofirm(buffer+9, socket_instance);
        return -1;
    }
    printf("\n2\n");
    if(!strncmp("/nickname ", buffer, 10)){
        printf("\n%d Setting nick %s\n", user_id, buffer+10);
        if(set_nick(user_id, users, buffer+10))
            return -4;
        return -1;
    }
    printf("\n3\n");
    if(!get_nickname(user_id, users))
        return -3;

    printf("\n4\n");
    if(!strncmp("/join ", buffer, 6)){
        printf("\n%d Entering into %s\n", user_id, buffer+6);
        int channel_id = enter_or_create_channel(get_user_by_id(user_id, users), 
                                                    buffer+6,
                                                    channels);
        if(channel_id > 0){
            set_channel(user_id, users, channel_id);
            printf("\n5\n");
            return -1;
        }

        return -2;
    } 
    printf("\n6\n");    
    if(get_channel(user_id, users) <= 0)
        return -2;
    printf("\n7\n");
    return 0;
}

void append_nick(int user_id, char *message_to_append, char *dest){
    strcpy(dest, get_nickname(user_id, users));
    strcat(dest, ": ");
    strcat(dest, message_to_append);
}

void *messager_receiver(void *user_id_ptr){
    /*
        recieve all incoming messages from a client

        socket_pointer: client to listen
    */
    int user_id = *((int *)user_id_ptr);
    int socket_instance = get_socket(user_id, users);
    char message_received[MAX_SEND];
    char message_with_nick[MAX_SEND+60];
    int len;

    
    while((len = recv(socket_instance, message_received, MAX_SEND,0)) > 0) {
        message_received[len] = '\0';
        if(!check_command(user_id, message_received)){
            append_nick(user_id, message_received, message_with_nick);
            sendtoall(user_id, message_with_nick);
        }
    }
    pthread_exit(NULL);
}

void intHandler(int signum) {
    /*
        Signal Handler 
    */
    int i;
    keepRunning = 0;
    
    for(i = 0; i < n; i++) //close threads
        if(threads[i] && pthread_kill(threads[i], 0) == 0)
            pthread_detach(threads[i]);
    
    close(socket_master);  // close socket
    exit(0);
}

int main(){ 
    int connfd, len; 
    struct sockaddr_in serveraddress, cli; 
    
    signal(SIGINT, &intHandler);

    // socket create and verification 
    socket_master = socket(AF_INET, SOCK_STREAM, 0); 
    if (socket_master == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&serveraddress, sizeof(serveraddress)); 
  
    // assign IP, PORT 
    serveraddress.sin_family = AF_INET; 
    serveraddress.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    serveraddress.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    int bind_ret;
    if ((bind_ret = bind(socket_master, (SA*)&serveraddress, sizeof(serveraddress))) != 0) { 
        printf("socket bind failed...%d\n", bind_ret); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(socket_master, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    
    len = sizeof(cli); 
    int p_ret = 0;
    
    char connect_cmd[10];
    static const char *compare_cmd = EXIT_CMD;

    users = users_init();
    channels = channels_init();
    keepRunning = 1;
    while(keepRunning){
        if((connfd = accept(socket_master, (SA*)&cli, &len)) < 0)
            printf("accept failed  \n");
        else{
            printf("server acccept the client <%d>...\n", connfd); 
            
            pthread_mutex_lock(&mutex);
            
            clients[n+1]= connfd;
            n++;

            int user_id = create_user(connfd, users);

            // creating a thread for each client

            p_ret = pthread_create(&(threads[n]), NULL, (void *)messager_receiver, &user_id);
            pthread_cond_init(&(condition[n]), NULL);
            pthread_mutex_init(&(mutex_confirm[n]), NULL);   
        
            pthread_mutex_unlock(&mutex);
        }
    }
} 