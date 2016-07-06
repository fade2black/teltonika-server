#ifndef _PACKET_PARSER
#define _PACKET_PARSER

#define CODEC_ID 8
#define NUM_OF_DATA 9
#define FISRT_RECORD_OFFSET 10
#define MAX_AVL_RECORDS 25


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
  unsigned char codec_id;
  unsigned char number_of_data;
  AVL_data records[MAX_AVL_RECORDS];
} AVL_data_array;

void parse_AVL_data_array(const unsigned char* data_packet, AVL_data_array* data_array);
/* diagnostics functions */
void print_AVL_data(const AVL_data_array* data_array); /* to stdio */
void print_raw_packet(const unsigned char* data_packet, size_t len); /* to stdio */

#endif
