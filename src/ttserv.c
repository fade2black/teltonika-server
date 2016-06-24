
#include "hdrs.h"
#include "logger.h"
#include "slots_mng.h"
#include <assert.h>

#define BUF_SIZE 500

#define WAIT_FOR_IMEI 1
#define WAIT_00_01_TOBE_SENT 2
#define WAIT_FOR_DATA_PACKET 3
#define WAIT_NUM_RECIEVED_DATA_TOBE_SENT 4

#define NUM_OF_DATA 9

static unsigned char input_buffer[INPUT_BUFSIZE];
struct event_base *base;
static GHashTable* hash;


typedef struct _client_info
{
  struct bufferevent *bev;
  char state;
  GByteArray *imei;
  GByteArray *data_packet;
} client_info;
static client_info clients[MAXCLIENTS];


/* if all bytes of imei are read then return TRUE
   else return FALSE */
static int
process_imei(const unsigned char* data, size_t nbytes, int slot)
{
  size_t length;
  size_t num_of_read_bytes;

  logger_puts("processing imei...");

  /* append bytes to imei */
  g_byte_array_append(clients[slot].imei, (guint8*)data, nbytes);
  num_of_read_bytes = clients[slot].imei->len;

  if (num_of_read_bytes > 1)
  {
    /* more than two bytes have already been read, so we can check
       whether or not we have read the entire message */
    length = clients[slot].imei->data[0];
    length <<= 8;
    length |= clients[slot].imei->data[1];

    if ((num_of_read_bytes - 2) < length)
      return FALSE;
    else if ((num_of_read_bytes - 2) == length)
      return TRUE;
    else
    {
      logger_puts("ERROR: %s, '%s', line %d, number of bytes read is greater than indicated value in the IMEI message (first two bytes)", __FILE__, __func__, __LINE__);
      fatal("ERROR: %s, '%s', line %d, number of bytes read is greater than indicated value in the IMEI message (first two bytes)", __FILE__, __func__, __LINE__);
    }
  }
  return FALSE;
}

static int
process_data_packet(const unsigned char* data, size_t nbytes, int slot)
{
  size_t length;
  size_t num_of_read_bytes;

  logger_puts("processing data packet");

  g_byte_array_append(clients[slot].data_packet, (guint8*)data, nbytes);
  num_of_read_bytes = clients[slot].data_packet->len;

  if (num_of_read_bytes > 7) /* if at least 8 bytes are recieved */
  {
    length = clients[slot].data_packet->data[4];
    length <<= 8;
    length |= clients[slot].data_packet->data[5];
    length <<= 8;
    length |= clients[slot].data_packet->data[6];
    length <<= 8;
    length |= clients[slot].data_packet->data[7];

    logger_puts("  length: %zd, num_of_read_bytes: %zd", length, num_of_read_bytes);

    if (num_of_read_bytes < (length + 12))
      return FALSE;
    else if (num_of_read_bytes == (length + 12))
      return TRUE;
    else
    {
      logger_puts("ERROR: %s, '%s', line %d, number of bytes read is greater than indicated value in the data packet (first four bytes)", __FILE__, __func__, __LINE__);
      fatal("ERROR: %s, '%s', line %d, number of bytes read is greater than indicated value in the data packet (first four bytes)", __FILE__, __func__, __LINE__);
    }
  }

  return FALSE;
}




static void
add_client(struct bufferevent *bev)
{
  int empty_slot = get_empty_slot();
  g_hash_table_insert(hash,  GINT_TO_POINTER(bev), GINT_TO_POINTER(empty_slot));

  clients[empty_slot].state = WAIT_FOR_IMEI;

  /* allocate memmory */
  clients[empty_slot].imei = g_byte_array_new();
  assert(clients[empty_slot].imei != NULL);
  clients[empty_slot].data_packet = g_byte_array_new();
  assert(clients[empty_slot].data_packet != NULL);

  logger_puts("in WAIT_IMEI state");
}

static void
remove_client(struct bufferevent *bev)
{
  int slot = GPOINTER_TO_INT(g_hash_table_lookup(hash, GINT_TO_POINTER(bev)));
  if (slot < 0 || slot > MAXCLIENTS)
  {
    logger_puts("ERROR: %s, '%s', line %d, slot value (%d) out of range", __FILE__, __func__, __LINE__, slot);
    fatal("ERROR: slot value (%d) out of range");
  }

  /* free allocated memories */
  g_byte_array_free (clients[slot].imei, TRUE);
  g_byte_array_free (clients[slot].data_packet, TRUE);
  /*******************************************/

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
    logger_puts("ERROR: %s, '%s', line %d, bufferevent error", __FILE__, __func__, __LINE__);
    fatal("Error from bufferevent");
  }
  if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    bufferevent_free(bev);
}

/****************************************************************************/
static void
serv_read_cb(struct bufferevent *bev, void *ctx)
{
  size_t accept;
  struct evbuffer *input = bufferevent_get_input(bev);
  size_t nbytes;

  int slot = GPOINTER_TO_INT(g_hash_table_lookup(hash, GINT_TO_POINTER(bev)));
  assert(0 <= slot && slot < MAXCLIENTS);

  if (evbuffer_get_length(input) > INPUT_BUFSIZE)
  {
    logger_puts("ERROR: %s, '%s', line %d, insufficient buffer size", __FILE__, __func__, __LINE__);
    fatal("ERROR: Insufficient buffer size");
  }

  memset(input_buffer, 0, sizeof(char)*INPUT_BUFSIZE);
  nbytes = bufferevent_read(bev, input_buffer, INPUT_BUFSIZE);

  if (nbytes == -1)
  {
    logger_puts("ERROR: %s, '%s', line %d, couldn't read data from bufferevent", __FILE__, __func__, __LINE__);
    fatal("ERROR: %s, '%s', line %d, couldn't read data from bufferevent", __FILE__, __func__, __LINE__);
  }

  if (clients[slot].state == WAIT_FOR_IMEI)
  {
    /* if process_imei returns TRUE then imei are read entirely,
       otherwise stay in the WAIT_FOR_IMEI state*/
    if (process_imei(input_buffer, nbytes, slot))
    {
      /* send 00/01*/
      logger_puts("Sending 'accept module'...");
      if (bufferevent_write(bev, &accept, 1) == -1)
      {
        logger_puts("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
        fatal("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
      }
      clients[slot].state = WAIT_00_01_TOBE_SENT;
      logger_puts("in WAIT_00_01_TOBE_SENT state");
    }
  }
  else if (clients[slot].state == WAIT_FOR_DATA_PACKET)
  {
    if (process_data_packet(input_buffer, nbytes, slot))/* if entire AVL packet ia read*/
    {
       logger_puts("%d bytes of data packet recieved, sending ack %zd\n", clients[slot].data_packet->len, clients[slot].data_packet->data[NUM_OF_DATA]);
      /* send #data recieved */
      accept = clients[slot].data_packet->data[NUM_OF_DATA];
      if (bufferevent_write(bev, &accept, 1) == -1)
      {
        logger_puts("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
        fatal("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
      }
      clients[slot].state = WAIT_NUM_RECIEVED_DATA_TOBE_SENT;
      logger_puts("in WAIT_NUM_RECIEVED_DATA_TOBE_SENT state");
    }
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
    logger_puts("in WAIT_FOR_DATA_PACKET state");
  }
  else if (clients[slot].state == WAIT_NUM_RECIEVED_DATA_TOBE_SENT)
  {
    remove_client(bev);
    logger_puts("client removed");
  }
}

/****************************************************************************/
static void
accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ctx)
{
  /* We got a new connection! Set up a bufferevent for it. */
  logger_puts("A new connection established from");
  printf("A new connection established\n");

  struct event_base *base = evconnlistener_get_base(listener);
  struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

  bufferevent_setcb(bev, serv_read_cb, serv_write_cb, serv_event_cb, NULL);

  add_client(bev);

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
    logger_puts("ERROR: %s, '%s', line %d, could not create hash table", __FILE__, __func__, __LINE__);
    logger_close();
    fatal("ERROR: Could not create hash table");
  }

  int port = 1975;

  if (argc > 1)
    port = atoi(argv[1]);

  logger_open("ttserv.log");

  if (port<=0 || port>65535)
  {
    logger_puts("ERROR: %s, '%s', line %d, invalid port", __FILE__, __func__, __LINE__);
    logger_close();
    fatal("Invalid port");
  }

  base = event_base_new();
  if (!base)
  {
    logger_puts("ERROR: %s, '%s', line %d, couldn't open event base", __FILE__, __func__, __LINE__);
    logger_close();
    fatal("ERROR: %s, '%s', line %d, couldn't open event base", __FILE__, __func__, __LINE__);
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
    logger_puts("ERROR: %s, '%s', line %d, couldn't create listener", __FILE__, __func__, __LINE__);
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
