#include "users.h"

#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 

typedef struct USER_TAG {
    int socket;
    char nick[51];
    int id;
    int channel_id;
} USER;

typedef struct USERS_TAG {
    USER **users_list;
    int _n_users;
    int last_id;
} USERS;

USERS *users_init(){
    /* 
    create a USERS handler initialized
     */
    USERS *users = (USERS *) calloc(1, sizeof(USERS));
    users->_n_users = 0;
    users->last_id = 0;
    users->users_list = (USER **) calloc(1, sizeof(USERS *));
    return users;
}

void free_user(USER *user){
    free(user);
}

void finish_users(USERS *users){
    /*
    Free memory of all users
    */
    while(users->_n_users > 0){
        free_user(users->users_list[users->_n_users]);
        users->_n_users--;
    }
    free(users);
}

USER *_create_user(int socket_id, int id){
    USER *user = (USER *) calloc(1, sizeof(USER));
    user->socket = socket_id;
    user->nick[0] = '\0';
    user->id = id;
}

void append_user(USER *user, USERS *users){
    users->_n_users++;
    users->users_list = (USER **) realloc(users->users_list, 
                                                    sizeof(USER *)*users->_n_users);
    users->users_list[users->_n_users-1] = user;
    users->last_id = user->id;
}

int create_user(int socket_id, USERS *users){
    USER *user = _create_user(socket_id, users->last_id+1);
    append_user(user, users);
    return user->id;
}

int delete_user(USERS *users, USER *user){
    if(!users->_n_users) return -2; 

    int i, found = 0;
    for(i=0; i < users->_n_users; i++)
        if(users->users_list[i] == user){
            found = 1;
            break;
        }

    if(!found)
        return -1;

    free_user(user);
    if(i != users->_n_users-1)
        memcpy(&(users->users_list[i]), &(users->users_list[i+1]), users->_n_users-i-1);
    users->_n_users--;
    users->users_list = (USER **) realloc(users->users_list, 
                                                    sizeof(USER *)*users->_n_users);
    return 0;
}

int delete_user_by_id(USERS *users, int id){
    for(int i=0; i < users->_n_users; i++)
        if(users->users_list[i]->id == id)
            return delete_user(users, users->users_list[i]);
    return -1;
}

int get_socket_id(USER *user){
    return user->socket;
}

USER *get_user_by_id(int id, USERS *users){
    printf("\nAqui2 ");
    for(int i=0; i < users->_n_users; i++)
        if(users->users_list[i]->id == id)
            return users->users_list[i];
    printf("NULL\n ");
    return NULL;
}

int get_socket(int user_id, USERS *users){
    USER *user = get_user_by_id(user_id, users);
    return user ? get_socket_id(user) : 0;
}

void set_socket(int user_id, USERS *users, int socket){
    USER *user = get_user_by_id(user_id, users);
    user->socket = socket;
}

void set_channel(int user_id, USERS *users, int channel){
    USER *user = get_user_by_id(user_id, users);
    user->channel_id = channel;
}

int get_channel(int user_id, USERS *users){
    USER *user = get_user_by_id(user_id, users);
    printf("\nAQUI %p %d\n", user, user->channel_id);
    return user ? user->channel_id : 0;
}

int *get_users_id(void **users, int n_users){
    USER **users_list = (USER **) users;
    int *ids = (int *) calloc(n_users, sizeof(int));
    for(int i = 0; i < n_users; i++){
        ids[i] = users_list[i]->id;
    }

    return ids;
}

char *get_nickname(int user_id, USERS *users){
    USER *user = get_user_by_id(user_id, users);
    return user && strlen(user->nick) ? user->nick : NULL;
}

int set_nick(int user_id, USERS *users, char *nick){
    for(int i=0; i < users->_n_users; i++)
        if(!strcmp(users->users_list[i]->nick, nick))
            return -1;
        
    USER *user = get_user_by_id(user_id, users);
    strcpy(user->nick, nick);
    return 0;   
}