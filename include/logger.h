#ifndef LOGGER
#define LOGGER

FILE *logger_fp;
time_t rawtime;

void logger_open(char* filename);
void logger_puts(char* info);
void logger_close();

#endif
