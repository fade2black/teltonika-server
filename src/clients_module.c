#include "hdrs.h"
#include "logger_module.h"
#include "clients_module.h"
#include <assert.h>

static int empty_slots[MAXCLIENTS];
static char empty_slot_occupied[MAXCLIENTS];
static int es_index;
static GHashTable* clients_hash;
static client_info clients[MAXCLIENTS];


static
void init_empty_slots()
{
  for(es_index = 0; es_index < MAXCLIENTS; es_index++)
  {
    empty_slots[es_index] = es_index;
    empty_slot_occupied[es_index] = 0;
  }
}


static
int get_empty_slot()
{
  int slot;

  if (es_index == 0)
    fatal("get_empty_slot: empty stack");


  slot = empty_slot_occupied[--es_index];
  if (empty_slot_occupied[slot])
    fatal("get_empty_slot: slot already occupied");

  empty_slot_occupied[es_index] = 1;
  return empty_slots[es_index];
}


static
void put_empty_slot(int i)
{
  if (es_index >= MAXCLIENTS)
    fatal("put_empty_slot: stack overflow");

  if (!empty_slot_occupied[i])
    return; /* since the slot is already in the pool */

  empty_slots[es_index] = i;
  empty_slot_occupied[es_index] = 0;
  es_index++;
}


void
init_clients_hash()
{
  init_empty_slots();
  clients_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
  if (!clients_hash)
  {
    logger_puts("ERROR: %s, '%s', line %d, 'g_hash_table_new' failed", __FILE__, __func__, __LINE__);
    fatal("ERROR: 'g_hash_table_new' failed");
  }
}


void
add_client(struct bufferevent *bev)
{
  int empty_slot = get_empty_slot();

  g_hash_table_insert(clients_hash,  GINT_TO_POINTER(bev), GINT_TO_POINTER(empty_slot));
  clients[empty_slot].state = WAIT_FOR_IMEI;
  /* allocate memmory */
  clients[empty_slot].imei = g_byte_array_new();
  assert(clients[empty_slot].imei != NULL);
  clients[empty_slot].data_packet = g_byte_array_new();
  assert(clients[empty_slot].data_packet != NULL);
}

client_info* client
get_client(struct bufferevent *bev)
{
  int slot = GPOINTER_TO_INT(g_hash_table_lookup(clients_hash, GINT_TO_POINTER(bev)));

  if (slot < 0 || slot >= MAXCLIENTS)
  {
    logger_puts("ERROR: %s, '%s', line %d, slot value (%d) out of range", __FILE__, __func__, __LINE__, slot);
    fatal("ERROR: slot value (%d) out of range");
  }

  return &clients[slot];
}

void
remove_client(struct bufferevent *bev)
{
  int slot = GPOINTER_TO_INT(g_hash_table_lookup(clients_hash, GINT_TO_POINTER(bev)));
  if (slot < 0 || slot >= MAXCLIENTS)
  {
    logger_puts("ERROR: %s, '%s', line %d, slot value (%d) out of range", __FILE__, __func__, __LINE__, slot);
    fatal("ERROR: slot value (%d) out of range");
  }

  /* free allocated memory */
  g_byte_array_free (clients[slot].imei, TRUE);
  g_byte_array_free (clients[slot].data_packet, TRUE);

  g_hash_table_remove(clients_hash, GINT_TO_POINTER(bev));
  put_empty_slot(slot);
}
