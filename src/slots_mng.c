#include "hdrs.h"
#include "global_consts.h"
#include "slots_mng.h"

void init_empty_slots()
{
  for(es_index = 0; es_index < MAXCLIENTS; es_index++)
  {
    empty_slots[es_index] = es_index;
    empty_slot_occupied[es_index] = 0;
  }
}

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

void put_empty_slot(int i)
{
  if (es_index >= MAXCLIENTS)
    fatal("put_empty_slot: stack overflow");

  if (!empty_slot_occupied[i])
    return; /* since the slot already in the pool */

  empty_slots[es_index] = i;
  empty_slot_occupied[es_index] = 0;
  es_index++;
}
