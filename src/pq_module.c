#include <libpq-fe.h>
#include <time.h>
#include "hdrs.h"
#include "conf_module.h"
#include "pq_module.h"
#include "logger_module.h"
#include "parser_module.h"

#define DB_KEYS_NUMBER 3
#define IO_ELEM_KEYS_NUMBER 3

static PGconn *conn;
static int io_ignation_id;
static int io_speed_id;
static int io_odometer_id;


void
db_connect()
{
 char db_keys[DB_KEYS_NUMBER+1][MAX_CONF_STRING_LEN] = {"dbname", "username", "password", ""};
 char db_values[DB_KEYS_NUMBER][MAX_CONF_STRING_LEN];

 char io_elem_keys[IO_ELEM_KEYS_NUMBER + 1][MAX_CONF_STRING_LEN] = {"io_ignation_id", "io_speed_id", "io_odometer_id", ""};
 char io_elem_values[IO_ELEM_KEYS_NUMBER][MAX_CONF_STRING_LEN];

 char conn_string[256];

 conf_read("database", db_keys, db_values);

 sprintf(conn_string, "user=%s dbname=%s password=%s", db_values[0], db_values[1], db_values[2]);
 conn = PQconnectdb(conn_string);

 if (PQstatus(conn) == CONNECTION_BAD)
 {
    logger_puts("Connection to database failed: %s", PQerrorMessage(conn));
    PQfinish(conn);
    fatal("Connection to database failed: %s", PQerrorMessage(conn));
  }
  else
    logger_puts("Connection to database established successfully");

  conf_read("IO Elements", io_elem_keys, io_elem_values);
  io_ignation_id = atoi(io_elem_values[0]);
  io_speed_id = atoi(io_elem_values[1]);
  io_odometer_id = atoi(io_elem_values[2]);
}

/*******************************************************************/
static int retrieve_ignation_value(const io_element* io_elem)
{
  int i;
  for(i = 0; i < io_elem->number_of_1byte_io; i++)
  {
    if (io_elem->one_byte_io[i].id == io_speed_id)
      return io_elem->one_byte_io[i].value;
  }

  return -1;
}

static int retrieve_speed_value(const io_element* io_elem)
{
  int i;
  for(i = 0; i < io_elem->number_of_2byte_io; i++)
  {
    if (io_elem->two_byte_io[i].id == io_speed_id)
      return io_elem->two_byte_io[i].value;
  }

  return -1;
}

static int retrieve_odometer_value(const io_element* io_elem)
{
  int i;
  for(i = 0; i < io_elem->number_of_4byte_io; i++)
  {
    if (io_elem->four_byte_io[i].id == io_odometer_id)
      return io_elem->four_byte_io[i].value;
  }

  return -1;
}
/*******************************************************************/

void db_store_AVL_data_array(const AVL_data_array* data_array)
{
  PGresult *res;
  char query[512];
  char time_str[80];
  struct tm* tminfo;
  int i;
  AVL_data avl_data;

  for(i = 0; i < data_array->number_of_data; i++)
  {
    avl_data = data_array->records[i];

    tminfo = localtime(&avl_data.timestamp);
    strftime(time_str, 80, "%Y-%m-%d %H:%M:%S %z", tminfo);

    sprintf(query, "INSERT INTO avl_records(tmstamp, latitude, longitude, altitude, angle, satellites, speed,\
io_speed, io_odometer, io_ignation) VALUES ('%s', %lf, %lf, %d, %d, %d, %d, %d, %d, %d)",
    time_str,
    avl_data.gps_elem.latitude,
    avl_data.gps_elem.longitude,
    avl_data.gps_elem.altitude,
    avl_data.gps_elem.angle,
    avl_data.gps_elem.satellites,
    avl_data.gps_elem.speed,
    retrieve_speed_value(&avl_data.io_elem),
    retrieve_odometer_value(&avl_data.io_elem),
    retrieve_ignation_value(&avl_data.io_elem)
    );

    res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
      logger_puts("ERROR: %s", PQerrorMessage(conn));
      printf("ERROR: %s\n", PQerrorMessage(conn));
      PQclear(res);
      PQfinish(conn);
      exit(EXIT_FAILURE);
    }

    PQclear(res);
  }
}

/*
void do_exit(PGconn *conn, PGresult *res)
{
  fprintf(stderr, "%s\n", PQerrorMessage(conn));

  PQclear(res);
  PQfinish(conn);

  exit(EXIT_FAILURE);
}

int main()
{

  PGconn *conn = PQconnectdb("user=teltonika dbname=teltonika password=teltonika");
  PGresult *res;

  if (PQstatus(conn) == CONNECTION_BAD)
  {
    fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));

    PQfinish(conn);
    exit(EXIT_FAILURE);
  }

  res = PQexec(conn, "INSERT INTO avl_records(tmstamp, latitude, longitude, altitude, angle, satellites, speed,\
io_speed, io_odometer, io_ignation, created_at, updated_at )\
VALUES('2016-04-12 04:05:06', 25.3032016, 54.7146368, 111, 214, 4, 4, 5, 12345, 1, '2016-07-06 01:46:32', '2016-07-06 01:46:32' )");

  if (PQresultStatus(res) != PGRES_COMMAND_OK)
     do_exit(conn, res);

  PQclear(res);
  PQfinish(conn);

  exit(EXIT_SUCCESS);

}
*/

void
db_close()
{
 if (conn)
  PQfinish(conn);
}
