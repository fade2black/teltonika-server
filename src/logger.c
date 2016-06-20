#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "hdrs.h"
#include "logger.h"


void logger_open(const char* filename)
{
  logger_fp = fopen(filename, "a");

  if (!logger_fp)
    errExit("open");
}

void logger_puts(const char* format, ...)
{
  const size_t buf_size = 256;
  va_list arg_list;
  int saved_errno;
  char tmbuf[75], buffer[buf_size];

  time(&rawtime);
  strftime(tmbuf, 75,"%Y-%m-%d %T", localtime(&rawtime));

  saved_errno = errno;
  va_start(arg_list, format);
  vsnprintf (buffer, buf_size, format, arg_list);
  va_end (arg_list);
  errno = saved_errno;

  fprintf(logger_fp, "%s:- %s\n", tmbuf, buffer);
  fflush(logger_fp);


}

void logger_close()
{
  fclose(logger_fp);
}
