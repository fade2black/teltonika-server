#include "hdrs.h"
#include "slots_mng.h"
int main()
{
  int i, slot;

  puts("Testing slots_mng.*...");

  init_empty_slots();
  if (get_empty_slot() == MAXCLIENTS - 1)
     puts("Success");
  else
     puts("Failure");

  put_empty_slot(MAXCLIENTS - 1);
  if (get_empty_slot() == MAXCLIENTS - 1)
     puts("Success");
  else
     puts("Failure");

  put_empty_slot(MAXCLIENTS - 1);

  for(i = 0; i<MAXCLIENTS; i++)
    slot = get_empty_slot();

  if (slot == 0)
     puts("Success");
  else
     puts("Failure");

  for(i = 0; i<MAXCLIENTS; i++)
    put_empty_slot(i);

  if (get_empty_slot() == MAXCLIENTS - 1)
     puts("Success");
  else
     puts("Failure");

  get_empty_slot();
  get_empty_slot();
  put_empty_slot(1);
  put_empty_slot(1);

  get_empty_slot();
  if (get_empty_slot() == 1)
     puts("Failure");
  else
     puts("Success");

  exit(EXIT_SUCCESS);
}
