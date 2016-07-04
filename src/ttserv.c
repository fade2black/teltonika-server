
#include "hdrs.h"
#include "logger_module.h"
#include "clients_module.h"
#include "parser_module.h"
#include <assert.h>

static unsigned char input_buffer[INPUT_BUFSIZE];
struct event_base *base;


static void
print_data_packet(const client_info* client)
{
  int i;
  AVL_data_array data_array;
  printf("IMEI: ");
  for(i = 0; i < client->imei->len; i++)
    printf("%c", client->imei->data[i]);
  printf("\n");
  parse_AVL_data_array(client->data_packet->data, &data_array);
  print_avl_data_array(&data_array);
}

/* if all bytes of imei are read then return TRUE else return FALSE */
static int
process_imei(const unsigned char* data, size_t nbytes,  client_info* client)
{
  size_t length;
  size_t num_of_read_bytes;

  assert(client != NULL);

  /* append bytes to imei */
  g_byte_array_append(client->imei, (guint8*)data, nbytes);
  num_of_read_bytes = client->imei->len;

  if (num_of_read_bytes > 1)
  {
    /* more than two bytes have already been read, so we can check
       whether or not we have read the entire message */
    length = client->imei->data[0];
    length <<= 8;
    length |= client->imei->data[1];

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
process_data_packet(const unsigned char* data, size_t nbytes, client_info* client)
{
  size_t length;
  size_t num_of_read_bytes;

  logger_puts("processing data packet");

  g_byte_array_append(client->data_packet, (guint8*)data, nbytes);
  num_of_read_bytes = client->data_packet->len;

  if (num_of_read_bytes > 7) /* if at least 8 bytes are recieved */
  {
    length = client->data_packet->data[4];
    length <<= 8;
    length |= client->data_packet->data[5];
    length <<= 8;
    length |= client->data_packet->data[6];
    length <<= 8;
    length |= client->data_packet->data[7];

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
  struct evbuffer *input = bufferevent_get_input(bev);
  unsigned char ack[4] = {0,0,0,0};
  int nbytes;
  client_info client;

  get_client(bev, &client);
  assert(client != NULL);

  if (evbuffer_get_length(input) > INPUT_BUFSIZE)
  {
    logger_puts("ERROR: %s, '%s', line %d, insufficient buffer size", __FILE__, __func__, __LINE__);
    fatal("ERROR: Insufficient buffer size");
  }

  memset(input_buffer, 0, INPUT_BUFSIZE);

  nbytes = bufferevent_read(bev, input_buffer, INPUT_BUFSIZE);
  if ( nbytes == -1)
  {
    logger_puts("ERROR: %s, '%s', line %d, couldn't read data from bufferevent", __FILE__, __func__, __LINE__);
    fatal("ERROR: %s, '%s', line %d, couldn't read data from bufferevent", __FILE__, __func__, __LINE__);
  }

  if (client.state == WAIT_FOR_IMEI)
  {
    /* if process_imei returns TRUE then imei are read entirely, otherwise stay in the WAIT_FOR_IMEI state*/
    if (process_imei(input_buffer, nbytes, &client))
    {
      ack[0] = 0x01;
      if (bufferevent_write(bev, ack, 1) == -1)
      {
        logger_puts("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
        fatal("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
      }
      client.state = WAIT_00_01_TOBE_SENT;
      logger_puts("in WAIT_00_01_TOBE_SENT state");
    }
  }
  else if (client.state == WAIT_FOR_DATA_PACKET)
  {
    if (process_data_packet(input_buffer, nbytes, &client))/* if entire AVL packet is read*/
    {
      /*logger_puts("%d bytes of data packet recieved, sending ack %zd", client.data_packet->len, client.data_packet->data[NUM_OF_DATA]);*/
      /* send #data recieved */
      ack[3] = client.data_packet->data[NUM_OF_DATA];
      if (bufferevent_write(bev, ack, 4) == -1)
      {
        logger_puts("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
        fatal("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
      }
      client.state = WAIT_NUM_RECIEVED_DATA_TOBE_SENT;
      logger_puts("in WAIT_NUM_RECIEVED_DATA_TOBE_SENT state");
    }
  }
}

/****************************************************************************/
static void
serv_write_cb(struct bufferevent *bev, void *ctx)
{
  client_info client;

  get_client(bev, &client);
  assert(client != NULL);

  if (client.state == WAIT_00_01_TOBE_SENT)
  {
    client.state = WAIT_FOR_DATA_PACKET;
    logger_puts("in WAIT_FOR_DATA_PACKET state");
  }
  else if (client.state == WAIT_NUM_RECIEVED_DATA_TOBE_SENT)
  {
    print_data_packet(&client);/* for debug purpose */

    remove_client(bev);
    bufferevent_disable(bev, EV_READ | EV_WRITE);
    bufferevent_setcb(bev, NULL, NULL, NULL, NULL);
    bufferevent_free(bev);
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

  init_clients_hash();


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
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(0);
  sin.sin_port = htons(port);

  listener = evconnlistener_new_bind(
    base,
    accept_conn_cb,
    NULL, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
    (struct sockaddr*)&sin, sizeof(sin));

  if (!listener)
  {
    logger_puts("ERROR: %s, '%s', line %d, couldn't create listener", __FILE__, __func__, __LINE__);
    logger_close();
    fatal("Couldn't create listener");
  }

  evconnlistener_set_error_cb(listener, accept_error_cb);

  event_base_dispatch(base);

  /* free */
  evconnlistener_free(listener);
  event_base_free(base);

  logger_close();
  exit(EXIT_SUCCESS);
}
