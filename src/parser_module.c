#include <time.h>
#include "hdrs.h"
#include "parser_module.h"

static void
parse_gps_element(const unsigned char* data_packet, size_t* pos, gps_element* gps_elem)
{
  int int_val, i;
  size_t index = *pos;

  /* Longitude */
  int_val = data_packet[index++];
  for(i = 0; i < 3; i++)
  {
    int_val <<= 8;
    int_val |= data_packet[index++];
  }
  gps_elem->longitude = int_val/10000000.0;

  /* Latitude */
  int_val = data_packet[index++];
  for(i = 0; i < 3; i++)
  {
    int_val <<= 8;
    int_val |= data_packet[index++];
  }
  gps_elem->latitude = int_val/10000000.0;


  /* Altitude */
  gps_elem->altitude = data_packet[index++];
  gps_elem->altitude <<= 8;
  gps_elem->altitude |= data_packet[index++];

  /* Angle */
  gps_elem->angle = data_packet[index++];
  gps_elem->angle <<= 8;
  gps_elem->angle |= data_packet[index++];

  /* satellites */
  gps_elem->satellites =  data_packet[index++];

  /* Speed */
  gps_elem->speed = data_packet[index++];
  gps_elem->speed <<= 8;
  gps_elem->speed |= data_packet[index++];

  *pos = index;
}
/******************************************************************************/
static void
parse_io_element(const unsigned char* data_packet, size_t* pos, io_element* io_elem)
{
  int i, j;
  size_t index = *pos;

  io_elem->event_io_id = data_packet[index++];
  io_elem->number_of_total_io = data_packet[index++];

  /* 1-byte values */
  io_elem->number_of_1byte_io = data_packet[index++];
  for(i = 0; i < io_elem->number_of_1byte_io; i++)
  {
    io_elem->one_byte_io[i].id = data_packet[index++];
    io_elem->one_byte_io[i].value = data_packet[index++];
  }

  /* 2-byte values */
  io_elem->number_of_2byte_io = data_packet[index++];
  for(i = 0; i < io_elem->number_of_2byte_io; i++)
  {
    io_elem->two_byte_io[i].id = data_packet[index++];
    io_elem->two_byte_io[i].value = data_packet[index++];
    io_elem->two_byte_io[i].value <<= 8;
    io_elem->two_byte_io[i].value |= data_packet[index++];
  }

  /* 4-byte values */
  io_elem->number_of_4byte_io = data_packet[index++];
  for(i = 0; i < io_elem->number_of_4byte_io; i++)
  {
    io_elem->four_byte_io[i].id = data_packet[index++];
    io_elem->four_byte_io[i].value = data_packet[index++];
    for( j = 0; j < 3; j++ )
    {
      io_elem->four_byte_io[i].value <<= 8;
      io_elem->four_byte_io[i].value |= data_packet[index++];
    }
  }

  /* 8-byte values */
  io_elem->number_of_8byte_io = data_packet[index++];
  for(i = 0; i < io_elem->number_of_8byte_io; i++)
  {
    io_elem->eight_byte_io[i].id = data_packet[index++];
    io_elem->eight_byte_io[i].value = data_packet[index++];
    for( j = 0; j < 7; j++ )
    {
      io_elem->eight_byte_io[i].value <<= 8;
      io_elem->eight_byte_io[i].value |= data_packet[index++];
    }
  }

  *pos = index;
}
/******************************************************************************/
/*  AVL data:  | <timestamp> | <priority> | <GPS element> | <IO Element> |  */
static void
parse_AVL_data(const unsigned char* data_packet, size_t* pos, AVL_data* avl_data)
{
  int i;
  long int timestamp;
  gps_element gps_elem;
  io_element io_elem;
  size_t index = *pos;

  /* parse timestamp */
  timestamp = data_packet[index++];
  for(i = 0; i < 7; i++)
  {
    timestamp <<= 8;
    timestamp |= data_packet[index++];
  }
  avl_data->timestamp = timestamp / 1000;
  /* Priority */
  avl_data->priority = data_packet[index++];
  /* GPS Element */
  parse_gps_element(data_packet, &index, &gps_elem);
  avl_data->gps_elem = gps_elem;
  /* IO Element */
  parse_io_element(data_packet, &index, &io_elem);
  avl_data->io_elem = io_elem;

  *pos = index;
}

/******************************************************************************/
void
parse_AVL_data_array(const unsigned char* data_packet, AVL_data_array* data_array)
{
  size_t position = FISRT_RECORD_OFFSET;
  AVL_data avl_data;
  int i;

  data_array->number_of_data = data_packet[NUM_OF_DATA];
  data_array->codec_id       = data_packet[CODEC_ID];

  for(i = 0; i < data_array->number_of_data; i++)
  {
    parse_AVL_data(data_packet, &position, &avl_data);
    data_array->records[i] = avl_data;
  }
}

/******************************************************************************/
/* for diagnostics purpose */
void
print_raw_packet(const unsigned char* data_packet, size_t len)
{
  int i;
  for(i = 0; i < len; i++)
    printf("%02x|", data_packet[i]);
  printf("\n");
}

/******************************************************************************/
/* for diagnostics purpose */
void
print_AVL_data(const AVL_data_array* data_array)
{
  int i, j;
  char buffer[80];
  struct tm* tminfo;
  AVL_data avl_data;

  printf("Codec ID: %d\n", data_array->codec_id);
  printf("Number of data: %d\n", data_array->number_of_data);

  for(i = 0; i < data_array->number_of_data; i++)
  {
    printf(" Data %d\n", i+1);
    avl_data = data_array->records[i];

    tminfo = localtime(&avl_data.timestamp);
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S %z", tminfo);
    printf("   Timestamp: %s\n", buffer);
    printf("   Priority: %d\n", avl_data.priority);
    /* GPS element*/
    printf("   GPS Element\n");
    printf("     Latitude: %lf\n", avl_data.gps_elem.latitude);
    printf("     Longitude: %lf\n", avl_data.gps_elem.longitude);
    printf("     Altitude: %d\n", avl_data.gps_elem.altitude);
    printf("     Angle: %d\n", avl_data.gps_elem.angle);
    printf("     satellites: %d\n", avl_data.gps_elem.satellites);
    printf("     Speed: %d\n", avl_data.gps_elem.speed);
    /* IO Element */
    printf("   IO Element\n");
    printf("     Event IO ID: %d\n", avl_data.io_elem.event_io_id);
    printf("     #total IO: %d\n", avl_data.io_elem.number_of_total_io);
    /* 1-byte */
    printf("     #1-byte IO: %d\n", avl_data.io_elem.number_of_1byte_io);
    for(j = 0; j < avl_data.io_elem.number_of_1byte_io; j++)
      printf("      (id:%d, val:%lu) ", avl_data.io_elem.one_byte_io[j].id, avl_data.io_elem.one_byte_io[j].value);
    printf("\n");
    /* 2-byte */
    printf("     #2-byte IO: %d\n", avl_data.io_elem.number_of_2byte_io);
    for(j = 0; j < avl_data.io_elem.number_of_2byte_io; j++)
      printf("      (id:%d, val:%lu) ", avl_data.io_elem.two_byte_io[j].id, avl_data.io_elem.two_byte_io[j].value);
    printf("\n");
    /* 4-byte */
    printf("     #4-byte IO: %d\n", avl_data.io_elem.number_of_4byte_io);
    for(j = 0; j < avl_data.io_elem.number_of_4byte_io; j++)
      printf("      (id:%d, val:%lu) ", avl_data.io_elem.four_byte_io[j].id, avl_data.io_elem.four_byte_io[j].value);
    printf("\n");
    /* 8-byte */
    printf("     #8-byte IO: %d\n", avl_data.io_elem.number_of_8byte_io);

    printf("--------------------------------------------\n");
  }
}
