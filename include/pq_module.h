#ifndef _PQ_MODULE
#define _PQ_MODULE

#include "parser_module.h"

void db_connect();
void db_store_AVL_data_array(const AVL_data_array* data_array);
void db_close();
#endif
