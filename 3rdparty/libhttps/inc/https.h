#ifndef __HTTPS_H__
#define __HTTPS_H__

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32) || defined(_WIN64)
    #if defined(LIBHTTPS_EXPORTS)
        #define HTTPS_API(type) __declspec(dllexport) type __stdcall
    #elif defined(LIBHTTPS_STATIC)
        #define HTTPS_API(type) type __stdcall
    #else
        #define HTTPS_API(type) __declspec(dllimport) type __stdcall
    #endif
#elif defined(__linux__) || defined(__APPLE__)
    #ifdef LIBHTTPS_EXPORTS
        #define HTTPS_API(type) __attribute__((visibility("default"))) type
    #else
        #define HTTPS_API(type) type
    #endif
#else
    #define HTTPS_API(type) type
#endif

typedef struct request_handle* request_handle_t;
typedef struct request_result  request_result_t;
typedef unsigned int(*progresser_t)(void *param, double total, double offset);
typedef unsigned int(*datawriter_t)(void *data, unsigned int size, unsigned int nmemb, void *stream);
typedef unsigned int(*completer_t)(request_result_t *result);

/*
* level - 0, error
* level - 1, warn
* level - 2, info
* level - 3, debug
*/
typedef void (*logger_t)(int level, const char* text);
typedef void (*ioechor_t)(const void* data, unsigned int size, void* context);

typedef enum
{
    GET,
    POST,
    HEAD
} request_method_t;

typedef enum
{
    SUCC = 0,
    FAIL = -1,
    COMP = -2, // completed
    CLOS = -3, // socket closed
    NONB = -4, // non-block
    NOFE = -5, // no feature
    ECAN = -6, // request canceled
    ETIM = -7, // request timeout
} https_errno_t;

typedef struct http_header
{
    char *name;
    char *value;
    struct http_header *next;
} http_header_t;

typedef struct
{
    request_method_t method;
    char*            url;
    char*            content;        // for POST
    int              content_length; // for POST
    char*            extraheader;    // customer request header
    datawriter_t     writer;         // if not set, using internal default writer
    void*            stream;         // for writer
    progresser_t     progresser;
    void*            param;          // for progresser
    completer_t      completer;
    void*            param1;         // for completer
    void*            param2;         // for completer
    int              noproxy;        // 0: default, 1: not use proxy
    int              timeout;        // timeout in milliseconds, <= 0: default or no timeout
} request_t;

struct request_result
{
    https_errno_t    errno_;
    void*            param1; // request_t.param1
    void*            param2; // request_t.param2
    http_header_t*   header;
    int              status_code;
    char*            status_desc;
    char*            body;
    int              bodylen;
    request_t*       req;
    request_handle_t handle;
};

typedef struct
{
    int   enable;
    char* addr;
    char* user;
    char* pass;
    int   port;
} http_proxy_t;

typedef enum
{
    UTF8,
    GBK
} http_charset_t;

typedef struct
{
    char* protocol;
    char* user;
    char* password;
    char* host;
    int   port;
    char* path;
    char* query;
    char* fragment;
} url_parts_t;


HTTPS_API(int) https_init(void);

HTTPS_API(int) https_uninit(void);

HTTPS_API(int) https_set_timeout(int timeout); // default timeout in milliseconds, <= 0: no timeout

HTTPS_API(int) https_set_proxy(const http_proxy_t *proxy);

HTTPS_API(int) https_set_cert(int enable, const char *filename);

HTTPS_API(int) https_set_logger(logger_t logger);

HTTPS_API(int) https_set_ioechor(ioechor_t echor, void *context);

HTTPS_API(int) https_set_local_ip(const char* ip);

HTTPS_API(request_handle_t) https_request(const request_t *req);

HTTPS_API(int) https_cancel(request_handle_t handle);

HTTPS_API(request_t*) https_get_request_info(request_handle_t handle);

HTTPS_API(int) https_is_gzip(const http_header_t *header); // return 1: yes,  0: no

HTTPS_API(const char*) https_get_location(const http_header_t *header); // return NULL on error

HTTPS_API(http_charset_t) https_get_charset(const http_header_t *header);

HTTPS_API(int) https_parse_url_parts(const char *url, url_parts_t *parts);

HTTPS_API(void) https_free_url_parts(url_parts_t *parts);

HTTPS_API(int) https_url_parts_is_valid(const url_parts_t* parts); // return 1: yes,  0: no

HTTPS_API(int) https_normalize_url(const char* url, char* normalized_url);

HTTPS_API(int) https_combine_url(const char* base_url, const char* relative_url, char* combined_url);

HTTPS_API(int) https_url_encode(const char *src, char *dst, int dst_len); // return dst required size, < 0 fail

HTTPS_API(int) https_url_decode(const char *src, char *dst, int dst_len); // return dst required size, < 0 fail

HTTPS_API(int) https_base64_encode(const unsigned char *src, int src_len, char *dst, int dst_len); // return dst required size, < 0 fail

HTTPS_API(int) https_base64_decode(const char *src, int src_len, unsigned char *dst, int dst_len); // return dst required size, < 0 fail

#ifdef __cplusplus
}
#endif

#endif
