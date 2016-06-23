
#include "hdrs.h"
#include "logger.h"
#include "slots_mng.h"
#include <assert.h>

#define BUF_SIZE 500

#define WAIT_FOR_IMEI 1
#define WAIT_00_01_TOBE_SENT 2
#define WAIT_FOR_DATA_PACKET 3
#define WAIT_NUM_RECIEVED_DATA_TOBE_SENT 4

static char input_buffer[INPUT_BUFSIZE];
struct event_base *base;
static GHashTable* hash;

void process_imei(const char* imei)
{
  logger_puts("processing imei %s...", imei);
}
void process_data_packet(const char* dp)
{
  logger_puts("processing data packet %s...", dp);
}

typedef struct _client_info
{
  char ip_address[INET_ADDRSTRLEN];
  struct bufferevent *bev;
  char state;
} client_info;
static client_info clients[MAXCLIENTS];


static void
add_client(struct bufferevent *bev, char* ip_address)
{
  int empty_slot = get_empty_slot();
  g_hash_table_insert(hash,  GINT_TO_POINTER(bev), GINT_TO_POINTER(empty_slot));
  strcpy(clients[empty_slot].ip_address, ip_address);
  clients[empty_slot].state = WAIT_FOR_IMEI;
  logger_puts("in WAIT_IMEI state");
}

static void
remove_client(struct bufferevent *bev)
{
  int slot = GPOINTER_TO_INT(g_hash_table_lookup(hash, GINT_TO_POINTER(bev)));
  if (slot < 0 || slot > MAXCLIENTS)
  {
    logger_puts("ERROR: slot value (%d) out of range", slot);
    fatal("ERROR: slot value (%d) out of range");
  }

  g_hash_table_remove(hash,  GINT_TO_POINTER(bev));
  put_empty_slot(slot);

  bufferevent_disable(bev, EV_READ | EV_WRITE);
  bufferevent_setcb(bev, NULL, NULL, NULL, NULL);
  bufferevent_free(bev);
}

/***********************************************************/

static void
serv_event_cb(struct bufferevent *bev, short events, void *ctx)
{
  if (events & BEV_EVENT_ERROR)
  {
    logger_puts("Error: bufferevent error");
    fatal("Error from bufferevent");
  }
  if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    bufferevent_free(bev);
}

/****************************************************************************/
static void
serv_read_cb(struct bufferevent *bev, void *ctx)
{
  char accept = 1;
  struct evbuffer *input = bufferevent_get_input(bev);
  /* This callback is invoked when there is data to read on bev. */
  int slot = GPOINTER_TO_INT(g_hash_table_lookup(hash, GINT_TO_POINTER(bev)));
  assert(0 <= slot && slot < MAXCLIENTS);

  if (evbuffer_get_length(input) > INPUT_BUFSIZE)
  {
    logger_puts("ERROR: Insufficient buffer size");
    fatal("ERROR: Insufficient buffer size");
  }

  memset(input_buffer, 0, sizeof(char)*INPUT_BUFSIZE);
  if (bufferevent_read(bev, input_buffer, INPUT_BUFSIZE) == -1)
  {
    logger_puts("Couldn't read data from bufferevent");
    fatal("Couldn't read data from bufferevent");
  }

  if (clients[slot].state == WAIT_FOR_IMEI)
  {
    process_imei(input_buffer);
    /* send 00/01*/
    logger_puts("Sending 'accept module'...");
    if (bufferevent_write(bev, &accept, 1) == -1)
    {
      logger_puts("Couldn't write data to bufferevent");
      fatal("Couldn't write data to bufferevent");
    }
    clients[slot].state = WAIT_00_01_TOBE_SENT;
    puts("in WAIT_00_01_TOBE_SENT state");
  }
  else if (clients[slot].state == WAIT_FOR_DATA_PACKET)
  {
    process_data_packet(input_buffer);
    /* send #data recieved */
    accept = 17;
    if (bufferevent_write(bev, &accept, 1) == -1)
    {
      logger_puts("Couldn't write data to bufferevent");
      fatal("Couldn't write data to bufferevent");
    }
    clients[slot].state = WAIT_NUM_RECIEVED_DATA_TOBE_SENT;
    logger_puts("in WAIT_NUM_RECIEVED_DATA_TOBE_SENT state");
  }
}

/****************************************************************************/
static void
serv_write_cb(struct bufferevent *bev, void *ctx)
{
  int slot = GPOINTER_TO_INT(g_hash_table_lookup(hash, GINT_TO_POINTER(bev)));
  assert(0 <= slot && slot < MAXCLIENTS);

  if (clients[slot].state == WAIT_00_01_TOBE_SENT)
  {
    clients[slot].state = WAIT_FOR_DATA_PACKET;
    puts("in WAIT_FOR_DATA_PACKET state");
  }
  else if (clients[slot].state == WAIT_NUM_RECIEVED_DATA_TOBE_SENT)
  {
    remove_client(bev);
    puts("client removed");
  }
}

/****************************************************************************/
static void
accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ctx)
{
  /* We got a new connection! Set up a bufferevent for it. */
  char ip_address[INET_ADDRSTRLEN];
  struct sockaddr_in* saddr_in = (struct sockaddr_in *) address;

  /* get peer's ip address*/
  if (!inet_ntop(AF_INET, &(saddr_in->sin_addr), ip_address, INET_ADDRSTRLEN))
  {
    logger_puts("Error: inet_ntop failed");
    errExit("inet_ntop");
  }

  logger_puts("A new connection established from %s", ip_address);

  struct event_base *base = evconnlistener_get_base(listener);
  struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

  bufferevent_setcb(bev, serv_read_cb, serv_write_cb, serv_event_cb, NULL);

  add_client(bev, ip_address);

  bufferevent_enable(bev, EV_READ);
}


/****************************************************************************/
static void
accept_error_cb(struct evconnlistener *listener, void *ctx)
{
  struct event_base *base = evconnlistener_get_base(listener);
  int err = EVUTIL_SOCKET_ERROR();
  logger_puts("Got an error %d (%s) on the listener. Shutting down.\n", err, evutil_socket_error_to_string(err));
  event_base_loopexit(base, NULL);
}
/****************************************************************************/




int
main(int argc, char **argv)
{
  struct evconnlistener *listener;
  struct sockaddr_in sin;

  init_empty_slots();

  hash = g_hash_table_new(g_direct_hash, g_direct_equal);
  if (!hash)
  {
    logger_puts("ERROR: Could not create hash table");
    logger_close();
    fatal("ERROR: Could not create hash table");
  }

  int port = 1975;

  if (argc > 1)
    port = atoi(argv[1]);

  logger_open("ttserv.log");

  if (port<=0 || port>65535)
  {
    logger_puts("ERROR: Invalid port");
    logger_close();
    fatal("Invalid port");
  }

  base = event_base_new();
  if (!base)
  {
    logger_puts("ERROR: Couldn't open event base");
    logger_close();
    fatal("Couldn't open event base");
  }

  /* Clear the sockaddr before using it, in case there are extra
   * platform-specific fields that can mess us up. */
  memset(&sin, 0, sizeof(sin));
  /* This is an INET address */
  sin.sin_family = AF_INET;
  /* Listen on 0.0.0.0 */
  sin.sin_addr.s_addr = htonl(0);
  /* Listen on the given port. */
  sin.sin_port = htons(port);

  listener = evconnlistener_new_bind(base, accept_conn_cb, NULL, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
    (struct sockaddr*)&sin, sizeof(sin));

  if (!listener)
  {
    logger_puts("ERROR: Couldn't create listener");
    logger_close();
    fatal("Couldn't create listener");
  }

  evconnlistener_set_error_cb(listener, accept_error_cb);

  event_base_dispatch(base);

  evconnlistener_free(listener);
  event_base_free(base);

  logger_close();
  exit(EXIT_SUCCESS);
}
