
#include "hdrs.h"
#include "logger.h"
#include "slots_mng.h"
#define BUF_SIZE 500

typedef struct _client_info
{
  char ip_address[INET_ADDRSTRLEN];
  struct bufferevent *bev;
} client_info;

typedef struct _broadcast_info
{
  char ip_address[INET_ADDRSTRLEN]; /* whos sends */
  char mes[INPUT_BUFSIZE];
} broadcast_info;


struct event_base *base;
static GHashTable* hash;
static client_info clients[MAXCLIENTS];
static char input_buffer[INPUT_BUFSIZE];


static void
add_client(struct bufferevent *bev, char* ip_address)
{
  int empty_slot = get_empty_slot();
  g_hash_table_insert(hash,  GINT_TO_POINTER(bev), GINT_TO_POINTER(empty_slot));
  strcpy(clients[empty_slot].ip_address, ip_address);
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
echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
  if (events & BEV_EVENT_ERROR)
  {
    logger_puts("Error: bufferevent error");
    perror("Error from bufferevent");
  }
  if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    bufferevent_free(bev);
}


static void
echo_read_cb(struct bufferevent *bev, void *ctx)
{
  /* This callback is invoked when there is data to read on bev. */
  struct evbuffer *input = bufferevent_get_input(bev);
  int slot;

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
  input_buffer[strlen(input_buffer) - 2] = '\0';

  slot = GPOINTER_TO_INT(g_hash_table_lookup(hash, GINT_TO_POINTER(bev)));
  printf("%s: %s", clients[slot].ip_address, input_buffer);
  /*g_hash_table_foreach(hash, broadcast, &bi);*/
  if (!strcmp(input_buffer, "exit"))
    remove_client(bev);
}

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

  printf("A new connection established from %s", ip_address);
  /*logger_puts("A new connection established from %s", ip_address);*/

  struct event_base *base = evconnlistener_get_base(listener);
  struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

  bufferevent_setcb(bev, echo_read_cb, echo_write_cb, echo_event_cb, NULL);

  add_client(bev, ip_address);

  bufferevent_enable(bev, EV_READ | EV_WRITE);
}

static void
accept_error_cb(struct evconnlistener *listener, void *ctx)
{
  char log_mesg[BUF_SIZE];
  struct event_base *base = evconnlistener_get_base(listener);
  int err = EVUTIL_SOCKET_ERROR();

  sprintf(log_mesg, "Got an error %d (%s) on the listener. Shutting down.\n", err, evutil_socket_error_to_string(err));
  logger_puts(log_mesg);
  logger_close();

  event_base_loopexit(base, NULL);
}





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

  logger_close();
  exit(EXIT_SUCCESS);
}
