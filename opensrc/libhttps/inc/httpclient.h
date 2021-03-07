#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__


#ifdef __cplusplus
extern "C"
{
#endif

#ifdef LIBHTTPCLIENT_EXPORTS
#define HTTPCLIENT_API(type) __declspec(dllexport) type __stdcall
#else
#ifdef LIBHTTPS
#define HTTPCLIENT_API(type) type __stdcall
#else
#define HTTPCLIENT_API(type) __declspec(dllimport) type __stdcall
#endif
#endif

typedef void* req_handler_t;
typedef unsigned int(*progress_cb)(void *param, double dltotal, double dlnow, double ultotal, double ulnow);
typedef unsigned int(*write_data_cb)(void *data, unsigned int size, unsigned int nmemb, void *stream);
typedef struct request_result_t request_result_t;
typedef unsigned int(*complete_cb)(request_result_t *result);
typedef void (__stdcall *logger_print)(const char* format,...);

typedef enum request_method_t
{
    GET,
    POST,
    HEAD
} request_method_t;

typedef enum httpclient_errno_t
{
    succ = 0,
    fail = -1,
    comp = -2,
    clos = -3,
    nonb = -4,
} httpcli_errno_t;

typedef struct http_header_t
{
    char *name;
    char *value;
    struct http_header_t *next;
} http_header_t;

typedef struct request_t
{
    request_method_t method;
    char *url;
    char *content; // just for POST
    int   content_length; // just for POST
    char *cookie;
    write_data_cb writer; // if not set, using internal default writer
    void *stream; // for writer
    progress_cb progresser;
    void *param; // for progresser
    complete_cb completer;
    void *param1; // for completer
    void *param2; // for completer
} request_t;

typedef struct request_result_t
{
    int errno_; // httpclient_errno_t
    int cancel;
    void *param1; // request_t.param1
    void *param2; // request_t.param2
    http_header_t *header;
    int status_code;
    char *status_desc;
    char *body;
    int bodylen;
    request_t *req;
    req_handler_t handler;
} request_result_t;

typedef struct http_proxy_t
{
    int   enable;
    char *addr;
    char *user;
    char *pass;
    int   port;
} http_proxy_t;

typedef enum http_charset_t
{
    utf_8,
    gbk
} http_charset_t;

HTTPCLIENT_API(int) hapi_init(void);

HTTPCLIENT_API(int) hapi_uninit(void);

HTTPCLIENT_API(int) hapi_set_proxy(const http_proxy_t *proxy);

HTTPCLIENT_API(req_handler_t) hapi_request(const request_t *req);

HTTPCLIENT_API(int) hapi_cancel(req_handler_t handler);

HTTPCLIENT_API(request_t *) hapi_get_request_info(req_handler_t handler);

HTTPCLIENT_API(int) hapi_set_logger(logger_print logger);

HTTPCLIENT_API(int) hapi_enable_cache_cookie(int able);

HTTPCLIENT_API(int) hapi_url_encode(const char* src, char** dst);

HTTPCLIENT_API(int) hapi_url_decode(const char* src, char** dst);

HTTPCLIENT_API(int) hapi_url_free(char* dst);

HTTPCLIENT_API(int) hapi_base64_encode(const char *src, int slen, char *dst, int *dlen);

HTTPCLIENT_API(int) hapi_base64_decode(const char *src, int slen, char *dst, int *dlen);

HTTPCLIENT_API(int) hapi_is_gzip(const http_header_t *header); // 1: yes,  0: no

HTTPCLIENT_API(const char *) hapi_get_location(const http_header_t *header); // NULL: fail

HTTPCLIENT_API(http_charset_t) hapi_get_charset(const http_header_t *header); // NULL: fail


#ifdef __cplusplus
}
#endif

#endif
