#ifndef LOGGER
#define LOGGER

void logger_open(const char* filename);
void logger_puts(const char *format, ...);
void logger_close();

#endif
