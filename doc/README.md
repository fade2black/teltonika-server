# ttserv
## Client module
This module is responsible for storing info about connected clients. Data related to a client is represented by the following structure
```
typedef struct
{
  struct bufferevent *bev;
  char state;
  GByteArray *imei;
  GByteArray *data_packet;
} client_info;
```
The pointer `bev` to the `struct bufferevent` uniquely defines an instance of the struct. Client info is stored in a static array of size `MAXCLIENTS` (defined in `global_consts.h`). After info stored in a client info array at location `offset`, the pair <`bev`, `offset`> is stored in a hash table, where `bev` is a key and `offset` is a value.

Client info module defines four methods:
```
void init_clients_hash();
void add_client(struct bufferevent *bev);
client_info* get_client(struct bufferevent *bev);
void remove_client(struct bufferevent *bev);   
```

## Logger module
```
void logger_open(const char* filename);
void logger_puts(const char *format, ...);
void logger_close();
```
## Parser module
`void parse_AVL_data_array(const unsigned char* data_packet, AVL_data_array* data_array);`
