/**
 * @file cJSON_Path.h
 * @brief Basic JSONPath query support for cJSON
 * 
 * Supported syntax:
 *   $              - Root node
 *   $.store.book   - Child access with dot notation
 *   $['store']     - Child access with bracket notation
 *   $.book[0]      - Array index
 *   $.book[*]      - Array wildcard (returns all elements)
 *   $['book'][*]   - Mixed syntax
 * 
 */

#ifndef CJSON_PATH_H
#define CJSON_PATH_H

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Execute a JSONPath query on a cJSON object
 * 
 * @param root      cJSON root object (typically from cJSON_Parse)
 * @param path      JSONPath expression string
 * @param result    Output parameter: pointer to result array (caller must free)
 * @return          Number of matched nodes, -1 on parse error
 */
int cJSONPath_Query(cJSON* root, const char* path, cJSON*** result);

/**
 * Free query results
 * 
 * @param results   Result array from cJSONPath_Query
 */
void cJSONPath_FreeResults(cJSON** results);

#ifdef __cplusplus
}
#endif

#endif /* CJSON_PATH_H */