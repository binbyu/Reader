/**
 * @file cJSON_XPath.c
 * @brief Implementation of an XPath-like query engine for cJSON data.
 * 
 * This engine translates a subset of XPath syntax for querying JSON structures.
 * It borrows heavily from JSONPath concepts but uses the more familiar XPath notation
 * where applicable (e.g., '/' for child, '//' for recursive descent).
 */

#include "cJSON_XPath.h"
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
    TOKEN_SLASH,        /* /  */
    TOKEN_SLASHSLASH,   /* // */
    TOKEN_LBRACKET,     /* [  */
    TOKEN_RBRACKET,     /* ]  */
    TOKEN_STAR,         /* *  */
    TOKEN_DOT,          /* .  */
    TOKEN_STRING,       /* '...' or "..." */
    TOKEN_NUMBER,       /* 123 */
    TOKEN_IDENT,        /* unquoted key name */
    TOKEN_AT,           /* @  */
    TOKEN_EQ,           /* == */
    TOKEN_LT,           /* <  */
    TOKEN_GT,           /* >  */
    TOKEN_LPAREN,       /* (  */
    TOKEN_RPAREN,       /* )  */
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

static void lexer_init(lexer_t* lexer, const char* input) {
    lexer->input = input;
    lexer->pos = 0;
    lexer->len = (int)strlen(input);
    lexer->current.type = TOKEN_ERROR;
    lexer->current.value = NULL;
    lexer->current.int_value = 0;
}

static int is_ident_char(char c) {
    return isalnum(c) || c == '_' || c == '-';
}

static void free_token(token_t* token) {
    if (token->value) {
        free(token->value);
        token->value = NULL;
    }
}

/* Read next token from input */
static void next_token(lexer_t* lexer) {
    /* free previous token payload to avoid leaks */
    free_token(&lexer->current);
    while (lexer->pos < lexer->len && isspace(lexer->input[lexer->pos])) {
        lexer->pos++;
    }
    
    if (lexer->pos >= lexer->len) {
        lexer->current.type = TOKEN_EOF;
        return;
    }
    
    char c = lexer->input[lexer->pos];
    
    /* Single slash or recursive descent */
    if (c == '/') {
        lexer->pos++;
        if (lexer->pos < lexer->len && lexer->input[lexer->pos] == '/') {
            lexer->current.type = TOKEN_SLASHSLASH;
            lexer->pos++;
        } else {
            lexer->current.type = TOKEN_SLASH;
        }
        return;
    }
    
    if (c == '[') { lexer->current.type = TOKEN_LBRACKET; lexer->pos++; return; }
    if (c == ']') { lexer->current.type = TOKEN_RBRACKET; lexer->pos++; return; }
    if (c == '*') { lexer->current.type = TOKEN_STAR; lexer->pos++; return; }
    if (c == '.') { lexer->current.type = TOKEN_DOT; lexer->pos++; return; }
    if (c == '@') { lexer->current.type = TOKEN_AT; lexer->pos++; return; }
    if (c == '(') { lexer->current.type = TOKEN_LPAREN; lexer->pos++; return; }
    if (c == ')') { lexer->current.type = TOKEN_RPAREN; lexer->pos++; return; }
    
    /* Comparison operators */
    if (c == '=') {
        lexer->pos++;
        if (lexer->pos < lexer->len && lexer->input[lexer->pos] == '=') {
            lexer->current.type = TOKEN_EQ;
            lexer->pos++;
        } else {
            lexer->current.type = TOKEN_ERROR; /* Single = is invalid */
        }
        return;
    }
    if (c == '<') { lexer->current.type = TOKEN_LT; lexer->pos++; return; }
    if (c == '>') { lexer->current.type = TOKEN_GT; lexer->pos++; return; }
    
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
        if (lexer->current.value) {
            strncpy(lexer->current.value, lexer->input + start, value_len);
            lexer->current.value[value_len] = '\0';
        }
        lexer->current.type = TOKEN_STRING;
        lexer->pos++;
        return;
    }
    
    /* Number */
    if (isdigit(c) || (c == '-' && lexer->pos + 1 < lexer->len && isdigit(lexer->input[lexer->pos + 1]))) {
        int start = lexer->pos;
        if (lexer->input[lexer->pos] == '+' || lexer->input[lexer->pos] == '-') lexer->pos++;
        while (lexer->pos < lexer->len && isdigit(lexer->input[lexer->pos])) lexer->pos++;
        if (lexer->pos < lexer->len && lexer->input[lexer->pos] == '.') {
            lexer->pos++;
            while (lexer->pos < lexer->len && isdigit(lexer->input[lexer->pos])) lexer->pos++;
        }
        int value_len = lexer->pos - start;
        char* num_str = (char*)malloc(value_len + 1);
        if (!num_str) { lexer->current.type = TOKEN_ERROR; return; }
        strncpy(num_str, lexer->input + start, value_len);
        num_str[value_len] = '\0';
        lexer->current.value = num_str;
        lexer->current.int_value = atoi(num_str);
        lexer->current.type = TOKEN_NUMBER;
        return;
    }
    
    /* Identifier */
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

/* ============================================================================
 * AST Segment Definition (XPath flavored)
 * ============================================================================ */

typedef enum {
    PRED_NONE,
    PRED_INDEX,
    PRED_KEY_EQ_VAL,  /* [key='value'] */
    PRED_VAL_CMP       /* [@.key < 10] */
} predicate_type_t;

typedef struct {
    predicate_type_t type;
    int index;
    char* key;
    char* val;
    char* cmp_key;
    int cmp_op; /* 0: <, 1: >, 2: == */
    double cmp_val;
} predicate_t;

typedef struct segment_s {
    enum {
        SEG_ROOT,
        SEG_RECURSIVE,    
        SEG_KEY,
        SEG_INDEX,
        SEG_WILDCARD
    } type;
    
    union {
        char* key;
        int index;
    };
    
    predicate_t* predicate; /* New: filter predicate */
    struct segment_s* next;
} segment_t;

/* Parse a predicate */
static predicate_t* parse_predicate(lexer_t* lexer) {
    predicate_t* pred = (predicate_t*)calloc(1, sizeof(predicate_t));
    pred->type = PRED_NONE;

    if (lexer->current.type == TOKEN_NUMBER) {
        pred->type = PRED_INDEX;
        pred->index = lexer->current.int_value;
        free_token(&lexer->current);
        next_token(lexer);
        return pred;
    }
    
    if (lexer->current.type == TOKEN_IDENT || lexer->current.type == TOKEN_AT) {
        char* first_id = NULL;
        int is_at_expr = (lexer->current.type == TOKEN_AT);
        
        if (is_at_expr) {
            next_token(lexer); /* consume @ */
            if (lexer->current.type == TOKEN_DOT) next_token(lexer);
            if (lexer->current.type != TOKEN_IDENT) {
                fprintf(stderr, "Expected identifier after @\n");
                free(pred);
                return NULL;
            }
            first_id = _strdup(lexer->current.value);
            free_token(&lexer->current);
            next_token(lexer);
        } else {
            first_id = _strdup(lexer->current.value);
            free_token(&lexer->current);
            next_token(lexer);
        }

        if (lexer->current.type == TOKEN_EQ) {
            next_token(lexer);
            if (lexer->current.type == TOKEN_STRING) {
                if (is_at_expr) {
                    pred->type = PRED_VAL_CMP;
                    pred->cmp_key = first_id;
                    pred->cmp_op = 2; /* == */
                    pred->val = _strdup(lexer->current.value);
                } else {
                    pred->type = PRED_KEY_EQ_VAL;
                    pred->key = first_id;
                    pred->val = _strdup(lexer->current.value);
                }
                free_token(&lexer->current);
                next_token(lexer);
            }
        } else if (lexer->current.type == TOKEN_LT || lexer->current.type == TOKEN_GT) {
            pred->type = PRED_VAL_CMP;
            pred->cmp_key = first_id;
            pred->cmp_op = (lexer->current.type == TOKEN_LT) ? 0 : 1;
            next_token(lexer);
            if (lexer->current.type == TOKEN_NUMBER) {
                pred->cmp_val = atof(lexer->current.value);
                free_token(&lexer->current);
                next_token(lexer);
            }
        } else {
            /* Just a single identifier like [isbn] - treat as key-exists */
            pred->type = PRED_KEY_EQ_VAL;
            pred->key = first_id;
            pred->val = NULL;
        }
        return pred;
    }
    
    fprintf(stderr, "Invalid predicate\n");
    free(pred);
    return NULL;
}

static void free_segments(segment_t* seg) {
    while (seg) {
        segment_t* next = seg->next;
        if (seg->type == SEG_KEY && seg->key) free(seg->key);
        if (seg->predicate) {
            if (seg->predicate->key) free(seg->predicate->key);
            if (seg->predicate->val) free(seg->predicate->val);
            if (seg->predicate->cmp_key) free(seg->predicate->cmp_key);
            free(seg->predicate);
        }
        free(seg);
        seg = next;
    }
}

/* Parse XPath-like path string into a linked list of segments */
static segment_t* parse_xpath(const char* path) {
    lexer_t lexer;
    lexer_init(&lexer, path);
    next_token(&lexer);

    segment_t dummy = { 0 };
    segment_t* tail = &dummy;

    /* XPath must start with / or // */
    if (lexer.current.type != TOKEN_SLASH && lexer.current.type != TOKEN_SLASHSLASH) {
        fprintf(stderr, "XPath must start with '/' or '//'\n");
        return NULL;
    }

    /* Always add a ROOT segment first */
    segment_t* root_seg = (segment_t*)calloc(1, sizeof(segment_t));
    root_seg->type = SEG_ROOT;
    tail->next = root_seg;
    tail = root_seg;

    /* Handle initial // (recursive descent) */
    if (lexer.current.type == TOKEN_SLASHSLASH) {
        segment_t* rec_seg = (segment_t*)calloc(1, sizeof(segment_t));
        rec_seg->type = SEG_RECURSIVE;
        tail->next = rec_seg;
        tail = rec_seg;
        next_token(&lexer);
    }
    else {
        /* Single / - just consume it */
        next_token(&lexer);
    }

    /* If only "/" or "//" is specified, return current segments */
    if (lexer.current.type == TOKEN_EOF) {
        return dummy.next;
    }

    /* Parse remaining path segments */
    while (lexer.current.type != TOKEN_EOF && lexer.current.type != TOKEN_ERROR) {
        segment_t* seg = NULL;

        /* Handle // in the middle of path - like /company//name */
        if (lexer.current.type == TOKEN_SLASHSLASH) {
            seg = (segment_t*)calloc(1, sizeof(segment_t));
            seg->type = SEG_RECURSIVE;
            next_token(&lexer);

            /* After //, we might have an identifier or wildcard */
            /* Continue the loop to parse it as the next segment */
            tail->next = seg;
            tail = seg;
            continue;
        }

        /* Handle a single / separator - just skip it and continue */
        if (lexer.current.type == TOKEN_SLASH) {
            next_token(&lexer);
            continue;
        }

        /* Parse identifier */
        if (lexer.current.type == TOKEN_IDENT) {
            seg = (segment_t*)calloc(1, sizeof(segment_t));
            seg->type = SEG_KEY;
            seg->key = _strdup(lexer.current.value);
            free_token(&lexer.current);
            next_token(&lexer);
        }
        /* Parse wildcard */
        else if (lexer.current.type == TOKEN_STAR) {
            seg = (segment_t*)calloc(1, sizeof(segment_t));
            seg->type = SEG_WILDCARD;
            next_token(&lexer);
        }
        /* Parse bracket expression [index] or [predicate] */
        else if (lexer.current.type == TOKEN_LBRACKET) {
            seg = (segment_t*)calloc(1, sizeof(segment_t));
            next_token(&lexer);

            if (lexer.current.type == TOKEN_NUMBER) {
                seg->type = SEG_INDEX;
                seg->index = lexer.current.int_value;
                free_token(&lexer.current);
                next_token(&lexer);
            }
            else {
                /* Complex predicate */
                seg->type = SEG_KEY; /* Placeholder, will be filtered */
                seg->predicate = parse_predicate(&lexer);
            }

            if (lexer.current.type != TOKEN_RBRACKET) {
                fprintf(stderr, "Missing ']' in XPath\n");
                free_segments(dummy.next);
                return NULL;
            }
            next_token(&lexer);
        }
        else {
            fprintf(stderr, "Unexpected token type %d in XPath\n", lexer.current.type);
            free_segments(dummy.next);
            return NULL;
        }

        /* Add segment to list */
        tail->next = seg;
        tail = seg;

        /* Skip trailing slash if present */
        if (lexer.current.type == TOKEN_SLASH) {
            next_token(&lexer);
        }
    }

    return dummy.next;
}

/* ============================================================================
 * Execution Engine
 * ============================================================================ */

typedef struct {
    cJSON** items;
    int count;
    int capacity;
} result_list_t;

static void result_list_init(result_list_t* list) {
    list->count = 0;
    list->capacity = 8;
    list->items = (cJSON**)malloc(sizeof(cJSON*) * list->capacity);
}

static int result_list_contains(result_list_t* list, cJSON* item) {
    for (int i = 0; i < list->count; i++) {
        if (list->items[i] == item) return 1;
    }
    return 0;
}

static void result_list_add_unique(result_list_t* list, cJSON* item) {
    if (!result_list_contains(list, item)) {
        if (list->count >= list->capacity) {
            list->capacity *= 2;
            list->items = (cJSON**)realloc(list->items, sizeof(cJSON*) * list->capacity);
        }
        list->items[list->count++] = item;
    }
}

static int evaluate_predicate(cJSON* item, predicate_t* pred) {
    if (!pred || pred->type == PRED_NONE) return 1;
    
    switch (pred->type) {
        case PRED_INDEX:
            /* This is handled by the segment itself */
            return 1;
            
        case PRED_KEY_EQ_VAL:
            if (cJSON_IsObject(item)) {
                cJSON* child = cJSON_GetObjectItem(item, pred->key);
                if (!child) return 0;
                if (pred->val == NULL) return 1; /* Just check existence */
                char* child_str = cJSON_PrintUnformatted(child);
                /* Strip quotes if it's a string */
                char cmp_buf[256];
                if (child->type == cJSON_String) {
                    snprintf(cmp_buf, sizeof(cmp_buf), "\"%s\"", child->valuestring);
                } else {
                    strncpy(cmp_buf, child_str, sizeof(cmp_buf));
                }
                free(child_str);
                return strcmp(cmp_buf, pred->val) == 0;
            }
            return 0;
            
        case PRED_VAL_CMP:
            if (cJSON_IsObject(item)) {
                cJSON* child = cJSON_GetObjectItem(item, pred->cmp_key);
                if (!child || !cJSON_IsNumber(child)) return 0;
                double val = child->valuedouble;
                switch (pred->cmp_op) {
                    case 0: return val < pred->cmp_val;
                    case 1: return val > pred->cmp_val;
                    case 2: {
                        /* String comparison for == */
                        if (pred->val && child->type == cJSON_String) {
                            return strcmp(child->valuestring, pred->val) == 0;
                        }
                        return 0;
                    }
                }
            }
            return 0;
    }
    return 1;
}

/* Forward declaration */
static void execute_segment(cJSON* current, segment_t* seg, result_list_t* results);

static void execute_recursive_descent(cJSON* current, segment_t* seg, result_list_t* results) {
    if (!current || !seg) return;

    /* Match current node against the recursive target */
    if (seg->type == SEG_KEY) {
        if (cJSON_IsObject(current)) {
            cJSON* child = cJSON_GetObjectItem(current, seg->key);
            if (child && evaluate_predicate(child, seg->predicate)) {
                if (seg->next) {
                    execute_segment(child, seg->next, results);
                }
                else {
                    result_list_add_unique(results, child);
                }
            }
        }

        /* Also check array elements directly */
        if (cJSON_IsArray(current)) {
            int size = cJSON_GetArraySize(current);
            for (int i = 0; i < size; i++) {
                cJSON* item = cJSON_GetArrayItem(current, i);
                if (cJSON_IsObject(item)) {
                    cJSON* child = cJSON_GetObjectItem(item, seg->key);
                    if (child && evaluate_predicate(child, seg->predicate)) {
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
    else if (seg->type == SEG_WILDCARD) {
        /* Match all children */
        if (cJSON_IsObject(current)) {
            cJSON* child = current->child;
            while (child) {
                if (seg->next) {
                    execute_segment(child, seg->next, results);
                }
                else {
                    result_list_add_unique(results, child);
                }
                child = child->next;
            }
        }
        else if (cJSON_IsArray(current)) {
            int size = cJSON_GetArraySize(current);
            for (int i = 0; i < size; i++) {
                cJSON* item = cJSON_GetArrayItem(current, i);
                if (seg->next) {
                    execute_segment(item, seg->next, results);
                }
                else {
                    result_list_add_unique(results, item);
                }
            }
        }
        else {
            /* Leaf node (string, number, bool, null) */
            if (seg->next) {
                execute_segment(current, seg->next, results);
            }
            else {
                result_list_add_unique(results, current);
            }
        }
    }
    else if (seg->type == SEG_INDEX) {
        if (cJSON_IsArray(current)) {
            int size = cJSON_GetArraySize(current);
            if (seg->index >= 0 && seg->index < size) {
                cJSON* child = cJSON_GetArrayItem(current, seg->index);
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
    else if (seg->type == SEG_RECURSIVE) {
        /* Nested recursive descent: //...//... */
        /* First, try to match from current position */
        if (seg->next) {
            execute_recursive_descent(current, seg->next, results);
        }
    }

    /* Recurse into children for deeper search */
    if (cJSON_IsObject(current)) {
        cJSON* child = current->child;
        while (child) {
            /* Don't match the same node again, just recurse */
            execute_recursive_descent(child, seg, results);
            child = child->next;
        }
    }
    else if (cJSON_IsArray(current)) {
        int size = cJSON_GetArraySize(current);
        for (int i = 0; i < size; i++) {
            cJSON* item = cJSON_GetArrayItem(current, i);
            /* Don't match the same node again, just recurse */
            execute_recursive_descent(item, seg, results);
        }
    }
    /* For leaf nodes (string, number, bool, null), no children to recurse into */
}

static void execute_segment(cJSON* current, segment_t* seg, result_list_t* results) {
    if (!current || !seg) return;

    switch (seg->type) {
    case SEG_ROOT:
        /* SEG_ROOT should pass through to next segment if it exists */
        if (seg->next) {
            execute_segment(current, seg->next, results);
        }
        else {
            /* Only add root if it's the only segment (path is "/") */
            result_list_add_unique(results, current);
        }
        break;

    case SEG_RECURSIVE:
        if (seg->next) {
            /* Recursively search for the next segment's target */
            execute_recursive_descent(current, seg->next, results);
        }
        else {
            /* // at the end means select all descendants */
            /* Match everything by adding current and recursing */
            result_list_add_unique(results, current);
            /* Recurse into children to add them too */
            if (cJSON_IsObject(current)) {
                cJSON* child = current->child;
                while (child) {
                    execute_segment(child, seg, results);
                    child = child->next;
                }
            }
            else if (cJSON_IsArray(current)) {
                int size = cJSON_GetArraySize(current);
                for (int i = 0; i < size; i++) {
                    execute_segment(cJSON_GetArrayItem(current, i), seg, results);
                }
            }
        }
        break;

    case SEG_KEY:
        if (cJSON_IsObject(current)) {
            cJSON* child = cJSON_GetObjectItem(current, seg->key);
            if (child) {
                if (seg->next) execute_segment(child, seg->next, results);
                else result_list_add_unique(results, child);
            }
        }
        break;

    case SEG_INDEX:
        if (cJSON_IsArray(current)) {
            int idx = seg->index;
            int size = cJSON_GetArraySize(current);
            if (idx >= 0 && idx < size) {
                cJSON* child = cJSON_GetArrayItem(current, idx);
                if (seg->next) execute_segment(child, seg->next, results);
                else result_list_add_unique(results, child);
            }
        }
        break;

    case SEG_WILDCARD:
        if (cJSON_IsArray(current)) {
            int size = cJSON_GetArraySize(current);
            for (int i = 0; i < size; i++) {
                cJSON* child = cJSON_GetArrayItem(current, i);
                if (seg->next) execute_segment(child, seg->next, results);
                else result_list_add_unique(results, child);
            }
        }
        else if (cJSON_IsObject(current)) {
            cJSON* child = current->child;
            while (child) {
                if (seg->next) execute_segment(child, seg->next, results);
                else result_list_add_unique(results, child);
                child = child->next;
            }
        }
        break;
    }
}

/* ============================================================================
 * Public API
 * ============================================================================ */

int cJSONXPath_Query(cJSON* root, const char* xpath, cJSON*** result) {
    if (!root || !xpath || !result) return -1;
    
    segment_t* segments = parse_xpath(xpath);
    if (!segments) return -1;
    
    result_list_t results;
    result_list_init(&results);
    
    execute_segment(root, segments, &results);
    
    *result = results.items;
    free_segments(segments);
    return results.count;
}

void cJSONXPath_FreeResults(cJSON** results) {
    if (results) free(results);
}