#include "../includes/hdrs.h"
#include "logger.h"

int main()
{
  int i;
  
  logger_open("test.log");
  for(i=0; i<10; i++)
    logger_puts("hello world");

  logger_close();

  exit(EXIT_SUCCESS);
}
     
