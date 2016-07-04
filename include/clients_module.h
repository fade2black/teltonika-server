#ifndef CLIENTS_MODULE
#define CLIENTS_MODULE

#include "hdrs.h"

typedef struct
{
  struct bufferevent *bev;
  char state;
  GByteArray *imei;
  GByteArray *data_packet;
} client_info;


void init_clients_hash();
void add_client(struct bufferevent *bev);
client_info* get_client(struct bufferevent *bev);
void remove_client(struct bufferevent *bev);

#endif
