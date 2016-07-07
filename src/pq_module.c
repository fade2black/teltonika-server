#include <libpq-fe.h>
#include "hdrs.h"
#include "conf_module.h"
#include "pq_module.h"
#include "logger_module.h"

#define KEYS_NUMBER 3

static PGconn *conn;

void
db_connect()
{
 char keys[KEYS_NUMBER+1][MAX_CONF_STRING_LEN] = {"dbname", "username", "password", ""};
 char values[KEYS_NUMBER][MAX_CONF_STRING_LEN];
 char conn_string[256];

 conf_read("database", keys, values);

 sprintf(conn_string, "user=%s dbname=%s password=%s", values[0], values[1], values[2]);
 conn = PQconnectdb(conn_string);

 if (PQstatus(conn) == CONNECTION_BAD)
 {
    logger_puts("Connection to database failed: %s", PQerrorMessage(conn));
    PQfinish(conn);
    fatal("Connection to database failed: %s", PQerrorMessage(conn));
  }
  else
    logger_puts("Connection to database established successfully");
}

void
db_close()
{
 if (conn)
  PQfinish(conn);
}
