# ttserv
## Client module
This module is responsible for storing info about connected clients. Data related to a client is represented by the following structure
```
typedef struct
{
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
```
typedef struct
{
  double latitude;
  double longitude;
  int altitude;
  int angle;
  unsigned char satellites;
  int speed;
} gps_element;

typedef struct
{
  unsigned char id;
  unsigned long int value;
} io_element_node;

typedef struct
{
  unsigned char event_io_id;
  unsigned char number_of_total_io;

  unsigned char number_of_1byte_io;
  io_element_node one_byte_io[4];

  unsigned char number_of_2byte_io;
  io_element_node two_byte_io[4];

  unsigned char number_of_4byte_io;
  io_element_node four_byte_io[4];

  unsigned char number_of_8byte_io;
  io_element_node eight_byte_io[4];

} io_element;

typedef struct
{
  long int timestamp;
  unsigned char priority;
  gps_element gps_elem;
  io_element io_elem;
} AVL_data;

typedef struct
{
  char imei[50];
  unsigned char codec_id;
  unsigned char number_of_data;
  AVL_data records[MAX_AVL_RECORDS];
} AVL_data_array;
```
`void parse_AVL_data_array(const unsigned char* data_packet, AVL_data_array* data_array);`

## Database
Database engine: `POSTRGRESQL` </br>
Database name: `teltonika`</br>
Table: `avl_records` <br>
- id
- tmstamp
- latitude
- longitude
- altitude
- angle
- satellites
- speed
- io_speed
- io_odometer
- io_ignation
- created_at
- updated_at

#### Query for creating `avl_records` table
```
CREATE TABLE IF NOT EXISTS avl_records (
  id SERIAL PRIMARY KEY,
  tmstamp TIMESTAMP,
  latitude NUMERIC(8,6),
  longitude NUMERIC(8,6),
  altitude SMALLINT,
  angle SMALLINT,
  satellites SMALLINT,
  speed SMALLINT,
  io_speed SMALLINT,
  io_odometer INTEGER,
  io_ignation SMALLINT,
  created_at TIMESTAMP,
  updated_at TIMESTAMP
);
```
