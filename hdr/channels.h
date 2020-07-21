#ifndef CHANNELS_H_
#define CHANNELS_H_

typedef struct CHANNELS_TAG CHANNELS;

int enter_or_create_channel(void *member, char *name, CHANNELS *channels);
void **get_members(int channel_id, CHANNELS *channels);
int get_n_members(int channel_id, CHANNELS *channels);
CHANNELS *channels_init();
#endif