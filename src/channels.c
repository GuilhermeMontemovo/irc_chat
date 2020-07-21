#include "channels.h"
#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 

typedef struct CHANNEL_TAG {
    void **members;
    int _n_members;
    void *admin;
    char *name;
    int id;
} CHANNEL;

typedef struct CHANNELS_TAG {
   CHANNEL **channels_list;
   int _n_channels;
   int last_id;
} CHANNELS;

void append_channel(CHANNEL *channel, CHANNELS *channels){
    channels->_n_channels++;
    channels->channels_list = (CHANNEL **) realloc(channels->channels_list, 
                                                    sizeof(CHANNEL *)*channels->_n_channels);
    channels->channels_list[channels->_n_channels-1] = channel;
    channels->last_id = channel->id;
}

int create_channel(CHANNELS *channels){
    CHANNEL *channel = (CHANNEL *) calloc(1, sizeof(CHANNEL));
    channel->id = channels->last_id + 1;
    append_channel(channel, channels);
    return channel->id;
}

void free_channel(CHANNEL *channel){
    free(channel->members);
    free(channel->name);
    free(channel);
}

int delete_channel(CHANNELS *channels, CHANNEL *channel){
    if(!channels->_n_channels) return -2; 

    int i, found = 0;
    for(i=0; i < channels->_n_channels; i++)
        if(channels->channels_list[i] == channel){
            found = 1;
            break;
        }

    if(!found)
        return -1;

    free_channel(channel);
    if(i != channels->_n_channels-1)
        memcpy(&(channels->channels_list[i]), &(channels->channels_list[i+1]), channels->_n_channels-i-1);
    channels->_n_channels--;
    channels->channels_list = (CHANNEL **) realloc(channels->channels_list, 
                                                    sizeof(CHANNEL *)*channels->_n_channels);
    return 0;
}

int delete_channel_by_id(CHANNELS *channels, int id){
    for(int i=0; i < channels->_n_channels; i++)
        if(channels->channels_list[i]->id == id)
            return delete_channel(channels, channels->channels_list[i]);
    return -1;
}

CHANNELS *channels_init(){
    /* 
    create a CHANNELS handler initialized
     */
    CHANNELS *channels = (CHANNELS *) calloc(1, sizeof(CHANNELS));
    channels->_n_channels = 0;
    channels->last_id = 0;
    channels->channels_list = (CHANNEL **) calloc(1, sizeof(CHANNEL *));;
    return channels;
}

void finish_channels(CHANNELS *channels){
    /*
    Free memory of all channels of a channel and itseld
    */
    while(channels->_n_channels > 0){
        free_channel(channels->channels_list[channels->_n_channels]);
        channels->_n_channels--;
    }
    free(channels);
}

void add_member(void *member, CHANNEL *channel){
    channel->_n_members++;
    if(channel->_n_members == 1){
        channel->members = (void **) calloc(1, sizeof(void *));
        channel->admin = member;
    } else
        channel->members = (void **) realloc(channel->members, sizeof(void *)*(channel->_n_members));

    channel->members[channel->_n_members-1] = member;
}

int add_member_to_channel(void *member, int channel_id, CHANNELS *channels){
    if(channel_id > channels->last_id) return -1;
    printf("\nAqui3\n");
    int i, found = 0;

    for(i=0; i < channels->_n_channels; i++)
        if(channels->channels_list[i]->id == channel_id){
            found = 1;
            break;
        }

    if(!found) return -2;
    add_member(member, channels->channels_list[i]);
    return 0;   
}

int get_channel_id(char *name, CHANNELS *channels){
    for(int i=0; i < channels->_n_channels; i++)
        if(!strcmp(channels->channels_list[i]->name, name))
            return channels->channels_list[i]->id;
        
    return 0;
}

CHANNEL *get_channel_by_id(int id, CHANNELS *channels){
    for(int i=0; i < channels->_n_channels; i++)
        if(channels->channels_list[i]->id == id)
            return channels->channels_list[i];
    return NULL;    
}

void set_channel_name(int channel_id, CHANNELS *channels, char *name){
    printf("\n------ Setting channel %d name to %s\n", channel_id, name);
    CHANNEL *channel = get_channel_by_id(channel_id, channels);
    channel->name = (char *) realloc(channel->name, sizeof(char)*(strlen(name)+1));
    strcpy(channel->name, name);
    printf("chanel name: %s", channel->name);
}

int enter_or_create_channel(void *member, char *name, CHANNELS *channels){
    printf("\n------------- CREATING OR ENTER A CHANNEL \n");
    printf("\n%p, %s, %p\n", member, name, channels);
    int id = get_channel_id(name, channels);
    printf("get id %d\n", id);
    if(!id){
        id = create_channel(channels);
        printf("new id %d\n", id);
        set_channel_name(id, channels, name);
    }

    int status = add_member_to_channel(member, id, channels);
    printf("status %d\n", status);
    if(status)
        return -1;
    return id;
}

void **get_members(int channel_id, CHANNELS *channels){
    CHANNEL *channel = get_channel_by_id(channel_id, channels);
    return channel ? channel->members : NULL;
}

int get_n_members(int channel_id, CHANNELS *channels){
    CHANNEL *channel = get_channel_by_id(channel_id, channels);
    return channel ? channel->_n_members : 0;
}
