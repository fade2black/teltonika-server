#ifndef _PACKET_PARSER
#define _PACKET_PARSER

#define CODEC_ID 8
#define NUM_OF_DATA 9
#define FISRT_RECORD_OFFSET 10
#define MAX_AVL_RECORDS 50


typedef struct
{
  double latitude;
  double longitude;
  int altitude;
  int angle;
  unsigned char sattelites;
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
  io_element_node one_byte_io[256];

  unsigned char number_of_2byte_io;
  io_element_node two_byte_io[256];

  unsigned char number_of_4byte_io;
  io_element_node four_byte_io[256];

  unsigned char number_of_8byte_io;
  io_element_node eight_byte_io[256];

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
  unsigned char codec_id;
  unsigned char number_of_data;
  AVL_data data_records[MAX_AVL_RECORDS];
} AVL_data_array;

size_t get_data_length(unsigned char* data_packet);
void parse_gps_element(unsigned char* data_packet, size_t* pos, gps_element* gps_elem);
void parse_io_element(unsigned char* data_packet, size_t* pos, io_element* io_elem);
void parse_AVL_data(unsigned char* data_packet, size_t* pos, AVL_data* avl_data);
void parse_AVL_data_array(unsigned char* data_packet, AVL_data_array* data_array);
void print_avl_data_array(AVL_data_array* data_array); /* to stdio */

#endif
