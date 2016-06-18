#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "hdrs.h"
#include "logger.h"


void logger_open(char* filename)
{
  logger_fp = fopen(filename, "a");

  if (!logger_fp)
    errExit("open");
}

void logger_puts(char* info)
{
  time(&rawtime);
  char tmbuf[75];

  strftime(tmbuf, 75,"%Y-%m-%d %T", localtime(&rawtime));

  fprintf(logger_fp, "%s:- %s\n", tmbuf, info);
}

void logger_close()
{
  fclose(logger_fp);
}
