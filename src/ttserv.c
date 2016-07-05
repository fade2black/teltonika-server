
#include "hdrs.h"
#include "logger_module.h"
#include "clients_module.h"
#include "parser_module.h"
#include <assert.h>

static unsigned char input_buffer[INPUT_BUFSIZE];
struct event_base *base;
static GQueue* queue;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_consumer = PTHREAD_COND_INITIALIZER;




static void
push_onto_queue(const client_info* client)
{
  AVL_data_array *data_array;
  int s;

  data_array = (AVL_data_array*) malloc(sizeof(AVL_data_array));
  if (!data_array)
  {
    logger_puts("ERROR: %s, '%s', line %d, malloc failed", __FILE__, __func__, __LINE__);
    fatal("%s, '%s', line %d, malloc failed", __FILE__, __func__, __LINE__);
  }

  parse_AVL_data_array(client->data_packet->data, data_array);

  s = pthread_mutex_lock(&mtx);
  if (s != 0)
  {
    logger_puts("ERROR: %s, '%s', line %d, pthread_mutex_lock failed with code %d", __FILE__, __func__, __LINE__, s);
    fatal("ERROR: %s, '%s', line %d, pthread_mutex_lock failed with code %d", __FILE__, __func__, __LINE__, s);
  }

  g_queue_push_head(queue, data_array);

  s = pthread_mutex_unlock(&mtx);
  if (s != 0)
  {
    logger_puts("ERROR: %s, '%s', line %d, pthread_mutex_unlock failed with code %d", __FILE__, __func__, __LINE__, s);
    fatal("ERROR: %s, '%s', line %d, pthread_mutex_unlock failed with code %d", __FILE__, __func__, __LINE__, s);
  }

  /* for debug purpose */
  int i;
  printf("IMEI: ");
  for(i = 0; i < client->imei->len; i++)
    printf("%c", client->imei->data[i]);
  printf("\n");

}
/***********************************************************/

static void*
thread_consumer(void *arg)
{
  AVL_data_array *data_array;
  int s;

  while(1)
  {
    s = pthread_mutex_lock(&mtx);
    if (s != 0)
    {
      logger_puts("ERROR: %s, '%s', line %d, pthread_mutex_lock failed with code %d", __FILE__, __func__, __LINE__, s);
      fatal("ERROR: %s, '%s', line %d, pthread_mutex_lock failed with code %d", __FILE__, __func__, __LINE__, s);
    }
    /******* LOCKED *******************************************/

    /* Wait until queue has at least one element*/
    while (queue->length == 0)
    {
        s = pthread_cond_wait(&cond_consumer, &mtx);
        if (s != 0)
        {
          logger_puts("ERROR: %s, '%s', line %d, pthread_cond_wait failed with code %d", __FILE__, __func__, __LINE__, s);
          fatal("ERROR: %s, '%s', line %d, pthread_cond_wait failed with code %d", __FILE__, __func__, __LINE__, s);
        }
    }

    /* consume all AVL data*/
    while (queue->length)
    {
      data_array = g_queue_pop_tail(queue);

      /* for debug purpose */
      print_AVL_data(data_array);
      /*****************************/

      free(data_array);
    }

    /******* UNLOCK *******************************************/
    s = pthread_mutex_unlock(&mtx);
    if (s != 0)
    {
      logger_puts("ERROR: %s, '%s', line %d, pthread_mutex_unlock failed with code %d", __FILE__, __func__, __LINE__, s);
      fatal("ERROR: %s, '%s', line %d, pthread_mutex_unlock failed with code %d", __FILE__, __func__, __LINE__, s);
    }

  }/* while(1) */

  return 0;
}
/***********************************************************/

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
/***********************************************************/

/* if all bytes of data packet are read then return TRUE else return FALSE */
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
  client_info* client = NULL;

  client = get_client(bev);
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

  if (client->state == WAIT_FOR_IMEI)
  {
    /* if process_imei returns TRUE then imei are read entirely, otherwise stay in the WAIT_FOR_IMEI state*/
    if (process_imei(input_buffer, nbytes, client))
    {
      ack[0] = 0x01;
      if (bufferevent_write(bev, ack, 1) == -1)
      {
        logger_puts("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
        fatal("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
      }
      client->state = WAIT_00_01_TOBE_SENT;
      logger_puts("in WAIT_00_01_TOBE_SENT state");
    }
  }
  else if (client->state == WAIT_FOR_DATA_PACKET)
  {
    if (process_data_packet(input_buffer, nbytes, client))/* if entire AVL packet is read*/
    {
      /*logger_puts("%d bytes of data packet recieved, sending ack %zd", client->data_packet->len, client->data_packet->data[NUM_OF_DATA]);*/
      /* send #data recieved */
      ack[3] = client->data_packet->data[NUM_OF_DATA];
      if (bufferevent_write(bev, ack, 4) == -1)
      {
        logger_puts("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
        fatal("ERROR: %s, '%s', line %d, couldn't write data to bufferevent", __FILE__, __func__, __LINE__);
      }
      client->state = WAIT_NUM_RECIEVED_DATA_TOBE_SENT;
      logger_puts("in WAIT_NUM_RECIEVED_DATA_TOBE_SENT state");
    }
  }
}
/****************************************************************************/

static void
serv_write_cb(struct bufferevent *bev, void *ctx)
{
  client_info* client = NULL;

  client = get_client(bev);
  assert(client != NULL);

  if (client->state == WAIT_00_01_TOBE_SENT)
  {
    client->state = WAIT_FOR_DATA_PACKET;
    logger_puts("in WAIT_FOR_DATA_PACKET state");
  }
  else if (client->state == WAIT_NUM_RECIEVED_DATA_TOBE_SENT)
  {
    push_onto_queue(client); /* for parsing and storing in DB, by another thread */

    /* Wake waiting consumer */
    s = pthread_cond_signal(&cond_consumer);
    if (s != 0)
    {
      logger_puts("ERROR: %s, '%s', line %d, pthread_cond_signal failed with code %d", __FILE__, __func__, __LINE__, s);
      fatal("ERROR: %s, '%s', line %d, pthread_cond_signal failed with code %d", __FILE__, __func__, __LINE__, s);
    }

    remove_client(bev);
    bufferevent_disable(bev, EV_READ | EV_WRITE);
    bufferevent_setcb(bev, NULL, NULL, NULL, NULL);
    bufferevent_free(bev);
    /*logger_puts("client removed");*/
  }
}
/****************************************************************************/

static void
accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ctx)
{
  /*logger_puts("A new connection established from");*/
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
  int s, port = 1975;;
  void* res;
  pthread_t pthread;

  logger_open("ttserv.log");

  init_clients_hash();
  queue = g_queue_new();
  assert(queue != NULL);

  /* start parallel thread */
  s = pthread_create(&pthread, NULL, thread_consumer, NULL);
  if (s != 0)
  {
    logger_puts("ERROR: %s, '%s', line %d, pthread_create failed with error %d", __FILE__, __func__, __LINE__, s);
    logger_close();
    fatal("ERROR: %s, '%s', line %d, pthread_create failed with error %d", __FILE__, __func__, __LINE__, s);
  }
  logger_puts("INFO: AVL data consumer started successfully");
  puts("AVL data consumer thread started successfully");


  if (argc > 1)
    port = atoi(argv[1]);

  if (port <= 0 || port > 65535)
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
     platform-specific fields that can mess us up. */
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


  /* JUST IN CASE,
     in fact this code should be unreachable since event_base_dispatch loops infinitely */
  s = pthread_join(pthread, &res);
  if (s != 0)
  {
    logger_puts("ERROR: %s, '%s', line %d, pthread_join failed with error %d", __FILE__, __func__, __LINE__, s);
    logger_close();
    fatal("ERROR: %s, '%s', line %d, pthread_join failed with error %d", __FILE__, __func__, __LINE__, s);
  }
  puts("AVL data consumer thread finished successfully.");

  /* free */
  evconnlistener_free(listener);
  event_base_free(base);

  while(queue->length)
    free(g_queue_pop_head(queue));

  g_queue_free(queue);
  pthread_mutex_destroy(&mtx);
  pthread_cond_destroy(&cond_consumer);

  logger_close();
  exit(EXIT_SUCCESS);
}
