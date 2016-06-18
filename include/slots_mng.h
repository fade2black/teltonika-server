#ifndef _SLOTS_MANAGER_
#define _SLOTS_MANAGER_

#include "global_consts.h"

static int empty_slots[MAXCLIENTS];
static char empty_slot_occupied[MAXCLIENTS];
static int es_index;

void init_empty_slots();
int get_empty_slot();
void put_empty_slot(int i);
#endif
