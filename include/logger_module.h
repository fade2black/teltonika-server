#ifndef LOGGER
#define LOGGER

FILE *logger_fp;
time_t rawtime;

void logger_open(const char* filename);
void logger_puts(const char *format, ...);
void logger_close();

#endif
