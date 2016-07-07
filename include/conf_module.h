#ifndef CONF_PARSER
#define CONF_PARSER
#define MAX_CONF_STRING_LEN 256
#define CONFIG_FILE "ttserv.conf"

void conf_read(char* group_name, char keys[][MAX_CONF_STRING_LEN], char values[][MAX_CONF_STRING_LEN]);
#endif
