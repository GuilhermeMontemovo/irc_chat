#ifndef USERS_H_
#define USERS_H_

typedef struct USERS_TAG USERS;
typedef struct USER_TAG USER;

USERS *users_init();
USER *get_user_by_id(int , USERS *);
int create_user(int, USERS *);
int get_socket(int, USERS*);

int set_nick(int, USERS *, char *);
int get_channel(int , USERS *);
void set_socket(int , USERS *, int );
void set_channel(int , USERS *, int );
int *get_users_id(void **, int );
int get_socket(int , USERS *);

char *get_nickname(int , USERS *);
#endif