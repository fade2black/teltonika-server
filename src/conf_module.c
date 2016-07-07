#include "hdrs.h"
#include "conf_module.h"

void
conf_read(char* group_name, char keys[][MAX_CONF_STRING_LEN], char values[][MAX_CONF_STRING_LEN])
{
  GError* error = NULL;
  GKeyFile* gkf = g_key_file_new();
  int i;
  char* value;

  if (!g_key_file_load_from_file(gkf, CONFIG_FILE, G_KEY_FILE_NONE, NULL))
    fatal("Could not read config file %s\n", CONFIG_FILE);

  for(i=0; keys[i][0] != '\0'; i++)
  {
    value = g_key_file_get_string (gkf, group_name, keys[i], &error);
    if (!value)
      printf("Warning: %s\n", error->message);
    else
      strcpy(values[i], value);

    g_clear_error(&error);
  }
}
