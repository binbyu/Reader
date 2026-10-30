/**
 * @file cJSON_XPath.h
 * @brief XPath-like query interface for cJSON
 * 
 * Provides an XPath-inspired query language for navigating and extracting
 * data from cJSON structures. Supports a subset of XPath syntax adapted
 * for JSON data models.
 * 
 * Supported Syntax:
 *   /key          - Direct child access
 *   //key         - Recursive descent (find at any depth)
 *   /key1/key2    - Nested path traversal
 *   /*            - Wildcard (all children)
 *   /array[2]     - Array index access
 *   /book[price<10] - Predicate filtering with comparison operators
 * 
 * Usage Example:
 *   cJSON* root = cJSON_Parse(json_string);
 *   cJSON** results = NULL;
 *   int count = cJSONXPath_Query(root, "//author", &results);
 *   // Process results...
 *   cJSONXPath_FreeResults(results);
 *   cJSON_Delete(root);
 */

#ifndef CJSON_XPATH_H
#define CJSON_XPATH_H

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Execute an XPath-like query on a cJSON structure
 * 
 * Parses the provided XPath expression and evaluates it against the 
 * given JSON root node. Results are returned as an array of cJSON pointers.
 * 
 * @param root    The root cJSON node to query
 * @param xpath   The XPath-like query string (must start with '/' or '//')
 * @param result  Output parameter: array of cJSON pointers matching the query
 * 
 * @return Number of results found, or -1 on error
 * 
 * @note The caller is responsible for freeing the result array using
 *       cJSONXPath_FreeResults(). The individual cJSON items in the 
 *       result array should NOT be freed - they are owned by the 
 *       original JSON document.
 */
int cJSONXPath_Query(cJSON* root, const char* xpath, cJSON*** result);

/**
 * @brief Free the result array from cJSONXPath_Query
 * 
 * Frees the array of cJSON pointers returned by cJSONXPath_Query.
 * This does NOT free the actual cJSON nodes, only the array container.
 * 
 * @param results  The results array to free (can be NULL)
 */
void cJSONXPath_FreeResults(cJSON** results);

#ifdef __cplusplus
}
#endif

#endif /* CJSON_XPATH_H */