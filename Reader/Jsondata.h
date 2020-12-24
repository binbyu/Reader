#ifndef __JSON_DATA_H__
#define __JSON_DATA_H__

#include "types.h"

char* create_json(header_t* data);

void create_json_free(char* json);

bool parser_json(const char* json, header_t* default, void **data, int *size);

#endif