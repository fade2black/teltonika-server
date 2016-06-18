#ifndef LOGGER
#define LOGGER

static FILE *logger_fp;
static time_t rawtime;

void logger_open(char* filename);
void logger_puts(char* info);
void logger_close();

#endif
