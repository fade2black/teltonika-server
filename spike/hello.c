#include <stdio.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <glib.h>
#include "includes/hdrs.h"

void print_hash(gpointer key, gpointer value, gpointer user_data)
{
  printf("key: %p, value: %d\n", (void*)((long int *)key), *((int*)value));
}


int main()
{
  int i, val;
  int arr[10];
  void* keys[10];
  int vals[10];

  printf("%ld bytes per pointer\n", sizeof(void *));
  printf("%ld byte per long int\n", sizeof(long int));

  for(i = 0; i < 10; i++)
  {
    arr[i] = i*i;
    vals[i] = i;
    keys[i] = (void *)(arr + i);
  }

  GHashTable* hash = g_hash_table_new(g_int64_hash, g_int64_equal);
  if (!hash)
    fatal("Couldn't create a hash");

  for(i = 0; i < 10; i++)
  {
    g_hash_table_insert(hash,  &keys[i], &vals[i]);
  }
  printf("Hash size is %d\n", g_hash_table_size(hash));

  for(i = 0; i < 10; i++)
  {
     val = * ((int*) g_hash_table_lookup(hash, &keys[i]));
     printf("key: %p, value: %d\n", keys[i], val);
  }

  puts("-------------------------");

  g_hash_table_foreach(hash, print_hash, NULL);

  g_hash_table_remove_all(hash);
  g_hash_table_destroy(hash);

  exit(EXIT_SUCCESS);
}
