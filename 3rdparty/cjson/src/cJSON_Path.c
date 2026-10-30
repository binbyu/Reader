/**
 * @file cJSON_Path.c
 * @brief Implementation of basic JSONPath query engine for cJSON
 */

#include "cJSON_Path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(_MSC_VER)
# define cjson_strdup _strdup
#else
# define cjson_strdup strdup
#endif

/* ============================================================================
 * Lexer / Parser
 * ============================================================================ */

typedef enum {
    TOKEN_ROOT,
    TOKEN_DOT,
    TOKEN_DOTDOT,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_STAR,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_IDENT,
    TOKEN_EOF,
    TOKEN_ERROR
} token_type_t;

typedef struct {
    token_type_t type;
    char* value;      /* For STRING, IDENT, NUMBER */
    int int_value;    /* For NUMBER */
} token_t;

typedef struct {
    const char* input;
    int pos;
    int len;
    token_t current;
} lexer_t;

/* Forward declarations */
static void next_token(lexer_t* lexer);

/* Initialize lexer */
static void lexer_init(lexer_t* lexer, const char* input) {
    lexer->input = input;
    lexer->pos = 0;
    lexer->len = (int)strlen(input);
    lexer->current.type = TOKEN_ERROR;
    lexer->current.value = NULL;
    lexer->current.int_value = 0;
}

/* Check if character can be part of an identifier */
static int is_ident_char(char c) {
    return isalnum(c) || c == '_' || c == '-';
}

/* Free token's internal data */
static void free_token(token_t* token) {
    if (token->value) {
        free(token->value);
        token->value = NULL;
    }
}

/* Read next token from input */
static void next_token(lexer_t* lexer) {
    /* free previous token payload (if any) to avoid leaks */
    free_token(&lexer->current);
    /* Skip whitespace */
    while (lexer->pos < lexer->len && isspace(lexer->input[lexer->pos])) {
        lexer->pos++;
    }
    
    if (lexer->pos >= lexer->len) {
        lexer->current.type = TOKEN_EOF;
        return;
    }
    
    char c = lexer->input[lexer->pos];
    
    /* Root node */
    if (c == '$') {
        lexer->current.type = TOKEN_ROOT;
        lexer->pos++;
        return;
    }
    
    /* Dot or DotDot (..) */
    if (c == '.') {
        lexer->pos++;
        /* Check if next char is also a dot */
        if (lexer->pos < lexer->len && lexer->input[lexer->pos] == '.') {
            lexer->current.type = TOKEN_DOTDOT;
            lexer->pos++;
        } else {
            lexer->current.type = TOKEN_DOT;
        }
        return;
    }
    
    /* Left bracket */
    if (c == '[') {
        lexer->current.type = TOKEN_LBRACKET;
        lexer->pos++;
        return;
    }
    
    /* Right bracket */
    if (c == ']') {
        lexer->current.type = TOKEN_RBRACKET;
        lexer->pos++;
        return;
    }
    
    /* Star (wildcard) */
    if (c == '*') {
        lexer->current.type = TOKEN_STAR;
        lexer->pos++;
        return;
    }
    
    /* String (single or double quoted) */
    if (c == '\'' || c == '"') {
        char quote = c;
        lexer->pos++;
        int start = lexer->pos;
        
        while (lexer->pos < lexer->len && lexer->input[lexer->pos] != quote) {
            lexer->pos++;
        }
        
        int value_len = lexer->pos - start;
        lexer->current.value = (char*)malloc(value_len + 1);
        strncpy(lexer->current.value, lexer->input + start, value_len);
        lexer->current.value[value_len] = '\0';
        lexer->current.type = TOKEN_STRING;
        lexer->pos++; /* Skip closing quote */
        return;
    }
    
    /* Number (support optional leading '-' and digits) */
    if (isdigit(c) || (c == '-' && lexer->pos + 1 < lexer->len && isdigit(lexer->input[lexer->pos + 1]))) {
        int start = lexer->pos;
        if (lexer->input[lexer->pos] == '-') lexer->pos++;
        while (lexer->pos < lexer->len && isdigit(lexer->input[lexer->pos])) lexer->pos++;

        /* optional fractional part (allow floats though indices are ints) */
        if (lexer->pos < lexer->len && lexer->input[lexer->pos] == '.') {
            lexer->pos++;
            while (lexer->pos < lexer->len && isdigit(lexer->input[lexer->pos])) lexer->pos++;
        }

        int value_len = lexer->pos - start;
        char* num_str = (char*)malloc(value_len + 1);
        if (!num_str) {
            lexer->current.type = TOKEN_ERROR;
            return;
        }
        strncpy(num_str, lexer->input + start, value_len);
        num_str[value_len] = '\0';

        lexer->current.value = num_str;
        lexer->current.int_value = atoi(num_str);
        lexer->current.type = TOKEN_NUMBER;
        return;
    }
    
    /* Identifier (unquoted key name) */
    if (is_ident_char(c)) {
        int start = lexer->pos;
        while (lexer->pos < lexer->len && is_ident_char(lexer->input[lexer->pos])) {
            lexer->pos++;
        }
        
        int value_len = lexer->pos - start;
        lexer->current.value = (char*)malloc(value_len + 1);
        strncpy(lexer->current.value, lexer->input + start, value_len);
        lexer->current.value[value_len] = '\0';
        lexer->current.type = TOKEN_IDENT;
        return;
    }
    
    lexer->current.type = TOKEN_ERROR;
}

/* Expect a specific token type, abort on mismatch */
static int expect(lexer_t* lexer, token_type_t type, const char* msg) {
    if (lexer->current.type != type) {
        fprintf(stderr, "JSONPath parse error: expected %s\n", msg);
        return -1;
    }
    return 0;
}

/* ============================================================================
 * AST Segment Definition
 * ============================================================================ */

typedef struct segment_s {
    enum {
        SEG_ROOT,
        SEG_RECURSIVE,
        SEG_KEY,
        SEG_INDEX,
        SEG_WILDCARD
    } type;
    
    union {
        char* key;      /* For SEG_KEY */
        int index;      /* For SEG_INDEX */
    };
    
    struct segment_s* next;
} segment_t;

/* Parse JSONPath string into a linked list of segments */
static segment_t* parse_segments(const char* path) {
    lexer_t lexer;
    lexer_init(&lexer, path);
    next_token(&lexer);
    
    segment_t dummy = {0};
    segment_t* tail = &dummy;
    
    /* Must start with $ */
    if (0 != expect(&lexer, TOKEN_ROOT, "$"))
    {
        return NULL;
    }
    next_token(&lexer);
    
    /* Single $ only - query root node */
    if (lexer.current.type == TOKEN_EOF) {
        segment_t* seg = (segment_t*)calloc(1, sizeof(segment_t));
        seg->type = SEG_ROOT;
        tail->next = seg;
        tail = seg;
        return dummy.next;
    }
    
    /* Parse remaining path segments */
    while (lexer.current.type != TOKEN_EOF && lexer.current.type != TOKEN_ERROR) {
        segment_t* seg = (segment_t*)calloc(1, sizeof(segment_t));
        
        /* Handle recursive descent .. */
        if (lexer.current.type == TOKEN_DOTDOT) {
            seg->type = SEG_RECURSIVE;
            next_token(&lexer);
            
            /* After .., we need a key or wildcard to search for */
            if (lexer.current.type == TOKEN_IDENT) {
                seg->key = cjson_strdup(lexer.current.value);
                free_token(&lexer.current);
                next_token(&lexer);
            } else if (lexer.current.type == TOKEN_STAR) {
                /* ..* means recursive wildcard - match everything */
                seg->type = SEG_RECURSIVE;
                /* We'll handle this specially in execution */
                next_token(&lexer);
            } else if (lexer.current.type == TOKEN_LBRACKET) {
                /* ..[expr] syntax */
                next_token(&lexer);
                
                if (lexer.current.type == TOKEN_STRING) {
                    seg->key = _strdup(lexer.current.value);
                    free_token(&lexer.current);
                    next_token(&lexer);
                    expect(&lexer, TOKEN_RBRACKET, "]");
                    next_token(&lexer);
                } else if (lexer.current.type == TOKEN_STAR) {
                    /* ..[*] = recursive wildcard */
                    next_token(&lexer);
                    expect(&lexer, TOKEN_RBRACKET, "]");
                    next_token(&lexer);
                } else {
                    fprintf(stderr, "Expected string or * after ..[\n");
                    free(seg);
                    return NULL;
                }
            } else {
                fprintf(stderr, "Expected key or wildcard after ..\n");
                free(seg);
                return NULL;
            }
        }
        /* Handle .key or .[key] or [key] or [index] or [*] */
        else if (lexer.current.type == TOKEN_DOT) {
            next_token(&lexer);
            
            if (lexer.current.type == TOKEN_IDENT) {
                seg->type = SEG_KEY;
                seg->key = cjson_strdup(lexer.current.value);
                free_token(&lexer.current);
                next_token(&lexer);
            } else if (lexer.current.type == TOKEN_STAR) {
                seg->type = SEG_WILDCARD;
                next_token(&lexer);
            } else {
                fprintf(stderr, "Expected identifier after .\n");
                free(seg);
                return NULL;
            }
        } 
        else if (lexer.current.type == TOKEN_LBRACKET) {
            next_token(&lexer);
            
            if (lexer.current.type == TOKEN_STRING) {
                seg->type = SEG_KEY;
                seg->key = cjson_strdup(lexer.current.value);
                free_token(&lexer.current);
                next_token(&lexer);
                expect(&lexer, TOKEN_RBRACKET, "]");
                next_token(&lexer);
            } 
            else if (lexer.current.type == TOKEN_NUMBER) {
                seg->type = SEG_INDEX;
                seg->index = lexer.current.int_value;
                free_token(&lexer.current);
                next_token(&lexer);
                expect(&lexer, TOKEN_RBRACKET, "]");
                next_token(&lexer);
            }
            else if (lexer.current.type == TOKEN_STAR) {
                seg->type = SEG_WILDCARD;
                next_token(&lexer);
                expect(&lexer, TOKEN_RBRACKET, "]");
                next_token(&lexer);
            }
            else {
                fprintf(stderr, "Expected string, number, or * after [\n");
                free(seg);
                return NULL;
            }
        }
        else {
            /* Implicit .key syntax (JSONPath allows $key = $.key) */
            if (lexer.current.type == TOKEN_IDENT) {
                seg->type = SEG_KEY;
                seg->key = cjson_strdup(lexer.current.value);
                free_token(&lexer.current);
                next_token(&lexer);
            } else {
                fprintf(stderr, "Unexpected token\n");
                free(seg);
                return NULL;
            }
        }
        
        tail->next = seg;
        tail = seg;
    }
    
    return dummy.next;
}

/* Free segment linked list */
static void free_segments(segment_t* seg) {
    while (seg) {
        segment_t* next = seg->next;
        if ((seg->type == SEG_KEY || seg->type == SEG_RECURSIVE) && seg->key) {
            free(seg->key);
        }
        free(seg);
        seg = next;
    }
}

/* ============================================================================
 * Execution Engine
 * ============================================================================ */

typedef struct {
    cJSON** items;
    int count;
    int capacity;
} result_list_t;

/* Initialize result list */
static void result_list_init(result_list_t* list) {
    list->count = 0;
    list->capacity = 8;
    list->items = (cJSON**)malloc(sizeof(cJSON*) * list->capacity);
}

/* Add an item to result list */
static void result_list_add(result_list_t* list, cJSON* item) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = (cJSON**)realloc(list->items, sizeof(cJSON*) * list->capacity);
    }
    list->items[list->count++] = item;
}

/* Merge another result list into this one */
static void result_list_merge(result_list_t* dest, result_list_t* src) {
    for (int i = 0; i < src->count; i++) {
        result_list_add(dest, src->items[i]);
    }
    free(src->items);
}

/* Forward declaration for recursive descent */
static void execute_segment(cJSON* current, segment_t* seg, result_list_t* results);

/* Check if item already exists in results list (by pointer comparison) */
static int result_list_contains(result_list_t* list, cJSON* item) {
    for (int i = 0; i < list->count; i++) {
        if (list->items[i] == item) {
            return 1;
        }
    }
    return 0;
}

/* Add an item to result list if not already present */
static void result_list_add_unique(result_list_t* list, cJSON* item) {
    if (!result_list_contains(list, item)) {
        result_list_add(list, item);
    }
}

/* Execute recursive descent: search entire tree for matching keys */
static void execute_recursive_descent(cJSON* current, segment_t* seg, result_list_t* results) {
    if (!current) return;

    /* If current node matches the key after .. */
    if (seg->key) {
        if (cJSON_IsObject(current)) {
            cJSON* child = cJSON_GetObjectItem(current, seg->key);
            if (child) {
                if (seg->next) {
                    execute_segment(child, seg->next, results);
                }
                else {
                    result_list_add_unique(results, child);
                }
            }
        }

        /* Also check array items for matching key */
        if (cJSON_IsArray(current)) {
            int size = cJSON_GetArraySize(current);
            for (int i = 0; i < size; i++) {
                cJSON* item = cJSON_GetArrayItem(current, i);
                if (cJSON_IsObject(item)) {
                    cJSON* child = cJSON_GetObjectItem(item, seg->key);
                    if (child) {
                        if (seg->next) {
                            execute_segment(child, seg->next, results);
                        }
                        else {
                            result_list_add_unique(results, child);
                        }
                    }
                }
            }
        }
    }
    else {
        /* ..* or ..[*] - recursive wildcard: match all descendants */
        if (seg->next) {
            execute_segment(current, seg->next, results);
        }
        else {
            result_list_add_unique(results, current);
        }
    }

    /* Recurse into children */
    if (cJSON_IsObject(current)) {
        cJSON* child = current->child;
        while (child) {
            execute_recursive_descent(child, seg, results);
            child = child->next;
        }
    }
    else if (cJSON_IsArray(current)) {
        int size = cJSON_GetArraySize(current);
        for (int i = 0; i < size; i++) {
            execute_recursive_descent(cJSON_GetArrayItem(current, i), seg, results);
        }
    }
}

/* Execute a segment on current JSON node, collecting results */
static void execute_segment(cJSON* current, segment_t* seg, result_list_t* results) {
    if (!current || !seg) return;
    
    switch (seg->type) {
        case SEG_ROOT:
            if (results->count == 0) {
                result_list_add(results, current);
            }
            break;
            
        case SEG_RECURSIVE:
            execute_recursive_descent(current, seg, results);
            break;
            
        case SEG_KEY:
            if (cJSON_IsObject(current)) {
                cJSON* child = cJSON_GetObjectItem(current, seg->key);
                if (child) {
                    if (seg->next) {
                        execute_segment(child, seg->next, results);
                    } else {
                        result_list_add(results, child);
                    }
                }
            }
            break;
            
        case SEG_INDEX:
            if (cJSON_IsArray(current)) {
                int array_size = cJSON_GetArraySize(current);
                if (seg->index >= 0 && seg->index < array_size) {
                    cJSON* child = cJSON_GetArrayItem(current, seg->index);
                    if (child) {
                        if (seg->next) {
                            execute_segment(child, seg->next, results);
                        } else {
                            result_list_add(results, child);
                        }
                    }
                }
            }
            break;
            
        case SEG_WILDCARD:
            if (cJSON_IsArray(current)) {
                int size = cJSON_GetArraySize(current);
                for (int i = 0; i < size; i++) {
                    cJSON* child = cJSON_GetArrayItem(current, i);
                    if (seg->next) {
                        execute_segment(child, seg->next, results);
                    } else {
                        result_list_add(results, child);
                    }
                }
            } 
            else if (cJSON_IsObject(current)) {
                cJSON* child = current->child;
                while (child) {
                    if (seg->next) {
                        execute_segment(child, seg->next, results);
                    } else {
                        result_list_add(results, child);
                    }
                    child = child->next;
                }
            }
            break;
    }
}

/* ============================================================================
 * Public API
 * ============================================================================ */

int cJSONPath_Query(cJSON* root, const char* path, cJSON*** result) {
    if (!root || !path || !result) return -1;
    
    /* Parse path into segment list */
    segment_t* segments = parse_segments(path);
    if (!segments) return -1;
    
    /* Execute query */
    result_list_t results;
    result_list_init(&results);
    
    if (segments->type == SEG_ROOT && !segments->next) {
        /* Query root node */
        result_list_add(&results, root);
    } else {
        /* Start execution from root */
        execute_segment(root, segments, &results);
    }
    
    /* Return results */
    *result = results.items;
    free_segments(segments);
    
    return results.count;
}

void cJSONPath_FreeResults(cJSON** results) {
    if (results) {
        free(results);
    }
}