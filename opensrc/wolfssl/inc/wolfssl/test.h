/* test.h */

#ifndef wolfSSL_TEST_H
#define wolfSSL_TEST_H

#ifdef FUSION_RTOS
    #include <fclstdio.h>
    #include <fclstdlib.h>
#else
    #include <stdio.h>
    #include <stdlib.h>
#endif
#include <assert.h>
#include <ctype.h>
#ifdef HAVE_ERRNO_H
    #include <errno.h>
#endif
#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/error-ssl.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/mem_track.h>
#include <wolfssl/wolfio.h>
#if defined(SHOW_CERTS) && \
    (defined(OPENSSL_EXTRA) || defined(OPENSSL_EXTRA_X509_SMALL))
    #include <wolfssl/wolfcrypt/asn.h> /* for domain component NID value */
#endif

#ifdef ATOMIC_USER
    #include <wolfssl/wolfcrypt/aes.h>
    #include <wolfssl/wolfcrypt/arc4.h>
    #include <wolfssl/wolfcrypt/hmac.h>
#endif
#ifdef HAVE_PK_CALLBACKS
    #include <wolfssl/wolfcrypt/asn.h>
    #ifndef NO_RSA
        #include <wolfssl/wolfcrypt/rsa.h>
    #endif
    #ifdef HAVE_ECC
        #include <wolfssl/wolfcrypt/ecc.h>
    #endif /* HAVE_ECC */
    #ifndef NO_DH
        #include <wolfssl/wolfcrypt/dh.h>
    #endif /* !NO_DH */
    #ifdef HAVE_ED25519
        #include <wolfssl/wolfcrypt/ed25519.h>
    #endif /* HAVE_ED25519 */
    #ifdef HAVE_CURVE25519
        #include <wolfssl/wolfcrypt/curve25519.h>
    #endif /* HAVE_ECC */
    #ifdef HAVE_ED448
        #include <wolfssl/wolfcrypt/ed448.h>
    #endif /* HAVE_ED448 */
    #ifdef HAVE_CURVE448
        #include <wolfssl/wolfcrypt/curve448.h>
    #endif /* HAVE_ECC */
#endif /*HAVE_PK_CALLBACKS */

#ifdef USE_WINDOWS_API
    #include <winsock2.h>
    #include <process.h>
    #ifdef TEST_IPV6            /* don't require newer SDK for IPV4 */
        #include <ws2tcpip.h>
        #include <wspiapi.h>
    #endif
    #define SOCKET_T SOCKET
    #define SNPRINTF _snprintf
    #define XSLEEP_MS(t) Sleep(t)
#elif defined(WOLFSSL_MDK_ARM) || defined(WOLFSSL_KEIL_TCP_NET)
    #include <string.h>
    #include "rl_net.h"
    #define SOCKET_T int
    typedef int socklen_t ;
    #define inet_addr wolfSSL_inet_addr
    static unsigned long wolfSSL_inet_addr(const char *cp)
    {
        unsigned int a[4] ; unsigned long ret ;
        sscanf(cp, "%d.%d.%d.%d", &a[0], &a[1], &a[2], &a[3]) ;
        ret = ((a[3]<<24) + (a[2]<<16) + (a[1]<<8) + a[0]) ;
        return(ret) ;
    }
    #if defined(HAVE_KEIL_RTX)
        #define XSLEEP_MS(t)  os_dly_wait(t)
    #elif defined(WOLFSSL_CMSIS_RTOS) || defined(WOLFSSL_CMSIS_RTOSv2)
        #define XSLEEP_MS(t)  osDelay(t)
    #endif
#elif defined(WOLFSSL_TIRTOS)
    #include <string.h>
    #include <netdb.h>
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <ti/sysbios/knl/Task.h>
    struct hostent {
        char *h_name; /* official name of host */
        char **h_aliases; /* alias list */
        int h_addrtype; /* host address type */
        int h_length; /* length of address */
        char **h_addr_list; /* list of addresses from name server */
    };
    #define SOCKET_T int
    #define XSLEEP_MS(t) Task_sleep(t/1000)
#elif defined(WOLFSSL_VXWORKS)
    #include <hostLib.h>
    #include <sockLib.h>
    #include <arpa/inet.h>
    #include <string.h>
    #include <selectLib.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <sys/time.h>
    #include <netdb.h>
    #include <pthread.h>
    #define SOCKET_T int
#elif defined(WOLFSSL_ZEPHYR)
    #include <string.h>
    #include <sys/types.h>
    #include <net/socket.h>
    #define SOCKET_T int
    #define SOL_SOCKET 1
    #define WOLFSSL_USE_GETADDRINFO

    static unsigned long inet_addr(const char *cp)
    {
        unsigned int a[4]; unsigned long ret;
        int i, j;
        for (i=0, j=0; i<4; i++) {
            a[i] = 0;
            while (cp[j] != '.' && cp[j] != '\0') {
                a[i] *= 10;
                a[i] += cp[j] - '0';
                j++;
            }
        }
        ret = ((a[3]<<24) + (a[2]<<16) + (a[1]<<8) + a[0]) ;
        return(ret) ;
    }
#elif defined(NETOS)
    #include <string.h>
    #include <sys/types.h>
    struct hostent {
        char* h_name;        /* official name of host */
        char** h_aliases;    /* alias list */
        int h_addrtype;      /* host address type */
        int h_length;        /* length of address */
        char** h_addr_list;  /* list of addresses from the name server */
    };
#else
    #include <string.h>
    #include <sys/types.h>
#ifndef WOLFSSL_LEANPSK
    #include <unistd.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <sys/time.h>
    #include <sys/socket.h>
    #include <pthread.h>
    #include <fcntl.h>
    #ifdef TEST_IPV6
        #include <netdb.h>
    #endif
#endif
    #ifdef FREESCALE_MQX
        typedef int socklen_t ;
    #endif
    #define SOCKET_T int
    #ifndef SO_NOSIGPIPE
        #include <signal.h>  /* ignore SIGPIPE */
    #endif
    #define SNPRINTF snprintf

    #define XSELECT_WAIT(x,y) do { \
        struct timeval tv = {((x) + ((y) / 1000000)),((y) % 1000000)}; \
        select(0, NULL, NULL, NULL, &tv); \
    } while (0)
    #define XSLEEP_US(u) XSELECT_WAIT(0,u)
    #define XSLEEP_MS(m) XSELECT_WAIT(0,(m)*1000)
#endif /* USE_WINDOWS_API */

#ifndef XSLEEP_MS
    #define XSLEEP_MS(t) sleep(t/1000)
#endif

#ifdef WOLFSSL_ASYNC_CRYPT
    #include <wolfssl/wolfcrypt/async.h>
#endif
#ifdef HAVE_CAVIUM
    #include <wolfssl/wolfcrypt/port/cavium/cavium_nitrox.h>
#endif
#ifdef _MSC_VER
    /* disable conversion warning */
    /* 4996 warning to use MS extensions e.g., strcpy_s instead of strncpy */
    #pragma warning(disable:4244 4996)
#endif

#ifndef WOLFSSL_CIPHER_LIST_MAX_SIZE
    #define WOLFSSL_CIPHER_LIST_MAX_SIZE 4096
#endif
/* Buffer for benchmark tests */
#ifndef TEST_BUFFER_SIZE
    #define TEST_BUFFER_SIZE 16384
#endif

#ifndef WOLFSSL_HAVE_MIN
    #define WOLFSSL_HAVE_MIN
    static WC_INLINE word32 min(word32 a, word32 b)
    {
        return a > b ? b : a;
    }
#endif /* WOLFSSL_HAVE_MIN */

/* Socket Handling */
#ifndef WOLFSSL_SOCKET_INVALID
#ifdef USE_WINDOWS_API
    #define WOLFSSL_SOCKET_INVALID  ((SOCKET_T)INVALID_SOCKET)
#elif defined(WOLFSSL_TIRTOS)
    #define WOLFSSL_SOCKET_INVALID  ((SOCKET_T)-1)
#else
    #define WOLFSSL_SOCKET_INVALID  (SOCKET_T)(0)
#endif
#endif /* WOLFSSL_SOCKET_INVALID */

#ifndef WOLFSSL_SOCKET_IS_INVALID
#if defined(USE_WINDOWS_API) || defined(WOLFSSL_TIRTOS)
    #define WOLFSSL_SOCKET_IS_INVALID(s)  ((SOCKET_T)(s) == WOLFSSL_SOCKET_INVALID)
#else
    #define WOLFSSL_SOCKET_IS_INVALID(s)  ((SOCKET_T)(s) < WOLFSSL_SOCKET_INVALID)
#endif
#endif /* WOLFSSL_SOCKET_IS_INVALID */

#if defined(__MACH__) || defined(USE_WINDOWS_API)
    #ifndef _SOCKLEN_T
        typedef int socklen_t;
    #endif
#endif


/* HPUX doesn't use socklent_t for third parameter to accept, unless
   _XOPEN_SOURCE_EXTENDED is defined */
#if !defined(__hpux__) && !defined(WOLFSSL_MDK_ARM) && !defined(WOLFSSL_IAR_ARM)\
 && !defined(WOLFSSL_ROWLEY_ARM)  && !defined(WOLFSSL_KEIL_TCP_NET)
    typedef socklen_t* ACCEPT_THIRD_T;
#else
    #if defined _XOPEN_SOURCE_EXTENDED
        typedef socklen_t* ACCEPT_THIRD_T;
    #else
        typedef int*       ACCEPT_THIRD_T;
    #endif
#endif



#ifdef SINGLE_THREADED
    typedef unsigned int  THREAD_RETURN;
    typedef void*         THREAD_TYPE;
    #define WOLFSSL_THREAD
#else
    #if defined(_POSIX_THREADS) && !defined(__MINGW32__)
        typedef void*         THREAD_RETURN;
        typedef pthread_t     THREAD_TYPE;
        #define WOLFSSL_THREAD
        #define INFINITE -1
        #define WAIT_OBJECT_0 0L
    #elif defined(WOLFSSL_MDK_ARM)|| defined(WOLFSSL_KEIL_TCP_NET) || defined(FREESCALE_MQX)
        typedef unsigned int  THREAD_RETURN;
        typedef int           THREAD_TYPE;
        #define WOLFSSL_THREAD
    #elif defined(WOLFSSL_TIRTOS)
        typedef void          THREAD_RETURN;
        typedef Task_Handle   THREAD_TYPE;
        #ifdef HAVE_STACK_SIZE
          #undef EXIT_TEST
          #define EXIT_TEST(ret)
        #endif
        #define WOLFSSL_THREAD
    #elif defined(WOLFSSL_ZEPHYR)
        typedef void            THREAD_RETURN;
        typedef struct k_thread THREAD_TYPE;
        #ifdef HAVE_STACK_SIZE
          #undef EXIT_TEST
          #define EXIT_TEST(ret)
        #endif
        #define WOLFSSL_THREAD
    #elif defined(NETOS)
        typedef UINT        THREAD_RETURN;
        typedef TX_THREAD   THREAD_TYPE;
        #define WOLFSSL_THREAD
        #define INFINITE TX_WAIT_FOREVER
        #define WAIT_OBJECT_0 TX_NO_WAIT
    #else
        typedef unsigned int  THREAD_RETURN;
        typedef intptr_t      THREAD_TYPE;
        #define WOLFSSL_THREAD __stdcall
    #endif
#endif


#ifdef TEST_IPV6
    typedef struct sockaddr_in6 SOCKADDR_IN_T;
    #define AF_INET_V    AF_INET6
#else
    typedef struct sockaddr_in  SOCKADDR_IN_T;
    #define AF_INET_V    AF_INET
#endif


#ifndef WOLFSSL_NO_TLS12
#define SERVER_DEFAULT_VERSION 3
#else
#define SERVER_DEFAULT_VERSION 4
#endif
#define SERVER_DTLS_DEFAULT_VERSION (-2)
#define SERVER_INVALID_VERSION (-99)
#define SERVER_DOWNGRADE_VERSION (-98)
#ifndef WOLFSSL_NO_TLS12
#define CLIENT_DEFAULT_VERSION 3
#else
#define CLIENT_DEFAULT_VERSION 4
#endif
#define CLIENT_DTLS_DEFAULT_VERSION (-2)
#define CLIENT_INVALID_VERSION (-99)
#define CLIENT_DOWNGRADE_VERSION (-98)
#define EITHER_DOWNGRADE_VERSION (-97)
#if !defined(NO_FILESYSTEM) && defined(WOLFSSL_MAX_STRENGTH)
    #define DEFAULT_MIN_DHKEY_BITS 2048
    #define DEFAULT_MAX_DHKEY_BITS 3072
#else
    #define DEFAULT_MIN_DHKEY_BITS 1024
    #define DEFAULT_MAX_DHKEY_BITS 2048
#endif
#if !defined(NO_FILESYSTEM) && defined(WOLFSSL_MAX_STRENGTH)
    #define DEFAULT_MIN_RSAKEY_BITS 2048
#else
    #ifndef DEFAULT_MIN_RSAKEY_BITS
    #define DEFAULT_MIN_RSAKEY_BITS 1024
    #endif
#endif
#if !defined(NO_FILESYSTEM) && defined(WOLFSSL_MAX_STRENGTH)
    #define DEFAULT_MIN_ECCKEY_BITS 256
#else
    #ifndef DEFAULT_MIN_ECCKEY_BITS
    #define DEFAULT_MIN_ECCKEY_BITS 224
    #endif
#endif

/* all certs relative to wolfSSL home directory now */
#if defined(WOLFSSL_NO_CURRDIR) || defined(WOLFSSL_MDK_SHELL)
#define caCertFile        "certs/ca-cert.pem"
#define eccCertFile       "certs/server-ecc.pem"
#define eccKeyFile        "certs/ecc-key.pem"
#define eccKeyPubFile     "certs/ecc-keyPub.pem"
#define eccRsaCertFile    "certs/server-ecc-rsa.pem"
#define svrCertFile       "certs/server-cert.pem"
#define svrKeyFile        "certs/server-key.pem"
#define svrKeyPubFile     "certs/server-keyPub.pem"
#define cliCertFile       "certs/client-cert.pem"
#define cliCertDerFile    "certs/client-cert.der"
#define cliCertFileExt    "certs/client-cert-ext.pem"
#define cliCertDerFileExt "certs/client-cert-ext.der"
#define cliKeyFile        "certs/client-key.pem"
#define cliKeyPubFile     "certs/client-keyPub.pem"
#define dhParamFile       "certs/dh2048.pem"
#define cliEccKeyFile     "certs/ecc-client-key.pem"
#define cliEccKeyPubFile  "certs/ecc-client-keyPub.pem"
#define cliEccCertFile    "certs/client-ecc-cert.pem"
#define caEccCertFile     "certs/ca-ecc-cert.pem"
#define crlPemDir         "certs/crl"
#define edCertFile        "certs/ed25519/server-ed25519-cert.pem"
#define edKeyFile         "certs/ed25519/server-ed25519-priv.pem"
#define edKeyPubFile      "certs/ed25519/server-ed25519-key.pem"
#define cliEdCertFile     "certs/ed25519/client-ed25519.pem"
#define cliEdKeyFile      "certs/ed25519/client-ed25519-priv.pem"
#define cliEdKeyPubFile   "certs/ed25519/client-ed25519-key.pem"
#define caEdCertFile      "certs/ed25519/ca-ed25519.pem"
#define ed448CertFile     "certs/ed448/server-ed448-cert.pem"
#define ed448KeyFile      "certs/ed448/server-ed448-priv.pem"
#define cliEd448CertFile  "certs/ed448/client-ed448.pem"
#define cliEd448KeyFile   "certs/ed448/client-ed448-priv.pem"
#define caEd448CertFile   "certs/ed448/ca-ed448.pem"
#define caCertFolder      "certs/"
#ifdef HAVE_WNR
    /* Whitewood netRandom default config file */
    #define wnrConfig     "wnr-example.conf"
#endif
#elif defined(NETOS) && defined(HAVE_FIPS)
    /* These defines specify the file system volume and root directory used by
     * the FTP server used in the only supported NETOS FIPS solution (at this
     * time), these can be tailored in the event a future FIPS solution is added
     * for an alternate NETOS use-case */
    #define FS_VOLUME1     "FLASH0"
    #define FS_VOLUME1_DIR FS_VOLUME1 "/"
    #define caCertFile     FS_VOLUME1_DIR "certs/ca-cert.pem"
    #define eccCertFile    FS_VOLUME1_DIR "certs/server-ecc.pem"
    #define eccKeyFile     FS_VOLUME1_DIR "certs/ecc-key.pem"
    #define svrCertFile    FS_VOLUME1_DIR "certs/server-cert.pem"
    #define svrKeyFile     FS_VOLUME1_DIR "certs/server-key.pem"
    #define cliCertFile    FS_VOLUME1_DIR "certs/client-cert.pem"
    #define cliKeyFile     FS_VOLUME1_DIR "certs/client-key.pem"
    #define ntruCertFile   FS_VOLUME1_DIR "certs/ntru-cert.pem"
    #define ntruKeyFile    FS_VOLUME1_DIR "certs/ntru-key.raw"
    #define dhParamFile    FS_VOLUME1_DIR "certs/dh2048.pem"
    #define cliEccKeyFile  FS_VOLUME1_DIR "certs/ecc-client-key.pem"
    #define cliEccCertFile FS_VOLUME1_DIR "certs/client-ecc-cert.pem"
    #define caEccCertFile  FS_VOLUME1_DIR "certs/ca-ecc-cert/pem"
    #define crlPemDir      FS_VOLUME1_DIR "certs/crl"
    #ifdef HAVE_WNR
        /* Whitewood netRandom default config file */
        #define wnrConfig  "wnr-example.conf"
    #endif
#else
#define caCertFile        "./certs/ca-cert.pem"
#define eccCertFile       "./certs/server-ecc.pem"
#define eccKeyFile        "./certs/ecc-key.pem"
#define eccKeyPubFile     "./certs/ecc-keyPub.pem"
#define eccRsaCertFile    "./certs/server-ecc-rsa.pem"
#define svrCertFile       "./certs/server-cert.pem"
#define svrKeyFile        "./certs/server-key.pem"
#define svrKeyPubFile     "./certs/server-keyPub.pem"
#define cliCertFile       "./certs/client-cert.pem"
#define cliCertDerFile    "./certs/client-cert.der"
#define cliCertFileExt    "./certs/client-cert-ext.pem"
#define cliCertDerFileExt "./certs/client-cert-ext.der"
#define cliKeyFile        "./certs/client-key.pem"
#define cliKeyPubFile     "./certs/client-keyPub.pem"
#define dhParamFile       "./certs/dh2048.pem"
#define cliEccKeyFile     "./certs/ecc-client-key.pem"
#define cliEccKeyPubFile  "./certs/ecc-client-keyPub.pem"
#define cliEccCertFile    "./certs/client-ecc-cert.pem"
#define caEccCertFile     "./certs/ca-ecc-cert.pem"
#define crlPemDir         "./certs/crl"
#define edCertFile        "./certs/ed25519/server-ed25519-cert.pem"
#define edKeyFile         "./certs/ed25519/server-ed25519-priv.pem"
#define edKeyPubFile      "./certs/ed25519/server-ed25519-key.pem"
#define cliEdCertFile     "./certs/ed25519/client-ed25519.pem"
#define cliEdKeyFile      "./certs/ed25519/client-ed25519-priv.pem"
#define cliEdKeyPubFile   "./certs/ed25519/client-ed25519-key.pem"
#define caEdCertFile      "./certs/ed25519/ca-ed25519.pem"
#define ed448CertFile     "./certs/ed448/server-ed448-cert.pem"
#define ed448KeyFile      "./certs/ed448/server-ed448-priv.pem"
#define cliEd448CertFile  "./certs/ed448/client-ed448.pem"
#define cliEd448KeyFile   "./certs/ed448/client-ed448-priv.pem"
#define caEd448CertFile   "./certs/ed448/ca-ed448.pem"
#define caCertFolder      "./certs/"
#ifdef HAVE_WNR
    /* Whitewood netRandom default config file */
    #define wnrConfig     "./wnr-example.conf"
#endif
#endif

typedef struct tcp_ready {
    word16 ready;              /* predicate */
    word16 port;
    char*  srfName;     /* server ready file name */
#if defined(_POSIX_THREADS) && !defined(__MINGW32__)
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
#endif
#ifdef NETOS
    TX_MUTEX mutex;
#endif
} tcp_ready;


static WC_INLINE void InitTcpReady(tcp_ready* ready)
{
    ready->ready = 0;
    ready->port = 0;
    ready->srfName = NULL;
#ifdef SINGLE_THREADED
#elif defined(_POSIX_THREADS) && !defined(__MINGW32__)
    pthread_mutex_init(&ready->mutex, 0);
    pthread_cond_init(&ready->cond, 0);
#elif defined(NETOS)
    tx_mutex_create(&ready->mutex, "wolfSSL Lock", TX_INHERIT);
#endif
}

#ifdef NETOS
    struct hostent* gethostbyname(vonst char* name);
#endif

static WC_INLINE void FreeTcpReady(tcp_ready* ready)
{
#ifdef SINGLE_THREADED
    (void)ready;
#elif defined(_POSIX_THREADS) && !defined(__MINGW32__)
    pthread_mutex_destroy(&ready->mutex);
    pthread_cond_destroy(&ready->cond);
#elif defined(NETOS)
    tx_mutex_delete(&ready->mutex);
#else
    (void)ready;
#endif
}

typedef WOLFSSL_METHOD* (*method_provider)(void);
typedef void (*ctx_callback)(WOLFSSL_CTX* ctx);
typedef void (*ssl_callback)(WOLFSSL* ssl);

typedef struct callback_functions {
    method_provider method;
    ctx_callback ctx_ready;
    ssl_callback ssl_ready;
    ssl_callback on_result;
    WOLFSSL_CTX* ctx;
    const char* caPemFile;
    const char* certPemFile;
    const char* keyPemFile;
#ifdef WOLFSSL_STATIC_MEMORY
    byte*               mem;
    word32              memSz;
    wolfSSL_method_func method_ex;
#endif
    int devId;
    int return_code;
    unsigned char isSharedCtx:1;
    unsigned char loadToSSL:1;
} callback_functions;

typedef struct func_args {
    int    argc;
    char** argv;
    int    return_code;
    tcp_ready* signal;
    callback_functions *callbacks;
} func_args;

#ifdef NETOS
    int dc_log_printf(char* format, ...);
    #undef printf
    #define printf dc_log_printf
#endif

void wait_tcp_ready(func_args*);

#ifdef WOLFSSL_ZEPHYR
typedef void THREAD_FUNC(void*, void*, void*);
#else
typedef THREAD_RETURN WOLFSSL_THREAD THREAD_FUNC(void*);
#endif

void start_thread(THREAD_FUNC, func_args*, THREAD_TYPE*);
void join_thread(THREAD_TYPE);

/* wolfSSL */
#ifndef TEST_IPV6
    static const char* const wolfSSLIP   = "127.0.0.1";
#else
    static const char* const wolfSSLIP   = "::1";
#endif
static const word16      wolfSSLPort = 11111;



#ifndef MY_EX_USAGE
#define MY_EX_USAGE 2
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#if defined(WOLFSSL_FORCE_MALLOC_FAIL_TEST) || defined(WOLFSSL_ZEPHYR)
    #ifndef EXIT_SUCCESS
        #define EXIT_SUCCESS   0
    #endif
    #define XEXIT(rc)   return rc
    #define XEXIT_T(rc) return (THREAD_RETURN)rc
#else
    #define XEXIT(rc)   exit((int)(rc))
    #define XEXIT_T(rc) exit((int)(rc))
#endif


static WC_INLINE
#if defined(WOLFSSL_FORCE_MALLOC_FAIL_TEST) || defined(WOLFSSL_ZEPHYR)
THREAD_RETURN
#else
WC_NORETURN void
#endif
err_sys(const char* msg)
{
#if !defined(__GNUC__)
    /* scan-build (which pretends to be gnuc) can get confused and think the
     * msg pointer can be null even when hardcoded and then it won't exit,
     * making null pointer checks above the err_sys() call useless.
     * We could just always exit() but some compilers will complain about no
     * possible return, with gcc we know the attribute to handle that with
     * WC_NORETURN. */
    if (msg)
#endif
    {
        printf("wolfSSL error: %s\n", msg);

        XEXIT_T(EXIT_FAILURE);
    }
}

static WC_INLINE
#if defined(WOLFSSL_FORCE_MALLOC_FAIL_TEST) || defined(WOLFSSL_ZEPHYR)
THREAD_RETURN
#else
WC_NORETURN void
#endif
err_sys_with_errno(const char* msg)
{
#if !defined(__GNUC__)
    /* scan-build (which pretends to be gnuc) can get confused and think the
     * msg pointer can be null even when hardcoded and then it won't exit,
     * making null pointer checks above the err_sys() call useless.
     * We could just always exit() but some compilers will complain about no
     * possible return, with gcc we know the attribute to handle that with
     * WC_NORETURN. */
    if (msg)
#endif
    {
#if defined(HAVE_STRING_H) && defined(HAVE_ERRNO_H)
        printf("wolfSSL error: %s: %s\n", msg, strerror(errno));
#else
        printf("wolfSSL error: %s\n", msg);
#endif

        XEXIT_T(EXIT_FAILURE);
    }
}


extern int   myoptind;
extern char* myoptarg;

/**
 *
 * @param argc Number of argv strings
 * @param argv Array of string arguments
 * @param optstring String containing the supported alphanumeric arguments.
 *                  A ':' following a character means that it requires a
 *                  value in myoptarg to be set. A ';' means that the
 *                  myoptarg is optional. myoptarg is set to "" if not
 *                  present.
 * @return Option letter in argument
 */
static WC_INLINE int mygetopt(int argc, char** argv, const char* optstring)
{
    static char* next = NULL;

    char  c;
    char* cp;

    /* Added sanity check because scan-build complains argv[myoptind] access
     * results in a null pointer dereference. */
    if (argv == NULL)  {
        myoptarg = NULL;
        return -1;
    }

    if (myoptind == 0)
        next = NULL;   /* we're starting new/over */

    if (next == NULL || *next == '\0') {
        if (myoptind == 0)
            myoptind++;

        if (myoptind >= argc || argv[myoptind] == NULL ||
                argv[myoptind][0] != '-' || argv[myoptind][1] == '\0') {
            myoptarg = NULL;
            if (myoptind < argc)
                myoptarg = argv[myoptind];

            return -1;
        }

        if (strcmp(argv[myoptind], "--") == 0) {
            myoptind++;
            myoptarg = NULL;

            if (myoptind < argc)
                myoptarg = argv[myoptind];

            return -1;
        }

        next = argv[myoptind];
        next++;                  /* skip - */
        myoptind++;
    }

    c  = *next++;
    /* The C++ strchr can return a different value */
    cp = (char*)strchr(optstring, c);

    if (cp == NULL || c == ':' || c == ';')
        return '?';

    cp++;

    if (*cp == ':') {
        if (*next != '\0') {
            myoptarg = next;
            next     = NULL;
        }
        else if (myoptind < argc) {
            myoptarg = argv[myoptind];
            myoptind++;
        }
        else
            return '?';
    }
    else if (*cp == ';') {
        myoptarg = (char*)"";
        if (*next != '\0') {
            myoptarg = next;
            next     = NULL;
        }
        else if (myoptind < argc) {
            /* Check if next argument is not a parameter argument */
            if (argv[myoptind] && argv[myoptind][0] != '-') {
                myoptarg = argv[myoptind];
                myoptind++;
            }
        }
    }

    return c;
}

struct mygetopt_long_config {
    const char *name;
    int takes_arg;
    int value;
};

/**
 *
 * @param argc Number of argv strings
 * @param argv Array of string arguments
 * @param optstring String containing the supported alphanumeric arguments.
 *                  A ':' following a character means that it requires a
 *                  value in myoptarg to be set. A ';' means that the
 *                  myoptarg is optional. myoptarg is set to "" if not
 *                  present.
 * @return Option letter in argument
 */
static WC_INLINE int mygetopt_long(int argc, char** argv, const char* optstring,
    const struct mygetopt_long_config *longopts, int *longindex)
{
    static char* next = NULL;

    int  c;
    char* cp;

    /* Added sanity check because scan-build complains argv[myoptind] access
     * results in a null pointer dereference. */
    if (argv == NULL)  {
        myoptarg = NULL;
        return -1;
    }

    if (myoptind == 0)
        next = NULL;   /* we're starting new/over */

    if (next == NULL || *next == '\0') {
        if (myoptind == 0)
            myoptind++;

        if (myoptind >= argc || argv[myoptind] == NULL ||
                argv[myoptind][0] != '-' || argv[myoptind][1] == '\0') {
            myoptarg = NULL;
            if (myoptind < argc)
                myoptarg = argv[myoptind];

            return -1;
        }

        if (strcmp(argv[myoptind], "--") == 0) {
            myoptind++;
            myoptarg = NULL;

            if (myoptind < argc)
                myoptarg = argv[myoptind];

            return -1;
        }

        if (strncmp(argv[myoptind], "--", 2) == 0) {
            const struct mygetopt_long_config *i;
            c = -1;
            myoptarg = NULL;
            for (i = longopts; i->name; ++i) {
                if (! strcmp(argv[myoptind] + 2, i->name)) {
                    c = i->value;
                    myoptind++;
                    if (longindex)
                        *longindex = (int)((i - longopts) / sizeof *i);
                    if (i->takes_arg) {
                        if (myoptind < argc) {
                            myoptarg = argv[myoptind];
                            myoptind++;
                        } else
                            return -1;
                    }
                    break;
                }
            }

            return c;
        }

        next = argv[myoptind];
        next++;                  /* skip - */
        myoptind++;
    }

    c  = *next++;
    /* The C++ strchr can return a different value */
    cp = (char*)strchr(optstring, c);

    if (cp == NULL || c == ':' || c == ';')
        return '?';

    cp++;

    if (*cp == ':') {
        if (*next != '\0') {
            myoptarg = next;
            next     = NULL;
        }
        else if (myoptind < argc) {
            myoptarg = argv[myoptind];
            myoptind++;
        }
        else
            return '?';
    }
    else if (*cp == ';') {
        myoptarg = (char*)"";
        if (*next != '\0') {
            myoptarg = next;
            next     = NULL;
        }
        else if (myoptind < argc) {
            /* Check if next argument is not a parameter argument */
            if (argv[myoptind] && argv[myoptind][0] != '-') {
                myoptarg = argv[myoptind];
                myoptind++;
            }
        }
    }

    return c;
}


#ifdef WOLFSSL_ENCRYPTED_KEYS

static WC_INLINE int PasswordCallBack(char* passwd, int sz, int rw, void* userdata)
{
    (void)rw;
    (void)userdata;
    if (userdata != NULL) {
        strncpy(passwd, (char*)userdata, sz);
        return (int)XSTRLEN((char*)userdata);
    }
    else {
        strncpy(passwd, "yassl123", sz);
        return 8;
    }
}

#endif

static const char* client_showpeer_msg[][9] = {
    /* English */
    {
        "SSL version is",
        "SSL cipher suite is",
        "SSL signature algorithm is",
        "SSL curve name is",
        "SSL DH size is",
        "SSL reused session",
        "Alternate cert chain used",
        "peer's cert info:",
        NULL
    },
#ifndef NO_MULTIBYTE_PRINT
    /* Japanese */
    {
        "SSL バージョンは",
        "SSL 暗号スイートは",
        "SSL signature algorithm is",
        "SSL 曲線名は",
        "SSL DH サイズは",
        "SSL 再利用セッション",
        "代替証明チェーンを使用",
        "相手方証明書情報",
        NULL
    },
#endif
};

#if defined(KEEP_PEER_CERT) || defined(KEEP_OUR_CERT) || defined(SESSION_CERTS)
static const char* client_showx509_msg[][5] = {
    /* English */
    {
        "issuer",
        "subject",
        "altname",
        "serial number",
        NULL
    },
#ifndef NO_MULTIBYTE_PRINT
    /* Japanese */
    {
        "発行者",
        "サブジェクト",
        "代替名",
        "シリアル番号",
        NULL
    },
#endif
};

/* lng_index is to specify the language for displaying message.              */
/* 0:English, 1:Japanese                                                     */
static WC_INLINE void ShowX509Ex(WOLFSSL_X509* x509, const char* hdr,
                                                                 int lng_index)
{
    char* altName;
    char* issuer;
    char* subject;
    byte  serial[32];
    int   ret;
    int   sz = sizeof(serial);
    const char** words = client_showx509_msg[lng_index];

    if (x509 == NULL) {
        printf("%s No Cert\n", hdr);
        return;
    }

    issuer  = wolfSSL_X509_NAME_oneline(
                                      wolfSSL_X509_get_issuer_name(x509), 0, 0);
    subject = wolfSSL_X509_NAME_oneline(
                                     wolfSSL_X509_get_subject_name(x509), 0, 0);

    printf("%s\n %s : %s\n %s: %s\n", hdr, words[0], issuer, words[1], subject);

    while ( (altName = wolfSSL_X509_get_next_altname(x509)) != NULL)
        printf(" %s = %s\n", words[2], altName);

    ret = wolfSSL_X509_get_serial_number(x509, serial, &sz);
    if (ret == WOLFSSL_SUCCESS) {
        int  i;
        int  strLen;
        char serialMsg[80];

        /* testsuite has multiple threads writing to stdout, get output
           message ready to write once */
        strLen = sprintf(serialMsg, " %s", words[3]);
        for (i = 0; i < sz; i++)
            sprintf(serialMsg + strLen + (i*3), ":%02x ", serial[i]);
        printf("%s\n", serialMsg);
    }

    XFREE(subject, 0, DYNAMIC_TYPE_OPENSSL);
    XFREE(issuer,  0, DYNAMIC_TYPE_OPENSSL);

#if defined(SHOW_CERTS) && defined(OPENSSL_EXTRA)
    {
        WOLFSSL_BIO* bio;
        char buf[256]; /* should be size of ASN_NAME_MAX */
        int  textSz;

        /* print out domain component if certificate has it */
        textSz = wolfSSL_X509_NAME_get_text_by_NID(
                wolfSSL_X509_get_subject_name(x509), NID_domainComponent,
                buf, sizeof(buf));
        if (textSz > 0) {
            printf("Domain Component = %s\n", buf);
        }

        bio = wolfSSL_BIO_new(wolfSSL_BIO_s_file());
        if (bio != NULL) {
            wolfSSL_BIO_set_fp(bio, stdout, BIO_NOCLOSE);
            wolfSSL_X509_print(bio, x509);
            wolfSSL_BIO_free(bio);
        }
    }
#endif /* SHOW_CERTS && OPENSSL_EXTRA */
}
/* original ShowX509 to maintain compatibility */
static WC_INLINE void ShowX509(WOLFSSL_X509* x509, const char* hdr)
{
    ShowX509Ex(x509, hdr, 0);
}

#endif /* KEEP_PEER_CERT || KEEP_OUR_CERT || SESSION_CERTS */

#if defined(SHOW_CERTS) && defined(SESSION_CERTS) && \
    (defined(OPENSSL_EXTRA) || defined(OPENSSL_EXTRA_X509_SMALL))
static WC_INLINE void ShowX509Chain(WOLFSSL_X509_CHAIN* chain, int count,
    const char* hdr)
{
    int i;
    int length;
    unsigned char buffer[3072];
    WOLFSSL_X509* chainX509;

    for (i = 0; i < count; i++) {
        wolfSSL_get_chain_cert_pem(chain, i, buffer, sizeof(buffer), &length);
        buffer[length] = 0;
        printf("\n%s: %d has length %d data = \n%s\n", hdr, i, length, buffer);

        chainX509 = wolfSSL_get_chain_X509(chain, i);
        if (chainX509)
            ShowX509(chainX509, hdr);
        else
            printf("get_chain_X509 failed\n");
        wolfSSL_FreeX509(chainX509);
    }
}
#endif /* SHOW_CERTS && SESSION_CERTS */

/* lng_index is to specify the language for displaying message.              */
/* 0:English, 1:Japanese                                                     */
static WC_INLINE void showPeerEx(WOLFSSL* ssl, int lng_index)
{
    WOLFSSL_CIPHER* cipher;
    const char** words = client_showpeer_msg[lng_index];

#if defined(HAVE_ECC) || defined(HAVE_CURVE25519) || defined(HAVE_CURVE448) || \
                                                                 !defined(NO_DH)
    const char *name;
#endif
#ifndef NO_DH
    int bits;
#endif
#ifdef OPENSSL_EXTRA
    int nid;
#endif
#ifdef KEEP_PEER_CERT
    WOLFSSL_X509* peer = wolfSSL_get_peer_certificate(ssl);
    if (peer)
        ShowX509Ex(peer, words[6], lng_index);
    else
        printf("peer has no cert!\n");
    wolfSSL_FreeX509(peer);
#endif
#if defined(SHOW_CERTS) && defined(KEEP_OUR_CERT) && \
    (defined(OPENSSL_EXTRA) || defined(OPENSSL_EXTRA_X509_SMALL))
    ShowX509(wolfSSL_get_certificate(ssl), "our cert info:");
    printf("Peer verify result = %lu\n", wolfSSL_get_verify_result(ssl));
#endif /* SHOW_CERTS && KEEP_OUR_CERT */
    printf("%s %s\n", words[0], wolfSSL_get_version(ssl));

    cipher = wolfSSL_get_current_cipher(ssl);
    printf("%s %s\n", words[1], wolfSSL_CIPHER_get_name(cipher));
#ifdef OPENSSL_EXTRA
    if (wolfSSL_get_signature_nid(ssl, &nid) == WOLFSSL_SUCCESS) {
        printf("%s %s\n", words[2], OBJ_nid2sn(nid));
    }
#endif
#if defined(HAVE_ECC) || defined(HAVE_CURVE25519) || defined(HAVE_CURVE448) || \
                                                                 !defined(NO_DH)
    if ((name = wolfSSL_get_curve_name(ssl)) != NULL)
        printf("%s %s\n", words[3], name);
#endif
#ifndef NO_DH
    else if ((bits = wolfSSL_GetDhKey_Sz(ssl)) > 0)
        printf("%s %d bits\n", words[4], bits);
#endif
    if (wolfSSL_session_reused(ssl))
        printf("%s\n", words[5]);
#ifdef WOLFSSL_ALT_CERT_CHAINS
    if (wolfSSL_is_peer_alt_cert_chain(ssl))
        printf("%s\n", words[6]);
#endif

#if defined(SHOW_CERTS) && defined(SESSION_CERTS) && \
    (defined(OPENSSL_EXTRA) || defined(OPENSSL_EXTRA_X509_SMALL))
    {
        WOLFSSL_X509_CHAIN* chain;

        chain = wolfSSL_get_peer_chain(ssl);
        ShowX509Chain(chain, wolfSSL_get_chain_count(chain), "session cert");

    #ifdef WOLFSSL_ALT_CERT_CHAINS
        if (wolfSSL_is_peer_alt_cert_chain(ssl)) {
            chain = wolfSSL_get_peer_alt_chain(ssl);
            ShowX509Chain(chain, wolfSSL_get_chain_count(chain), "alt cert");
        }
    #endif
    }
#endif /* SHOW_CERTS && SESSION_CERTS */
  (void)ssl;
}
/* original showPeer to maintain compatibility */
static WC_INLINE void showPeer(WOLFSSL* ssl)
{
    showPeerEx(ssl, 0);
}

static WC_INLINE void build_addr(SOCKADDR_IN_T* addr, const char* peer,
                              word16 port, int udp, int sctp)
{
    int useLookup = 0;
    (void)useLookup;
    (void)udp;
    (void)sctp;

    if (addr == NULL)
        err_sys("invalid argument to build_addr, addr is NULL");

    XMEMSET(addr, 0, sizeof(SOCKADDR_IN_T));

#ifndef TEST_IPV6
    /* peer could be in human readable form */
    if ( ((size_t)peer != INADDR_ANY) && isalpha((int)peer[0])) {
    #ifdef WOLFSSL_USE_POPEN_HOST
        char host_ipaddr[4] = { 127, 0, 0, 1 };
        int found = 1;

        if ((XSTRNCMP(peer, "localhost", 10) != 0) &&
            (XSTRNCMP(peer, "127.0.0.1", 10) != 0)) {
            FILE* fp;
            char host_out[100];
            char cmd[100];

            XSTRNCPY(cmd, "host ", 6);
            XSTRNCAT(cmd, peer, 99 - XSTRLEN(cmd));
            found = 0;
            fp = popen(cmd, "r");
            if (fp != NULL) {
                while (fgets(host_out, sizeof(host_out), fp) != NULL) {
                    int i;
                    int j = 0;
                    for (j = 0; host_out[j] != '\0'; j++) {
                        if ((host_out[j] >= '0') && (host_out[j] <= '9')) {
                            break;
                        }
                    }
                    found = (host_out[j] >= '0') && (host_out[j] <= '9');
                    if (!found) {
                        continue;
                    }

                    for (i = 0; i < 4; i++) {
                        host_ipaddr[i] = atoi(host_out + j);
                        while ((host_out[j] >= '0') && (host_out[j] <= '9')) {
                            j++;
                        }
                        if (host_out[j] == '.') {
                            j++;
                            found &= (i != 3);
                        }
                        else {
                            found &= (i == 3);
                            break;
                        }
                    }
                    if (found) {
                        break;
                    }
                }
                pclose(fp);
            }
        }
        if (found) {
            XMEMCPY(&addr->sin_addr.s_addr, host_ipaddr, sizeof(host_ipaddr));
            useLookup = 1;
        }
    #elif !defined(WOLFSSL_USE_GETADDRINFO)
        #if defined(WOLFSSL_MDK_ARM) || defined(WOLFSSL_KEIL_TCP_NET)
            int err;
            struct hostent* entry = gethostbyname(peer, &err);
        #elif defined(WOLFSSL_TIRTOS)
            struct hostent* entry = DNSGetHostByName(peer);
        #elif defined(WOLFSSL_VXWORKS)
            struct hostent* entry = (struct hostent*)hostGetByName((char*)peer);
        #else
            struct hostent* entry = gethostbyname(peer);
        #endif

        if (entry) {
            XMEMCPY(&addr->sin_addr.s_addr, entry->h_addr_list[0],
                   entry->h_length);
            useLookup = 1;
        }
    #else
        struct zsock_addrinfo hints, *addrInfo;
        char portStr[6];
        XSNPRINTF(portStr, sizeof(portStr), "%d", port);
        XMEMSET(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = udp ? SOCK_DGRAM : SOCK_STREAM;
        hints.ai_protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;
        if (getaddrinfo((char*)peer, portStr, &hints, &addrInfo) == 0) {
            XMEMCPY(addr, addrInfo->ai_addr, sizeof(*addr));
            useLookup = 1;
        }
    #endif
        else
            err_sys("no entry for host");
    }
#endif


#ifndef TEST_IPV6
    #if defined(WOLFSSL_MDK_ARM) || defined(WOLFSSL_KEIL_TCP_NET)
        addr->sin_family = PF_INET;
    #else
        addr->sin_family = AF_INET_V;
    #endif
    addr->sin_port = XHTONS(port);
    if ((size_t)peer == INADDR_ANY)
        addr->sin_addr.s_addr = INADDR_ANY;
    else {
        if (!useLookup)
            addr->sin_addr.s_addr = inet_addr(peer);
    }
#else
    addr->sin6_family = AF_INET_V;
    addr->sin6_port = XHTONS(port);
    if ((size_t)peer == INADDR_ANY) {
        addr->sin6_addr = in6addr_any;
    }
    else {
        #if defined(HAVE_GETADDRINFO) || defined(WOLF_C99)
            struct addrinfo  hints;
            struct addrinfo* answer = NULL;
            int    ret;
            char   strPort[80];

            XMEMSET(&hints, 0, sizeof(hints));

            hints.ai_family   = AF_INET_V;
            if (udp) {
                hints.ai_socktype = SOCK_DGRAM;
                hints.ai_protocol = IPPROTO_UDP;
            }
        #ifdef WOLFSSL_SCTP
            else if (sctp) {
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_protocol = IPPROTO_SCTP;
            }
        #endif
            else {
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_protocol = IPPROTO_TCP;
            }

            SNPRINTF(strPort, sizeof(strPort), "%d", port);
            strPort[79] = '\0';

            ret = getaddrinfo(peer, strPort, &hints, &answer);
            if (ret < 0 || answer == NULL)
                err_sys("getaddrinfo failed");

            XMEMCPY(addr, answer->ai_addr, answer->ai_addrlen);
            freeaddrinfo(answer);
        #else
            printf("no ipv6 getaddrinfo, loopback only tests/examples\n");
            addr->sin6_addr = in6addr_loopback;
        #endif
    }
#endif
}


static WC_INLINE void tcp_socket(SOCKET_T* sockfd, int udp, int sctp)
{
    (void)sctp;

    if (udp)
        *sockfd = socket(AF_INET_V, SOCK_DGRAM, IPPROTO_UDP);
#ifdef WOLFSSL_SCTP
    else if (sctp)
        *sockfd = socket(AF_INET_V, SOCK_STREAM, IPPROTO_SCTP);
#endif
    else
        *sockfd = socket(AF_INET_V, SOCK_STREAM, IPPROTO_TCP);

    if(WOLFSSL_SOCKET_IS_INVALID(*sockfd)) {
        err_sys_with_errno("socket failed\n");
    }

#ifndef USE_WINDOWS_API
#ifdef SO_NOSIGPIPE
    {
        int       on = 1;
        socklen_t len = sizeof(on);
        int       res = setsockopt(*sockfd, SOL_SOCKET, SO_NOSIGPIPE, &on, len);
        if (res < 0)
            err_sys_with_errno("setsockopt SO_NOSIGPIPE failed\n");
    }
#elif defined(WOLFSSL_MDK_ARM) || defined (WOLFSSL_TIRTOS) ||\
                        defined(WOLFSSL_KEIL_TCP_NET) || defined(WOLFSSL_ZEPHYR)
    /* nothing to define */
#elif defined(NETOS)
    /* TODO: signal(SIGPIPE, SIG_IGN); */
#else  /* no S_NOSIGPIPE */
    signal(SIGPIPE, SIG_IGN);
#endif /* S_NOSIGPIPE */

#if defined(TCP_NODELAY)
    if (!udp && !sctp)
    {
        int       on = 1;
        socklen_t len = sizeof(on);
        int       res = setsockopt(*sockfd, IPPROTO_TCP, TCP_NODELAY, &on, len);
        if (res < 0)
            err_sys_with_errno("setsockopt TCP_NODELAY failed\n");
    }
#endif
#endif  /* USE_WINDOWS_API */
}

#if defined(WOLFSSL_WOLFSENTRY_HOOKS) && defined(WOLFSENTRY_H)

#include <wolfsentry/wolfsentry_util.h>

#if !defined(NO_FILESYSTEM) && !defined(WOLFSENTRY_NO_JSON)
#include <wolfsentry/wolfsentry_json.h>
#endif

struct wolfsentry_data {
    struct wolfsentry_sockaddr remote;
    byte remote_addrbuf[16];
    struct wolfsentry_sockaddr local;
    byte local_addrbuf[16];
    wolfsentry_route_flags_t flags;
    void *heap;
    int alloctype;
};

static void free_wolfsentry_data(struct wolfsentry_data *data) {
    XFREE(data, data->heap, data->alloctype);
}

static struct wolfsentry_context *wolfsentry = NULL;

static int wolfsentry_data_index = -1;

static WC_INLINE int wolfsentry_store_endpoints(
    WOLFSSL *ssl,
    SOCKADDR_IN_T *remote,
    SOCKADDR_IN_T *local,
    int proto,
    wolfsentry_route_flags_t flags,
    struct wolfsentry_data **wolfsentry_data_out)
{
    struct wolfsentry_data *wolfsentry_data = (struct wolfsentry_data *)XMALLOC(
        sizeof *wolfsentry_data, NULL, DYNAMIC_TYPE_SOCKADDR);
    if (wolfsentry_data == NULL)
        return WOLFSSL_FAILURE;

    wolfsentry_data->heap = NULL;
    wolfsentry_data->alloctype = DYNAMIC_TYPE_SOCKADDR;

#ifdef TEST_IPV6
    if ((sizeof wolfsentry_data->remote_addrbuf < sizeof remote->sin6_addr) ||
        (sizeof wolfsentry_data->local_addrbuf < sizeof local->sin6_addr))
        return WOLFSSL_FAILURE;
    wolfsentry_data->remote.sa_family = wolfsentry_data->local.sa_family = remote->sin6_family;
    wolfsentry_data->remote.sa_port = ntohs(remote->sin6_port);
    wolfsentry_data->local.sa_port = ntohs(local->sin6_port);
    if (WOLFSENTRY_MASKIN_BITS(flags, WOLFSENTRY_ROUTE_FLAG_SA_REMOTE_ADDR_WILDCARD)) {
        wolfsentry_data->remote.addr_len = 0;
        XMEMSET(wolfsentry_data->remote.addr, 0, sizeof remote->sin6_addr);
    } else {
        wolfsentry_data->remote.addr_len = sizeof remote->sin6_addr * BITS_PER_BYTE;
        XMEMCPY(wolfsentry_data->remote.addr, &remote->sin6_addr, sizeof remote->sin6_addr);
    }
    if (WOLFSENTRY_MASKIN_BITS(flags, WOLFSENTRY_ROUTE_FLAG_SA_LOCAL_ADDR_WILDCARD)) {
        wolfsentry_data->local.addr_len = 0;
        XMEMSET(wolfsentry_data->local.addr, 0, sizeof local->sin6_addr);
    } else {
        wolfsentry_data->local.addr_len = sizeof local->sin6_addr * BITS_PER_BYTE;
        XMEMCPY(wolfsentry_data->local.addr, &local->sin6_addr, sizeof local->sin6_addr);
    }
#else
    if ((sizeof wolfsentry_data->remote_addrbuf < sizeof remote->sin_addr) ||
        (sizeof wolfsentry_data->local_addrbuf < sizeof local->sin_addr))
        return WOLFSSL_FAILURE;
    wolfsentry_data->remote.sa_family = wolfsentry_data->local.sa_family = remote->sin_family;
    wolfsentry_data->remote.sa_port = ntohs(remote->sin_port);
    wolfsentry_data->local.sa_port = ntohs(local->sin_port);
    if (WOLFSENTRY_MASKIN_BITS(flags, WOLFSENTRY_ROUTE_FLAG_SA_REMOTE_ADDR_WILDCARD)) {
        wolfsentry_data->remote.addr_len = 0;
        XMEMSET(wolfsentry_data->remote.addr, 0, sizeof remote->sin_addr);
    } else {
        wolfsentry_data->remote.addr_len = sizeof remote->sin_addr * BITS_PER_BYTE;
        XMEMCPY(wolfsentry_data->remote.addr, &remote->sin_addr, sizeof remote->sin_addr);
    }
    if (WOLFSENTRY_MASKIN_BITS(flags, WOLFSENTRY_ROUTE_FLAG_SA_LOCAL_ADDR_WILDCARD)) {
        wolfsentry_data->local.addr_len = 0;
        XMEMSET(wolfsentry_data->local.addr, 0, sizeof local->sin_addr);
    } else {
        wolfsentry_data->local.addr_len = sizeof local->sin_addr * BITS_PER_BYTE;
        XMEMCPY(wolfsentry_data->local.addr, &local->sin_addr, sizeof local->sin_addr);
    }
#endif
    wolfsentry_data->remote.sa_proto = wolfsentry_data->local.sa_proto = proto;
    wolfsentry_data->remote.interface = wolfsentry_data->local.interface = 0;
    wolfsentry_data->flags = flags;

    if (wolfSSL_set_ex_data_with_cleanup(
            ssl, wolfsentry_data_index, wolfsentry_data,
            (wolfSSL_ex_data_cleanup_routine_t)free_wolfsentry_data) !=
        WOLFSSL_SUCCESS) {
        free_wolfsentry_data(wolfsentry_data);
        return WOLFSSL_FAILURE;
    }

    if (wolfsentry_data_out != NULL)
        *wolfsentry_data_out = wolfsentry_data;

    return WOLFSSL_SUCCESS;
}

static int wolfSentry_NetworkFilterCallback(
    WOLFSSL *ssl,
    struct wolfsentry_context *_wolfsentry,
    wolfSSL_netfilter_decision_t *decision)
{
    struct wolfsentry_data *data;
    char inet_ntop_buf[INET6_ADDRSTRLEN], inet_ntop_buf2[INET6_ADDRSTRLEN];
    wolfsentry_errcode_t ret;
    wolfsentry_action_res_t action_results;

    if ((data = wolfSSL_get_ex_data(ssl, wolfsentry_data_index)) == NULL)
        return WOLFSSL_FAILURE;

    ret = wolfsentry_route_event_dispatch(
        _wolfsentry,
        &data->remote,
        &data->local,
        data->flags,
        NULL /* event_label */,
        0 /* event_label_len */,
        NULL /* caller_context */,
        NULL /* id */,
        NULL /* inexact_matches */,
        &action_results);

    if (ret >= 0) {
        if (WOLFSENTRY_MASKIN_BITS(action_results, WOLFSENTRY_ACTION_RES_REJECT))
            *decision = WOLFSSL_NETFILTER_REJECT;
        else if (WOLFSENTRY_MASKIN_BITS(action_results, WOLFSENTRY_ACTION_RES_ACCEPT))
            *decision = WOLFSSL_NETFILTER_ACCEPT;
        else
            *decision = WOLFSSL_NETFILTER_PASS;
    } else {
        printf("wolfsentry_route_event_dispatch error "
               WOLFSENTRY_ERROR_FMT "\n", WOLFSENTRY_ERROR_FMT_ARGS(ret));
        *decision = WOLFSSL_NETFILTER_PASS;
    }

    printf("wolfSentry got network filter callback: family=%d proto=%d rport=%d"
           " lport=%d raddr=%s laddr=%s interface=%d; decision=%d (%s)\n",
           data->remote.sa_family,
           data->remote.sa_proto,
           data->remote.sa_port,
           data->local.sa_port,
           inet_ntop(data->remote.sa_family, data->remote.addr, inet_ntop_buf,
                     sizeof inet_ntop_buf),
           inet_ntop(data->local.sa_family, data->local.addr, inet_ntop_buf2,
                     sizeof inet_ntop_buf2),
           data->remote.interface,
           *decision,
           *decision == WOLFSSL_NETFILTER_REJECT ? "REJECT" :
           *decision == WOLFSSL_NETFILTER_ACCEPT ? "ACCEPT" :
           *decision == WOLFSSL_NETFILTER_PASS ? "PASS" :
           "???");

    return WOLFSSL_SUCCESS;
}

static int wolfsentry_setup(
    struct wolfsentry_context **_wolfsentry,
    const char *_wolfsentry_config_path,
    wolfsentry_route_flags_t route_flags)
{
    wolfsentry_errcode_t ret;
    ret =  wolfsentry_init(NULL /* hpi */, NULL /* default config */,
                           _wolfsentry);
    if (ret < 0) {
        fprintf(stderr, "wolfsentry_init() returned " WOLFSENTRY_ERROR_FMT "\n",
                WOLFSENTRY_ERROR_FMT_ARGS(ret));
        err_sys("unable to initialize wolfSentry");
    }

    if (wolfsentry_data_index < 0)
        wolfsentry_data_index = wolfSSL_get_ex_new_index(0, NULL, NULL, NULL,
                                                         NULL);

#if !defined(NO_FILESYSTEM) && !defined(WOLFSENTRY_NO_JSON)
    if (_wolfsentry_config_path != NULL) {
        char buf[512], err_buf[512];
        struct wolfsentry_json_process_state *jps;

        FILE *f = fopen(_wolfsentry_config_path, "r");

        if (f == NULL) {
            fprintf(stderr, "fopen(%s): %s\n",_wolfsentry_config_path,strerror(errno));
            err_sys("unable to open wolfSentry config file");
        }

        if ((ret = wolfsentry_config_json_init(
                 *_wolfsentry,
                 WOLFSENTRY_CONFIG_LOAD_FLAG_NONE,
                 &jps)) < 0) {
            fprintf(stderr, "wolfsentry_config_json_init() returned "
                    WOLFSENTRY_ERROR_FMT "\n",
                    WOLFSENTRY_ERROR_FMT_ARGS(ret));
            err_sys("error while initializing wolfSentry config parser");
        }

        for (;;) {
            size_t n = fread(buf, 1, sizeof buf, f);
            if ((n < sizeof buf) && ferror(f)) {
                fprintf(stderr,"fread(%s): %s\n",_wolfsentry_config_path, strerror(errno));
                err_sys("error while reading wolfSentry config file");
            }

            ret = wolfsentry_config_json_feed(jps, buf, n, err_buf, sizeof err_buf);
            if (ret < 0) {
                fprintf(stderr, "%.*s\n", (int)sizeof err_buf, err_buf);
                err_sys("error while loading wolfSentry config file");
            }
            if ((n < sizeof buf) && feof(f))
                break;
        }
        fclose(f);

        if ((ret = wolfsentry_config_json_fini(jps, err_buf, sizeof err_buf)) < 0) {
            fprintf(stderr, "%.*s\n", (int)sizeof err_buf, err_buf);
            err_sys("error while loading wolfSentry config file");
        }

    } else
#endif /* !NO_FILESYSTEM && !WOLFSENTRY_NO_JSON */
    {
        struct wolfsentry_route_table *table;

        if ((ret = wolfsentry_route_get_table_static(*_wolfsentry,
                                                                &table)) < 0)
            fprintf(stderr, "wolfsentry_route_get_table_static() returned "
                    WOLFSENTRY_ERROR_FMT "\n",
                    WOLFSENTRY_ERROR_FMT_ARGS(ret));

        if (ret < 0)
            return ret;

        if (WOLFSENTRY_MASKIN_BITS(route_flags, WOLFSENTRY_ROUTE_FLAG_DIRECTION_OUT)) {
            struct {
                struct wolfsentry_sockaddr sa;
                byte buf[16];
            } remote, local;
            wolfsentry_ent_id_t id;
            wolfsentry_action_res_t action_results;

            if ((ret = wolfsentry_route_table_default_policy_set(
                     *_wolfsentry, table,
                     WOLFSENTRY_ACTION_RES_ACCEPT))
                < 0) {
                fprintf(stderr,
                        "wolfsentry_route_table_default_policy_set() returned "
                        WOLFSENTRY_ERROR_FMT "\n",
                        WOLFSENTRY_ERROR_FMT_ARGS(ret));
                return ret;
            }

            XMEMSET(&remote, 0, sizeof remote);
            XMEMSET(&local, 0, sizeof local);
#ifdef TEST_IPV6
            remote.sa.sa_family = local.sa.sa_family = AF_INET6;
            remote.sa.addr_len = 128;
            XMEMCPY(remote.sa.addr, "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\001", 16);
#else
            remote.sa.sa_family = local.sa.sa_family = AF_INET;
            remote.sa.addr_len = 32;
            XMEMCPY(remote.sa.addr, "\177\000\000\001", 4);
#endif

            if ((ret = wolfsentry_route_insert_static
                 (*_wolfsentry, NULL /* caller_context */, &remote.sa, &local.sa,
                  route_flags                                    |
                  WOLFSENTRY_ROUTE_FLAG_GREENLISTED              |
                  WOLFSENTRY_ROUTE_FLAG_PARENT_EVENT_WILDCARD    |
                  WOLFSENTRY_ROUTE_FLAG_REMOTE_INTERFACE_WILDCARD|
                  WOLFSENTRY_ROUTE_FLAG_LOCAL_INTERFACE_WILDCARD |
                  WOLFSENTRY_ROUTE_FLAG_SA_LOCAL_ADDR_WILDCARD   |
                  WOLFSENTRY_ROUTE_FLAG_SA_PROTO_WILDCARD        |
                  WOLFSENTRY_ROUTE_FLAG_SA_REMOTE_PORT_WILDCARD  |
                  WOLFSENTRY_ROUTE_FLAG_SA_LOCAL_PORT_WILDCARD,
                  0 /* event_label_len */, 0 /* event_label */, &id,
                  &action_results)) < 0) {
                fprintf(stderr, "wolfsentry_route_insert_static() returned "
                        WOLFSENTRY_ERROR_FMT "\n",
                        WOLFSENTRY_ERROR_FMT_ARGS(ret));
                return ret;
            }
        } else if (WOLFSENTRY_MASKIN_BITS(route_flags, WOLFSENTRY_ROUTE_FLAG_DIRECTION_IN)) {
            struct {
                struct wolfsentry_sockaddr sa;
                byte buf[16];
            } remote, local;
            wolfsentry_ent_id_t id;
            wolfsentry_action_res_t action_results;

            if ((ret = wolfsentry_route_table_default_policy_set(
                     *_wolfsentry, table,
                     WOLFSENTRY_ACTION_RES_REJECT|WOLFSENTRY_ACTION_RES_STOP))
                < 0) {
                fprintf(stderr,
                        "wolfsentry_route_table_default_policy_set() returned "
                        WOLFSENTRY_ERROR_FMT "\n",
                        WOLFSENTRY_ERROR_FMT_ARGS(ret));
                return ret;
            }

            XMEMSET(&remote, 0, sizeof remote);
            XMEMSET(&local, 0, sizeof local);
#ifdef TEST_IPV6
            remote.sa.sa_family = local.sa.sa_family = AF_INET6;
            remote.sa.addr_len = 128;
            XMEMCPY(remote.sa.addr, "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\001", 16);
#else
            remote.sa.sa_family = local.sa.sa_family = AF_INET;
            remote.sa.addr_len = 32;
            XMEMCPY(remote.sa.addr, "\177\000\000\001", 4);
#endif

            if ((ret = wolfsentry_route_insert_static
                 (*_wolfsentry, NULL /* caller_context */, &remote.sa, &local.sa,
                  route_flags                                    |
                  WOLFSENTRY_ROUTE_FLAG_GREENLISTED              |
                  WOLFSENTRY_ROUTE_FLAG_PARENT_EVENT_WILDCARD    |
                  WOLFSENTRY_ROUTE_FLAG_REMOTE_INTERFACE_WILDCARD|
                  WOLFSENTRY_ROUTE_FLAG_LOCAL_INTERFACE_WILDCARD |
                  WOLFSENTRY_ROUTE_FLAG_SA_LOCAL_ADDR_WILDCARD   |
                  WOLFSENTRY_ROUTE_FLAG_SA_PROTO_WILDCARD        |
                  WOLFSENTRY_ROUTE_FLAG_SA_REMOTE_PORT_WILDCARD  |
                  WOLFSENTRY_ROUTE_FLAG_SA_LOCAL_PORT_WILDCARD,
                  0 /* event_label_len */, 0 /* event_label */, &id,
                  &action_results)) < 0) {
                fprintf(stderr, "wolfsentry_route_insert_static() returned "
                        WOLFSENTRY_ERROR_FMT "\n",
                        WOLFSENTRY_ERROR_FMT_ARGS(ret));
                return ret;
            }
        }
    }

    return 0;
}

static WC_INLINE int tcp_connect_with_wolfSentry(
    SOCKET_T* sockfd,
    const char* ip,
    word16 port,
    int udp,
    int sctp,
    WOLFSSL* ssl,
    struct wolfsentry_context *_wolfsentry)
{
    SOCKADDR_IN_T remote_addr;
    struct wolfsentry_data *wolfsentry_data;
    char inet_ntop_buf[INET6_ADDRSTRLEN], inet_ntop_buf2[INET6_ADDRSTRLEN];
    wolfsentry_errcode_t ret;
    wolfsentry_action_res_t action_results;
    wolfSSL_netfilter_decision_t decision;

    build_addr(&remote_addr, ip, port, udp, sctp);

    {
        SOCKADDR_IN_T local_addr;
#ifdef TEST_IPV6
        local_addr.sin6_port = 0;
#else
        local_addr.sin_port = 0;
#endif
        ((struct sockaddr *)&local_addr)->sa_family = ((struct sockaddr *)&remote_addr)->sa_family;

        if (wolfsentry_store_endpoints(
                ssl, &remote_addr, &local_addr,
                udp ? IPPROTO_UDP : IPPROTO_TCP,
                WOLFSENTRY_ROUTE_FLAG_DIRECTION_OUT|
                WOLFSENTRY_ROUTE_FLAG_SA_LOCAL_ADDR_WILDCARD|
                WOLFSENTRY_ROUTE_FLAG_SA_LOCAL_PORT_WILDCARD, &wolfsentry_data) != WOLFSSL_SUCCESS)
            return WOLFSSL_FAILURE;
    }

    ret = wolfsentry_route_event_dispatch(
        _wolfsentry,
        &wolfsentry_data->remote,
        &wolfsentry_data->local,
        wolfsentry_data->flags,
        NULL /* event_label */,
        0    /* event_label_len */,
        NULL /* caller_context */,
        NULL /* id */,
        NULL /* inexact_matches */,
        &action_results);

    if (ret < 0) {
        printf("wolfsentry_route_event_dispatch error "
               WOLFSENTRY_ERROR_FMT "\n", WOLFSENTRY_ERROR_FMT_ARGS(ret));
        decision = WOLFSSL_NETFILTER_PASS;
    } else {
        if (WOLFSENTRY_MASKIN_BITS(action_results, WOLFSENTRY_ACTION_RES_REJECT))
            decision = WOLFSSL_NETFILTER_REJECT;
        else if (WOLFSENTRY_MASKIN_BITS(action_results, WOLFSENTRY_ACTION_RES_ACCEPT))
            decision = WOLFSSL_NETFILTER_ACCEPT;
        else
            decision = WOLFSSL_NETFILTER_PASS;
    }

    printf("wolfSentry callin from tcp_connect_with_wolfSentry: family=%d proto=%d rport=%d"
           " lport=%d raddr=%s laddr=%s interface=%d; decision=%d (%s)\n",
           wolfsentry_data->remote.sa_family,
           wolfsentry_data->remote.sa_proto,
           wolfsentry_data->remote.sa_port,
           wolfsentry_data->local.sa_port,
           inet_ntop(wolfsentry_data->remote.sa_family, wolfsentry_data->remote.addr, inet_ntop_buf,
                     sizeof inet_ntop_buf),
           inet_ntop(wolfsentry_data->local.sa_family, wolfsentry_data->local.addr, inet_ntop_buf2,
                     sizeof inet_ntop_buf2),
           wolfsentry_data->remote.interface,
           decision,
           decision == WOLFSSL_NETFILTER_REJECT ? "REJECT" :
           decision == WOLFSSL_NETFILTER_ACCEPT ? "ACCEPT" :
           decision == WOLFSSL_NETFILTER_PASS ?   "PASS" :
           "???");

    if (decision == WOLFSSL_NETFILTER_REJECT)
        return SOCKET_FILTERED_E;

    if (udp) {
        wolfSSL_dtls_set_peer(ssl, &remote_addr, sizeof(remote_addr));
    }
    tcp_socket(sockfd, udp, sctp);

    if (!udp) {
        if (connect(*sockfd, (const struct sockaddr*)&remote_addr, sizeof(remote_addr)) != 0)
            err_sys_with_errno("tcp connect failed");
    }

    return WOLFSSL_SUCCESS;
}

#define tcp_connect(sockfd, ip, port, udp, sctp, ssl) \
    tcp_connect_with_wolfSentry(sockfd, ip, port, udp, sctp, ssl, wolfsentry)

#else /* !WOLFSSL_WOLFSENTRY_HOOKS */

static WC_INLINE void tcp_connect(SOCKET_T* sockfd, const char* ip, word16 port,
                               int udp, int sctp, WOLFSSL* ssl)
{
    SOCKADDR_IN_T addr;
    build_addr(&addr, ip, port, udp, sctp);
    if (udp) {
        wolfSSL_dtls_set_peer(ssl, &addr, sizeof(addr));
    }
    tcp_socket(sockfd, udp, sctp);

    if (!udp) {
        if (connect(*sockfd, (const struct sockaddr*)&addr, sizeof(addr)) != 0)
            err_sys_with_errno("tcp connect failed");
    }
}

#endif /* WOLFSSL_WOLFSENTRY_HOOKS */


static WC_INLINE void udp_connect(SOCKET_T* sockfd, void* addr, int addrSz)
{
    if (connect(*sockfd, (const struct sockaddr*)addr, addrSz) != 0)
        err_sys_with_errno("tcp connect failed");
}


enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_SEND_READY,
    TEST_ERROR_READY
};


#if !defined(WOLFSSL_MDK_ARM) && !defined(WOLFSSL_KEIL_TCP_NET) && \
                                 !defined(WOLFSSL_TIRTOS)
static WC_INLINE int tcp_select_ex(SOCKET_T socketfd, int to_sec, int rx)
{
    fd_set fds, errfds;
    fd_set* recvfds = NULL;
    fd_set* sendfds = NULL;
    SOCKET_T nfds = socketfd + 1;
#if !defined(__INTEGRITY)
    struct timeval timeout = {(to_sec > 0) ? to_sec : 0, 0};
#else
    struct timeval timeout;
#endif
    int result;

    FD_ZERO(&fds);
    FD_SET(socketfd, &fds);
    FD_ZERO(&errfds);
    FD_SET(socketfd, &errfds);

    if (rx)
        recvfds = &fds;
    else
        sendfds = &fds;

#if defined(__INTEGRITY)
    timeout.tv_sec = (long long)(to_sec > 0) ? to_sec : 0, 0;
#endif
    result = select(nfds, recvfds, sendfds, &errfds, &timeout);

    if (result == 0)
        return TEST_TIMEOUT;
    else if (result > 0) {
        if (FD_ISSET(socketfd, &fds)) {
            if (rx)
                return TEST_RECV_READY;
            else
                return TEST_SEND_READY;
        }
        else if(FD_ISSET(socketfd, &errfds))
            return TEST_ERROR_READY;
    }

    return TEST_SELECT_FAIL;
}

static WC_INLINE int tcp_select(SOCKET_T socketfd, int to_sec)
{
    return tcp_select_ex(socketfd, to_sec, 1);
}

static WC_INLINE int tcp_select_tx(SOCKET_T socketfd, int to_sec)
{
    return tcp_select_ex(socketfd, to_sec, 0);
}

#elif defined(WOLFSSL_TIRTOS) || defined(WOLFSSL_KEIL_TCP_NET)
static WC_INLINE int tcp_select(SOCKET_T socketfd, int to_sec)
{
    return TEST_RECV_READY;
}
static WC_INLINE int tcp_select_tx(SOCKET_T socketfd, int to_sec)
{
    return TEST_SEND_READY;
}
#endif /* !WOLFSSL_MDK_ARM */


static WC_INLINE void tcp_listen(SOCKET_T* sockfd, word16* port, int useAnyAddr,
                              int udp, int sctp)
{
    SOCKADDR_IN_T addr;

    /* don't use INADDR_ANY by default, firewall may block, make user switch
       on */
    build_addr(&addr, (useAnyAddr ? INADDR_ANY : wolfSSLIP), *port, udp, sctp);
    tcp_socket(sockfd, udp, sctp);

#if !defined(USE_WINDOWS_API) && !defined(WOLFSSL_MDK_ARM)\
                   && !defined(WOLFSSL_KEIL_TCP_NET) && !defined(WOLFSSL_ZEPHYR)
    {
        int       res, on  = 1;
        socklen_t len = sizeof(on);
        res = setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
        if (res < 0)
            err_sys_with_errno("setsockopt SO_REUSEADDR failed\n");
    }
#ifdef SO_REUSEPORT
    {
        int       res, on  = 1;
        socklen_t len = sizeof(on);
        res = setsockopt(*sockfd, SOL_SOCKET, SO_REUSEPORT, &on, len);
        if (res < 0)
            err_sys_with_errno("setsockopt SO_REUSEPORT failed\n");
    }
#endif
#endif

    if (bind(*sockfd, (const struct sockaddr*)&addr, sizeof(addr)) != 0)
        err_sys_with_errno("tcp bind failed");
    if (!udp) {
        #ifdef WOLFSSL_KEIL_TCP_NET
            #define SOCK_LISTEN_MAX_QUEUE 1
        #else
            #define SOCK_LISTEN_MAX_QUEUE 5
        #endif
        if (listen(*sockfd, SOCK_LISTEN_MAX_QUEUE) != 0)
                err_sys_with_errno("tcp listen failed");
    }
    #if !defined(USE_WINDOWS_API) && !defined(WOLFSSL_TIRTOS) \
                                                     && !defined(WOLFSSL_ZEPHYR)
        if (*port == 0) {
            socklen_t len = sizeof(addr);
            if (getsockname(*sockfd, (struct sockaddr*)&addr, &len) == 0) {
                #ifndef TEST_IPV6
                    *port = XNTOHS(addr.sin_port);
                #else
                    *port = XNTOHS(addr.sin6_port);
                #endif
            }
        }
    #endif
}


#if 0
static WC_INLINE int udp_read_connect(SOCKET_T sockfd)
{
    SOCKADDR_IN_T cliaddr;
    byte          b[1500];
    int           n;
    socklen_t     len = sizeof(cliaddr);

    n = (int)recvfrom(sockfd, (char*)b, sizeof(b), MSG_PEEK,
                      (struct sockaddr*)&cliaddr, &len);
    if (n > 0) {
        if (connect(sockfd, (const struct sockaddr*)&cliaddr,
                    sizeof(cliaddr)) != 0)
            err_sys("udp connect failed");
    }
    else
        err_sys("recvfrom failed");

    return sockfd;
}
#endif

static WC_INLINE void udp_accept(SOCKET_T* sockfd, SOCKET_T* clientfd,
                              int useAnyAddr, word16 port, func_args* args)
{
    SOCKADDR_IN_T addr;

    (void)args;
    build_addr(&addr, (useAnyAddr ? INADDR_ANY : wolfSSLIP), port, 1, 0);
    tcp_socket(sockfd, 1, 0);


#if !defined(USE_WINDOWS_API) && !defined(WOLFSSL_MDK_ARM) \
                   && !defined(WOLFSSL_KEIL_TCP_NET) && !defined(WOLFSSL_ZEPHYR)
    {
        int       res, on  = 1;
        socklen_t len = sizeof(on);
        res = setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
        if (res < 0)
            err_sys_with_errno("setsockopt SO_REUSEADDR failed\n");
    }
#ifdef SO_REUSEPORT
    {
        int       res, on  = 1;
        socklen_t len = sizeof(on);
        res = setsockopt(*sockfd, SOL_SOCKET, SO_REUSEPORT, &on, len);
        if (res < 0)
            err_sys_with_errno("setsockopt SO_REUSEPORT failed\n");
    }
#endif
#endif

    if (bind(*sockfd, (const struct sockaddr*)&addr, sizeof(addr)) != 0)
        err_sys_with_errno("tcp bind failed");

    #if (defined(NO_MAIN_DRIVER) && !defined(USE_WINDOWS_API)) && !defined(WOLFSSL_TIRTOS)
        if (port == 0) {
            socklen_t len = sizeof(addr);
            if (getsockname(*sockfd, (struct sockaddr*)&addr, &len) == 0) {
                #ifndef TEST_IPV6
                    port = XNTOHS(addr.sin_port);
                #else
                    port = XNTOHS(addr.sin6_port);
                #endif
            }
        }
    #endif

#if defined(_POSIX_THREADS) && defined(NO_MAIN_DRIVER) && !defined(__MINGW32__)
    /* signal ready to accept data */
    {
    tcp_ready* ready = args->signal;
    pthread_mutex_lock(&ready->mutex);
    ready->ready = 1;
    ready->port = port;
    pthread_cond_signal(&ready->cond);
    pthread_mutex_unlock(&ready->mutex);
    }
#elif defined (WOLFSSL_TIRTOS)
    /* Need mutex? */
    tcp_ready* ready = args->signal;
    ready->ready = 1;
    ready->port = port;
#elif defined(NETOS)
    {
        tcp_ready* ready = args->signal;
        (void)tx_mutex_get(&ready->mutex, TX_WAIT_FOREVER);
        ready->ready = 1;
        ready->port = port;
        (void)tx_mutex_put(&ready->mutex);
    }
#else
    (void)port;
#endif

    *clientfd = *sockfd;
}

static WC_INLINE void tcp_accept(SOCKET_T* sockfd, SOCKET_T* clientfd,
                              func_args* args, word16 port, int useAnyAddr,
                              int udp, int sctp, int ready_file, int do_listen,
                              SOCKADDR_IN_T *client_addr, socklen_t *client_len)
{
    tcp_ready* ready = NULL;

    (void) ready; /* Account for case when "ready" is not used */

    if (udp) {
        udp_accept(sockfd, clientfd, useAnyAddr, port, args);
        return;
    }

    if(do_listen) {
        tcp_listen(sockfd, &port, useAnyAddr, udp, sctp);

    #if defined(_POSIX_THREADS) && defined(NO_MAIN_DRIVER) && !defined(__MINGW32__)
        /* signal ready to tcp_accept */
        if (args)
            ready = args->signal;
        if (ready) {
            pthread_mutex_lock(&ready->mutex);
            ready->ready = 1;
            ready->port = port;
            pthread_cond_signal(&ready->cond);
            pthread_mutex_unlock(&ready->mutex);
        }
    #elif defined (WOLFSSL_TIRTOS)
        /* Need mutex? */
        if (args)
            ready = args->signal;
        if (ready) {
            ready->ready = 1;
            ready->port = port;
        }
    #elif defined(NETOS)
        /* signal ready to tcp_accept */
        if (args)
            ready = args->signal;
        if (ready) {
            (void)tx_mutex_get(&ready->mutex, TX_WAIT_FOREVER);
            ready->ready = 1;
            ready->port = port;
            (void)tx_mutex_put(&ready->mutex);
        }
    #endif

        if (ready_file) {
        #if !defined(NO_FILESYSTEM) || defined(FORCE_BUFFER_TEST) && \
            !defined(NETOS)
            XFILE srf = NULL;
            if (args)
                ready = args->signal;

            if (ready) {
                srf = XFOPEN(ready->srfName, "w");

                if (srf) {
                    /* let's write port sever is listening on to ready file
                       external monitor can then do ephemeral ports by passing
                       -p 0 to server on supported platforms with -R ready_file
                       client can then wait for existence of ready_file and see
                       which port the server is listening on. */
                    fprintf(srf, "%d\n", (int)port);
                    fclose(srf);
                }
            }
        #endif
        }
    }

    *clientfd = accept(*sockfd, (struct sockaddr*)client_addr,
                      (ACCEPT_THIRD_T)client_len);
    if(WOLFSSL_SOCKET_IS_INVALID(*clientfd)) {
        err_sys_with_errno("tcp accept failed");
    }
}


static WC_INLINE void tcp_set_nonblocking(SOCKET_T* sockfd)
{
    #ifdef USE_WINDOWS_API
        unsigned long blocking = 1;
        int ret = ioctlsocket(*sockfd, FIONBIO, &blocking);
        if (ret == SOCKET_ERROR)
            err_sys_with_errno("ioctlsocket failed");
    #elif defined(WOLFSSL_MDK_ARM) || defined(WOLFSSL_KEIL_TCP_NET) \
        || defined (WOLFSSL_TIRTOS)|| defined(WOLFSSL_VXWORKS) \
        || defined(WOLFSSL_ZEPHYR)
         /* non blocking not supported, for now */
    #else
        int flags = fcntl(*sockfd, F_GETFL, 0);
        if (flags < 0)
            err_sys_with_errno("fcntl get failed");
        flags = fcntl(*sockfd, F_SETFL, flags | O_NONBLOCK);
        if (flags < 0)
            err_sys_with_errno("fcntl set failed");
    #endif
}


#ifndef NO_PSK

/* identity is OpenSSL testing default for openssl s_client, keep same */
static const char* kIdentityStr = "Client_identity";

static WC_INLINE unsigned int my_psk_client_cb(WOLFSSL* ssl, const char* hint,
        char* identity, unsigned int id_max_len, unsigned char* key,
        unsigned int key_max_len)
{
    (void)ssl;
    (void)hint;
    (void)key_max_len;

    /* see internal.h MAX_PSK_ID_LEN for PSK identity limit */
    XSTRNCPY(identity, kIdentityStr, id_max_len);

    if (wolfSSL_GetVersion(ssl) < WOLFSSL_TLSV1_3) {
        /* test key in hex is 0x1a2b3c4d , in decimal 439,041,101 , we're using
           unsigned binary */
        key[0] = 0x1a;
        key[1] = 0x2b;
        key[2] = 0x3c;
        key[3] = 0x4d;

        return 4;   /* length of key in octets or 0 for error */
    }
    else {
        int i;
        int b = 0x01;

        for (i = 0; i < 32; i++, b += 0x22) {
            if (b >= 0x100)
                b = 0x01;
            key[i] = b;
        }

        return 32;   /* length of key in octets or 0 for error */
    }
}


static WC_INLINE unsigned int my_psk_server_cb(WOLFSSL* ssl, const char* identity,
        unsigned char* key, unsigned int key_max_len)
{
    (void)ssl;
    (void)key_max_len;

    /* see internal.h MAX_PSK_ID_LEN for PSK identity limit */
    if (XSTRNCMP(identity, kIdentityStr, XSTRLEN(kIdentityStr)) != 0)
        return 0;

    if (wolfSSL_GetVersion(ssl) < WOLFSSL_TLSV1_3) {
        /* test key in hex is 0x1a2b3c4d , in decimal 439,041,101 , we're using
           unsigned binary */
        key[0] = 0x1a;
        key[1] = 0x2b;
        key[2] = 0x3c;
        key[3] = 0x4d;

        return 4;   /* length of key in octets or 0 for error */
    }
    else {
        int i;
        int b = 0x01;

        for (i = 0; i < 32; i++, b += 0x22) {
            if (b >= 0x100)
                b = 0x01;
            key[i] = b;
        }

        return 32;   /* length of key in octets or 0 for error */
    }
}

#ifdef WOLFSSL_TLS13
static WC_INLINE unsigned int my_psk_client_tls13_cb(WOLFSSL* ssl,
        const char* hint, char* identity, unsigned int id_max_len,
        unsigned char* key, unsigned int key_max_len, const char** ciphersuite)
{
    int i;
    int b = 0x01;
    const char* userCipher = (const char*)wolfSSL_get_psk_callback_ctx(ssl);

    (void)ssl;
    (void)hint;
    (void)key_max_len;

    /* see internal.h MAX_PSK_ID_LEN for PSK identity limit */
    XSTRNCPY(identity, kIdentityStr, id_max_len);

    for (i = 0; i < 32; i++, b += 0x22) {
        if (b >= 0x100)
            b = 0x01;
        key[i] = b;
    }

    *ciphersuite = userCipher ? userCipher : "TLS13-AES128-GCM-SHA256";

    return 32;   /* length of key in octets or 0 for error */
}


static WC_INLINE unsigned int my_psk_server_tls13_cb(WOLFSSL* ssl,
        const char* identity, unsigned char* key, unsigned int key_max_len,
        const char** ciphersuite)
{
    int i;
    int b = 0x01;
    int kIdLen = (int)XSTRLEN(kIdentityStr);
    const char* userCipher = (const char*)wolfSSL_get_psk_callback_ctx(ssl);

    (void)ssl;
    (void)key_max_len;

    /* see internal.h MAX_PSK_ID_LEN for PSK identity limit */
    if (XSTRNCMP(identity, kIdentityStr, kIdLen) != 0)
        return 0;
    if (identity[kIdLen] != '\0') {
        userCipher = wolfSSL_get_cipher_name_by_hash(ssl, identity + kIdLen);
    }

    for (i = 0; i < 32; i++, b += 0x22) {
        if (b >= 0x100)
            b = 0x01;
        key[i] = b;
    }

    *ciphersuite = userCipher ? userCipher : "TLS13-AES128-GCM-SHA256";

    return 32;   /* length of key in octets or 0 for error */
}
#endif

#if defined(OPENSSL_ALL) && !defined(NO_CERTS) && \
       !defined(NO_FILESYSTEM)
static unsigned char local_psk[32];
#endif
static WC_INLINE int my_psk_use_session_cb(WOLFSSL* ssl, 
            const WOLFSSL_EVP_MD* md, const unsigned char **id,
            size_t* idlen,  WOLFSSL_SESSION **sess)
{
#if defined(OPENSSL_ALL) && !defined(NO_CERTS) && \
       !defined(NO_FILESYSTEM)
    int i;
    int b = 0x01;
    WOLFSSL_SESSION* lsess;
    char buf[256];
    const char* cipher_id = "TLS13-AES128-GCM-SHA256";
    const SSL_CIPHER* cipher = NULL;
    STACK_OF(SSL_CIPHER) *supportedCiphers = NULL;
    int numCiphers = 0;
    (void)ssl;
    (void)md;
    
    printf("use psk session callback \n");
    
    lsess = wolfSSL_SESSION_new();
    if (lsess == NULL) {
        return 0;
    }
    supportedCiphers = SSL_get_ciphers(ssl);
    numCiphers = sk_num(supportedCiphers);

    for (i = 0; i < numCiphers; ++i) {
        
        if ((cipher = (const WOLFSSL_CIPHER*)sk_value(supportedCiphers, i))) {
            SSL_CIPHER_description(cipher, buf, sizeof(buf));
        }
        
        if (XMEMCMP(cipher_id, buf, XSTRLEN(cipher_id)) == 0) {
            break;
        }
    }
    
    if (i != numCiphers) {
        SSL_SESSION_set_cipher(lsess, cipher);
            for (i = 0; i < 32; i++, b += 0x22) {
            if (b >= 0x100)
                b = 0x01;
            local_psk[i] = b;
        }
        
        *id = local_psk;
        *idlen = 32;
        *sess = lsess;
        
        return 1;
    }
    else {
        *id = NULL;
        *idlen = 0;
        *sess = NULL;
        return 0;
    }
#else
    (void)ssl;
    (void)md;
    (void)id;
    (void)idlen;
    (void)sess;
    
    return 0;
#endif
}

static WC_INLINE unsigned int my_psk_client_cs_cb(WOLFSSL* ssl,
        const char* hint, char* identity, unsigned int id_max_len,
        unsigned char* key, unsigned int key_max_len, const char* ciphersuite)
{
    int i;
    int b = 0x01;

    (void)ssl;
    (void)hint;
    (void)key_max_len;

    /* see internal.h MAX_PSK_ID_LEN for PSK identity limit */
    XSTRNCPY(identity, kIdentityStr, id_max_len);
    XSTRNCAT(identity, ciphersuite + XSTRLEN(ciphersuite) - 6, id_max_len);

    for (i = 0; i < 32; i++, b += 0x22) {
        if (b >= 0x100)
            b = 0x01;
        key[i] = b;
    }

    return 32;   /* length of key in octets or 0 for error */
}

#endif /* !NO_PSK */


#if defined(WOLFSSL_USER_CURRTIME)
    extern   double current_time(int reset);

#elif defined(USE_WINDOWS_API)

    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    static WC_INLINE double current_time(int reset)
    {
        static int init = 0;
        static LARGE_INTEGER freq;

        LARGE_INTEGER count;

        if (!init) {
            QueryPerformanceFrequency(&freq);
            init = 1;
        }

        QueryPerformanceCounter(&count);

        (void)reset;
        return (double)count.QuadPart / freq.QuadPart;
    }

#elif defined(WOLFSSL_TIRTOS)
    extern double current_time();
#elif defined(WOLFSSL_ZEPHYR)
    extern double current_time();
#else

#if !defined(WOLFSSL_MDK_ARM) && !defined(WOLFSSL_KEIL_TCP_NET) && !defined(WOLFSSL_CHIBIOS)
    #ifndef NETOS
        #include <sys/time.h>
    #endif

    static WC_INLINE double current_time(int reset)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        (void)reset;

        return (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
    }
#else
    extern double current_time(int reset);
#endif
#endif /* USE_WINDOWS_API */


#if defined(HAVE_OCSP) && defined(WOLFSSL_NONBLOCK_OCSP)
static WC_INLINE int OCSPIOCb(void* ioCtx, const char* url, int urlSz,
    unsigned char* request, int requestSz, unsigned char** response)
{
#ifdef TEST_NONBLOCK_CERTS
    static int ioCbCnt = 0;
#endif

    (void)ioCtx;
    (void)url;
    (void)urlSz;
    (void)request;
    (void)requestSz;
    (void)response;

#ifdef TEST_NONBLOCK_CERTS
    if (ioCbCnt) {
        ioCbCnt = 0;
        return EmbedOcspLookup(ioCtx, url, urlSz, request, requestSz, response);
    }
    else {
        ioCbCnt = 1;
        return WOLFSSL_CBIO_ERR_WANT_READ;
    }
#else
    return EmbedOcspLookup(ioCtx, url, urlSz, request, requestSz, response);
#endif
}

static WC_INLINE void OCSPRespFreeCb(void* ioCtx, unsigned char* response)
{
    return EmbedOcspRespFree(ioCtx, response);
}
#endif

#if !defined(NO_CERTS)
    #if !defined(NO_FILESYSTEM) || \
        (defined(NO_FILESYSTEM) && defined(FORCE_BUFFER_TEST)) && \
        !defined(NETOS)

    /* reads file size, allocates buffer, reads into buffer, returns buffer */
    static WC_INLINE int load_file(const char* fname, byte** buf, size_t* bufLen)
    {
        int ret;
        long int fileSz;
        XFILE lFile;

        if (fname == NULL || buf == NULL || bufLen == NULL)
            return BAD_FUNC_ARG;

        /* set defaults */
        *buf = NULL;
        *bufLen = 0;

        /* open file (read-only binary) */
        lFile = XFOPEN(fname, "rb");
        if (!lFile) {
            printf("Error loading %s\n", fname);
            return BAD_PATH_ERROR;
        }

        fseek(lFile, 0, SEEK_END);
        fileSz = (int)ftell(lFile);
        rewind(lFile);
        if (fileSz  > 0) {
            *bufLen = (size_t)fileSz;
            *buf = (byte*)malloc(*bufLen);
            if (*buf == NULL) {
                ret = MEMORY_E;
                printf("Error allocating %lu bytes\n", (unsigned long)*bufLen);
            }
            else {
                size_t readLen = fread(*buf, *bufLen, 1, lFile);

                /* check response code */
                ret = (readLen > 0) ? 0 : -1;
            }
        }
        else {
            ret = BUFFER_E;
        }
        fclose(lFile);

        return ret;
    }

    enum {
        WOLFSSL_CA   = 1,
        WOLFSSL_CERT = 2,
        WOLFSSL_KEY  = 3,
        WOLFSSL_CERT_CHAIN = 4,
    };

    static WC_INLINE void load_buffer(WOLFSSL_CTX* ctx, const char* fname, int type)
    {
        int format = WOLFSSL_FILETYPE_PEM;
        byte* buff = NULL;
        size_t sz = 0;

        if (load_file(fname, &buff, &sz) != 0) {
            err_sys("can't open file for buffer load "
                    "Please run from wolfSSL home directory if not");
        }

        /* determine format */
        if (strstr(fname, ".der"))
            format = WOLFSSL_FILETYPE_ASN1;

        if (type == WOLFSSL_CA) {
            if (wolfSSL_CTX_load_verify_buffer(ctx, buff, (long)sz, format)
                                              != WOLFSSL_SUCCESS)
                err_sys("can't load buffer ca file");
        }
        else if (type == WOLFSSL_CERT) {
            if (wolfSSL_CTX_use_certificate_buffer(ctx, buff, (long)sz,
                        format) != WOLFSSL_SUCCESS)
                err_sys("can't load buffer cert file");
        }
        else if (type == WOLFSSL_KEY) {
            if (wolfSSL_CTX_use_PrivateKey_buffer(ctx, buff, (long)sz,
                        format) != WOLFSSL_SUCCESS)
                err_sys("can't load buffer key file");
        }
        else if (type == WOLFSSL_CERT_CHAIN) {
            if (wolfSSL_CTX_use_certificate_chain_buffer_format(ctx, buff,
                    (long)sz, format) != WOLFSSL_SUCCESS)
                err_sys("can't load cert chain buffer");
        }

        if (buff)
            free(buff);
    }

    static WC_INLINE void load_ssl_buffer(WOLFSSL* ssl, const char* fname, int type)
    {
        int format = WOLFSSL_FILETYPE_PEM;
        byte* buff = NULL;
        size_t sz = 0;

        if (load_file(fname, &buff, &sz) != 0) {
            err_sys("can't open file for buffer load "
                    "Please run from wolfSSL home directory if not");
        }

        /* determine format */
        if (strstr(fname, ".der"))
            format = WOLFSSL_FILETYPE_ASN1;

        if (type == WOLFSSL_CA) {
            /* verify certs (CA's) use the shared ctx->cm (WOLFSSL_CERT_MANAGER) */
            WOLFSSL_CTX* ctx = wolfSSL_get_SSL_CTX(ssl);
            if (wolfSSL_CTX_load_verify_buffer(ctx, buff, (long)sz, format)
                                              != WOLFSSL_SUCCESS)
                err_sys("can't load buffer ca file");
        }
        else if (type == WOLFSSL_CERT) {
            if (wolfSSL_use_certificate_buffer(ssl, buff, (long)sz,
                        format) != WOLFSSL_SUCCESS)
                err_sys("can't load buffer cert file");
        }
        else if (type == WOLFSSL_KEY) {
            if (wolfSSL_use_PrivateKey_buffer(ssl, buff, (long)sz,
                        format) != WOLFSSL_SUCCESS)
                err_sys("can't load buffer key file");
        }
        else if (type == WOLFSSL_CERT_CHAIN) {
            if (wolfSSL_use_certificate_chain_buffer_format(ssl, buff,
                    (long)sz, format) != WOLFSSL_SUCCESS)
                err_sys("can't load cert chain buffer");
        }

        if (buff)
            free(buff);
    }

    #ifdef TEST_PK_PRIVKEY
    static WC_INLINE int load_key_file(const char* fname, byte** derBuf, word32* derLen)
    {
        int ret;
        byte* buf = NULL;
        size_t bufLen;

        ret = load_file(fname, &buf, &bufLen);
        if (ret != 0)
            return ret;

        *derBuf = (byte*)malloc(bufLen);
        if (*derBuf == NULL) {
            free(buf);
            return MEMORY_E;
        }

        ret = wc_KeyPemToDer(buf, (word32)bufLen, *derBuf, (word32)bufLen, NULL);
        if (ret < 0) {
            free(buf);
            free(*derBuf);
            return ret;
        }
        *derLen = ret;
        free(buf);

        return 0;
    }
    #endif /* TEST_PK_PRIVKEY */

    #endif /* !NO_FILESYSTEM || (NO_FILESYSTEM && FORCE_BUFFER_TEST) */
#endif /* !NO_CERTS */

enum {
    VERIFY_OVERRIDE_ERROR,
    VERIFY_FORCE_FAIL,
    VERIFY_USE_PREVERFIY,
    VERIFY_OVERRIDE_DATE_ERR,
};
static THREAD_LS_T int myVerifyAction = VERIFY_OVERRIDE_ERROR;

/* The verify callback is called for every certificate only when
 * --enable-opensslextra is defined because it sets WOLFSSL_ALWAYS_VERIFY_CB and
 * WOLFSSL_VERIFY_CB_ALL_CERTS.
 * Normal cases of the verify callback only occur on certificate failures when the
 * wolfSSL_set_verify(ssl, SSL_VERIFY_PEER, myVerifyCb); is called
*/

static WC_INLINE int myVerify(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
    char buffer[WOLFSSL_MAX_ERROR_SZ];
#if defined(OPENSSL_EXTRA) || defined(OPENSSL_EXTRA_X509_SMALL)
    WOLFSSL_X509* peer;
#if defined(SHOW_CERTS) && !defined(NO_FILESYSTEM)
    WOLFSSL_BIO* bio = NULL;
    WOLFSSL_STACK* sk = NULL;
    X509* x509 = NULL;
    int i = 0;
#endif
#endif
    (void)preverify;

    /* Verify Callback Arguments:
     * preverify:           1=Verify Okay, 0=Failure
     * store->error:        Failure error code (0 indicates no failure)
     * store->current_cert: Current WOLFSSL_X509 object (only with OPENSSL_EXTRA)
     * store->error_depth:  Current Index
     * store->domain:       Subject CN as string (null term)
     * store->totalCerts:   Number of certs presented by peer
     * store->certs[i]:     A `WOLFSSL_BUFFER_INFO` with plain DER for each cert
     * store->store:        WOLFSSL_X509_STORE with CA cert chain
     * store->store->cm:    WOLFSSL_CERT_MANAGER
     * store->ex_data:      The WOLFSSL object pointer
     * store->discardSessionCerts: When set to non-zero value session certs
        will be discarded (only with SESSION_CERTS)
     */

    printf("In verification callback, error = %d, %s\n", store->error,
                                 wolfSSL_ERR_error_string(store->error, buffer));
#if defined(OPENSSL_EXTRA) || defined(OPENSSL_EXTRA_X509_SMALL)
    peer = store->current_cert;
    if (peer) {
        char* issuer  = wolfSSL_X509_NAME_oneline(
                                       wolfSSL_X509_get_issuer_name(peer), 0, 0);
        char* subject = wolfSSL_X509_NAME_oneline(
                                      wolfSSL_X509_get_subject_name(peer), 0, 0);
        printf("\tPeer's cert info:\n issuer : %s\n subject: %s\n", issuer,
                                                                  subject);
        XFREE(subject, 0, DYNAMIC_TYPE_OPENSSL);
        XFREE(issuer,  0, DYNAMIC_TYPE_OPENSSL);
#if defined(SHOW_CERTS) && !defined(NO_FILESYSTEM)
        /* avoid printing duplicate certs */
        if (store->depth == 1) {
            /* retrieve x509 certs and display them on stdout */
            sk = wolfSSL_X509_STORE_GetCerts(store);

            for (i = 0; i < wolfSSL_sk_X509_num(sk); i++) {
                x509 = wolfSSL_sk_X509_value(sk, i);
                bio = wolfSSL_BIO_new(wolfSSL_BIO_s_file());
                if (bio != NULL) {
                    wolfSSL_BIO_set_fp(bio, stdout, BIO_NOCLOSE);
                    wolfSSL_X509_print(bio, x509);
                    wolfSSL_BIO_free(bio);
                }
            }
            wolfSSL_sk_X509_free(sk);
        }
#endif
    }
    else
        printf("\tPeer has no cert!\n");
#else
    printf("\tPeer certs: %d\n", store->totalCerts);
    #ifdef SHOW_CERTS
    {   int i;
        for (i=0; i<store->totalCerts; i++) {
            WOLFSSL_BUFFER_INFO* cert = &store->certs[i];
            printf("\t\tCert %d: Ptr %p, Len %u\n", i, cert->buffer, cert->length);
        }
    }
    #endif /* SHOW_CERTS */
#endif /* OPENSSL_EXTRA || OPENSSL_EXTRA_X509_SMALL */

    printf("\tSubject's domain name at %d is %s\n", store->error_depth, store->domain);

    /* Testing forced fail case by return zero */
    if (myVerifyAction == VERIFY_FORCE_FAIL) {
        return 0; /* test failure case */
    }

    if (myVerifyAction == VERIFY_OVERRIDE_DATE_ERR && 
        (store->error == ASN_BEFORE_DATE_E || store->error == ASN_AFTER_DATE_E)) {
        printf("Overriding cert date error as example for bad clock testing\n");
        return 1;
    }

    /* If error indicate we are overriding it for testing purposes */
    if (store->error != 0 && myVerifyAction == VERIFY_OVERRIDE_ERROR) {
        printf("\tAllowing failed certificate check, testing only "
            "(shouldn't do this in production)\n");
    }

    /* A non-zero return code indicates failure override */
    return (myVerifyAction == VERIFY_OVERRIDE_ERROR) ? 1 : preverify;
}


#ifdef HAVE_EXT_CACHE

static WC_INLINE WOLFSSL_SESSION* mySessGetCb(WOLFSSL* ssl,
        const unsigned char* id, int id_len, int* copy)
{
    (void)ssl;
    (void)id;
    (void)id_len;
    (void)copy;

    /* using internal cache, this is for testing only */
    return NULL;
}

static WC_INLINE int mySessNewCb(WOLFSSL* ssl, WOLFSSL_SESSION* session)
{
    (void)ssl;
    (void)session;

    /* using internal cache, this is for testing only */
    return 0;
}

static WC_INLINE void mySessRemCb(WOLFSSL_CTX* ctx, WOLFSSL_SESSION* session)
{
    (void)ctx;
    (void)session;

    /* using internal cache, this is for testing only */
}

#endif /* HAVE_EXT_CACHE */


#ifdef HAVE_CRL

static WC_INLINE void CRL_CallBack(const char* url)
{
    printf("CRL callback url = %s\n", url);
}

#endif

#ifndef NO_DH
static WC_INLINE void SetDH(WOLFSSL* ssl)
{
    /* dh1024 p */
    static const unsigned char p[] =
    {
        0xE6, 0x96, 0x9D, 0x3D, 0x49, 0x5B, 0xE3, 0x2C, 0x7C, 0xF1, 0x80, 0xC3,
        0xBD, 0xD4, 0x79, 0x8E, 0x91, 0xB7, 0x81, 0x82, 0x51, 0xBB, 0x05, 0x5E,
        0x2A, 0x20, 0x64, 0x90, 0x4A, 0x79, 0xA7, 0x70, 0xFA, 0x15, 0xA2, 0x59,
        0xCB, 0xD5, 0x23, 0xA6, 0xA6, 0xEF, 0x09, 0xC4, 0x30, 0x48, 0xD5, 0xA2,
        0x2F, 0x97, 0x1F, 0x3C, 0x20, 0x12, 0x9B, 0x48, 0x00, 0x0E, 0x6E, 0xDD,
        0x06, 0x1C, 0xBC, 0x05, 0x3E, 0x37, 0x1D, 0x79, 0x4E, 0x53, 0x27, 0xDF,
        0x61, 0x1E, 0xBB, 0xBE, 0x1B, 0xAC, 0x9B, 0x5C, 0x60, 0x44, 0xCF, 0x02,
        0x3D, 0x76, 0xE0, 0x5E, 0xEA, 0x9B, 0xAD, 0x99, 0x1B, 0x13, 0xA6, 0x3C,
        0x97, 0x4E, 0x9E, 0xF1, 0x83, 0x9E, 0xB5, 0xDB, 0x12, 0x51, 0x36, 0xF7,
        0x26, 0x2E, 0x56, 0xA8, 0x87, 0x15, 0x38, 0xDF, 0xD8, 0x23, 0xC6, 0x50,
        0x50, 0x85, 0xE2, 0x1F, 0x0D, 0xD5, 0xC8, 0x6B,
    };

    /* dh1024 g */
    static const unsigned char g[] =
    {
      0x02,
    };

    wolfSSL_SetTmpDH(ssl, p, sizeof(p), g, sizeof(g));
}

static WC_INLINE void SetDHCtx(WOLFSSL_CTX* ctx)
{
    /* dh1024 p */
    static const unsigned char p[] =
    {
        0xE6, 0x96, 0x9D, 0x3D, 0x49, 0x5B, 0xE3, 0x2C, 0x7C, 0xF1, 0x80, 0xC3,
        0xBD, 0xD4, 0x79, 0x8E, 0x91, 0xB7, 0x81, 0x82, 0x51, 0xBB, 0x05, 0x5E,
        0x2A, 0x20, 0x64, 0x90, 0x4A, 0x79, 0xA7, 0x70, 0xFA, 0x15, 0xA2, 0x59,
        0xCB, 0xD5, 0x23, 0xA6, 0xA6, 0xEF, 0x09, 0xC4, 0x30, 0x48, 0xD5, 0xA2,
        0x2F, 0x97, 0x1F, 0x3C, 0x20, 0x12, 0x9B, 0x48, 0x00, 0x0E, 0x6E, 0xDD,
        0x06, 0x1C, 0xBC, 0x05, 0x3E, 0x37, 0x1D, 0x79, 0x4E, 0x53, 0x27, 0xDF,
        0x61, 0x1E, 0xBB, 0xBE, 0x1B, 0xAC, 0x9B, 0x5C, 0x60, 0x44, 0xCF, 0x02,
        0x3D, 0x76, 0xE0, 0x5E, 0xEA, 0x9B, 0xAD, 0x99, 0x1B, 0x13, 0xA6, 0x3C,
        0x97, 0x4E, 0x9E, 0xF1, 0x83, 0x9E, 0xB5, 0xDB, 0x12, 0x51, 0x36, 0xF7,
        0x26, 0x2E, 0x56, 0xA8, 0x87, 0x15, 0x38, 0xDF, 0xD8, 0x23, 0xC6, 0x50,
        0x50, 0x85, 0xE2, 0x1F, 0x0D, 0xD5, 0xC8, 0x6B,
    };

    /* dh1024 g */
    static const unsigned char g[] =
    {
      0x02,
    };

    wolfSSL_CTX_SetTmpDH(ctx, p, sizeof(p), g, sizeof(g));
}
#endif /* NO_DH */

#ifndef NO_CERTS

static WC_INLINE void CaCb(unsigned char* der, int sz, int type)
{
    (void)der;
    printf("Got CA cache add callback, derSz = %d, type = %d\n", sz, type);
}

#endif /* !NO_CERTS */


/* Wolf Root Directory Helper */
/* KEIL-RL File System does not support relative directory */
#if !defined(WOLFSSL_MDK_ARM) && !defined(WOLFSSL_KEIL_FS) && !defined(WOLFSSL_TIRTOS)
    /* Maximum depth to search for WolfSSL root */
    #define MAX_WOLF_ROOT_DEPTH 5

    static WC_INLINE int ChangeToWolfRoot(void)
    {
        #if !defined(NO_FILESYSTEM) || defined(FORCE_BUFFER_TEST) && \
            !defined(NETOS)
            int depth, res;
            XFILE keyFile;
            for(depth = 0; depth <= MAX_WOLF_ROOT_DEPTH; depth++) {
                keyFile = XFOPEN(dhParamFile, "rb");
                if (keyFile != NULL) {
                    fclose(keyFile);
                    return depth;
                }
            #ifdef USE_WINDOWS_API
                res = SetCurrentDirectoryA("..\\");
            #elif defined(NETOS)
                return 0;
            #else
                res = chdir("../");
            #endif
                if (res < 0) {
                    printf("chdir to ../ failed!\n");
                    break;
                }
            }

            err_sys("wolf root not found");
            return -1;
        #else
            return 0;
        #endif
    }
#endif /* !defined(WOLFSSL_MDK_ARM) && !defined(WOLFSSL_KEIL_FS) && !defined(WOLFSSL_TIRTOS) */

#ifdef HAVE_STACK_SIZE

typedef THREAD_RETURN WOLFSSL_THREAD (*thread_func)(void* args);
#define STACK_CHECK_VAL 0x01

struct stack_size_debug_context {
  unsigned char *myStack;
  size_t stackSize;
#ifdef HAVE_STACK_SIZE_VERBOSE
  size_t *stackSizeHWM_ptr;
  thread_func fn;
  void *args;
#endif
};

#ifdef HAVE_STACK_SIZE_VERBOSE

/* per-subtest stack high water mark tracking.
 *
 * enable with
 *
 * ./configure --enable-stacksize=verbose [...]
 */

static THREAD_RETURN debug_stack_size_verbose_shim(struct stack_size_debug_context *shim_args) {
  StackSizeCheck_myStack = shim_args->myStack;
  StackSizeCheck_stackSize = shim_args->stackSize;
  StackSizeCheck_stackSizeHWM_ptr = shim_args->stackSizeHWM_ptr;
  return shim_args->fn(shim_args->args);
}

static WC_INLINE int StackSizeSetOffset(const char *funcname, void *p)
{
    if (StackSizeCheck_myStack == NULL)
        return -BAD_FUNC_ARG;

    StackSizeCheck_stackOffsetPointer = p;

    printf("setting stack relative offset reference mark in %s to +%lu\n",
        funcname, (unsigned long)((char*)(StackSizeCheck_myStack +
                                  StackSizeCheck_stackSize) - (char *)p));

    return 0;
}

static WC_INLINE ssize_t StackSizeHWM(void)
{
    size_t i;
    ssize_t used;

    if (StackSizeCheck_myStack == NULL)
        return -BAD_FUNC_ARG;

    for (i = 0; i < StackSizeCheck_stackSize; i++) {
        if (StackSizeCheck_myStack[i] != STACK_CHECK_VAL) {
            break;
        }
    }

    used = StackSizeCheck_stackSize - i;
    if ((ssize_t)*StackSizeCheck_stackSizeHWM_ptr < used)
      *StackSizeCheck_stackSizeHWM_ptr = used;

    return used;
}

static WC_INLINE ssize_t StackSizeHWM_OffsetCorrected(void)
{
    ssize_t used = StackSizeHWM();
    if (used < 0)
        return used;
    if (StackSizeCheck_stackOffsetPointer)
        used -= (ssize_t)(((char *)StackSizeCheck_myStack + StackSizeCheck_stackSize) - (char *)StackSizeCheck_stackOffsetPointer);
    return used;
}

static
#ifdef __GNUC__
__attribute__((unused)) __attribute__((noinline))
#endif
int StackSizeHWMReset(void)
{
    volatile ssize_t i;

    if (StackSizeCheck_myStack == NULL)
        return -BAD_FUNC_ARG;

    for (i = (ssize_t)((char *)&i - (char *)StackSizeCheck_myStack) - (ssize_t)sizeof i - 1; i >= 0; --i)
    {
        StackSizeCheck_myStack[i] = STACK_CHECK_VAL;
    }

    return 0;
}

#define STACK_SIZE_CHECKPOINT(...) ({  \
    ssize_t HWM = StackSizeHWM_OffsetCorrected();    \
    __VA_ARGS__;                                     \
    printf("    relative stack peak usage = %ld bytes\n", HWM);  \
    StackSizeHWMReset();                             \
    })

#define STACK_SIZE_CHECKPOINT_WITH_MAX_CHECK(max, ...) ({  \
    ssize_t HWM = StackSizeHWM_OffsetCorrected();    \
    int _ret;                                        \
    __VA_ARGS__;                                     \
    printf("    relative stack peak usage = %ld bytes\n", HWM);  \
    _ret = StackSizeHWMReset();                      \
    if ((max >= 0) && (HWM > (ssize_t)(max))) {      \
        printf("    relative stack usage at %s L%d exceeds designated max %ld bytes.\n", __FILE__, __LINE__, (ssize_t)(max)); \
        _ret = -1;                                   \
    }                                                \
    _ret;                                            \
    })


#ifdef __GNUC__
#define STACK_SIZE_INIT() (void)StackSizeSetOffset(__FUNCTION__, __builtin_frame_address(0))
#endif

#endif /* HAVE_STACK_SIZE_VERBOSE */

static WC_INLINE int StackSizeCheck(func_args* args, thread_func tf)
{
    size_t         i;
    int            ret;
    void*          status;
    unsigned char* myStack = NULL;
    size_t         stackSize = 1024*1024;
    pthread_attr_t myAttr;
    pthread_t      threadId;
#ifdef HAVE_STACK_SIZE_VERBOSE
    struct stack_size_debug_context shim_args;
#endif

#ifdef PTHREAD_STACK_MIN
    if (stackSize < PTHREAD_STACK_MIN)
        stackSize = PTHREAD_STACK_MIN;
#endif

    ret = posix_memalign((void**)&myStack, sysconf(_SC_PAGESIZE), stackSize);
    if (ret != 0 || myStack == NULL)
        err_sys_with_errno("posix_memalign failed\n");

    XMEMSET(myStack, STACK_CHECK_VAL, stackSize);

    ret = pthread_attr_init(&myAttr);
    if (ret != 0)
        err_sys("attr_init failed");

    ret = pthread_attr_setstack(&myAttr, myStack, stackSize);
    if (ret != 0)
        err_sys("attr_setstackaddr failed");

#ifdef HAVE_STACK_SIZE_VERBOSE
    StackSizeCheck_stackSizeHWM = 0;
    shim_args.myStack = myStack;
    shim_args.stackSize = stackSize;
    shim_args.stackSizeHWM_ptr = &StackSizeCheck_stackSizeHWM;
    shim_args.fn = tf;
    shim_args.args = args;
    ret = pthread_create(&threadId, &myAttr, (thread_func)debug_stack_size_verbose_shim, (void *)&shim_args);
#else
    ret = pthread_create(&threadId, &myAttr, tf, args);
#endif
    if (ret != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }

    ret = pthread_join(threadId, &status);
    if (ret != 0)
        err_sys("pthread_join failed");

    for (i = 0; i < stackSize; i++) {
        if (myStack[i] != STACK_CHECK_VAL) {
            break;
        }
    }

    free(myStack);
#ifdef HAVE_STACK_SIZE_VERBOSE
    printf("stack used = %lu\n", StackSizeCheck_stackSizeHWM > (stackSize - i)
        ? (unsigned long)StackSizeCheck_stackSizeHWM
        : (unsigned long)(stackSize - i));
#else
    {
      size_t used = stackSize - i;
      printf("stack used = %lu\n", (unsigned long)used);
    }
#endif

    return (int)((size_t)status);
}

static WC_INLINE int StackSizeCheck_launch(func_args* args, thread_func tf, pthread_t *threadId, void **stack_context)
{
    int ret;
    unsigned char* myStack = NULL;
    size_t stackSize = 1024*1024;
    pthread_attr_t myAttr;

#ifdef PTHREAD_STACK_MIN
    if (stackSize < PTHREAD_STACK_MIN)
        stackSize = PTHREAD_STACK_MIN;
#endif

    struct stack_size_debug_context *shim_args = (struct stack_size_debug_context *)malloc(sizeof *shim_args);
    if (! shim_args) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    ret = posix_memalign((void**)&myStack, sysconf(_SC_PAGESIZE), stackSize);
    if (ret != 0 || myStack == NULL)
        err_sys_with_errno("posix_memalign failed\n");

    XMEMSET(myStack, STACK_CHECK_VAL, stackSize);

    ret = pthread_attr_init(&myAttr);
    if (ret != 0)
        err_sys("attr_init failed");

    ret = pthread_attr_setstack(&myAttr, myStack, stackSize);
    if (ret != 0)
        err_sys("attr_setstackaddr failed");

    shim_args->myStack = myStack;
    shim_args->stackSize = stackSize;
#ifdef HAVE_STACK_SIZE_VERBOSE
    shim_args->stackSizeHWM_ptr = &StackSizeCheck_stackSizeHWM;
    shim_args->fn = tf;
    shim_args->args = args;
    ret = pthread_create(threadId, &myAttr, (thread_func)debug_stack_size_verbose_shim, (void *)shim_args);
#else
    ret = pthread_create(threadId, &myAttr, tf, args);
#endif
    if (ret != 0) {
        fprintf(stderr,"pthread_create failed: %s",strerror(ret));
        exit(EXIT_FAILURE);
    }

    *stack_context = (void *)shim_args;

    return 0;
}

static WC_INLINE int StackSizeCheck_reap(pthread_t threadId, void *stack_context)
{
    struct stack_size_debug_context *shim_args = (struct stack_size_debug_context *)stack_context;
    size_t i;
    void *status;
    int ret = pthread_join(threadId, &status);
    if (ret != 0)
        err_sys("pthread_join failed");

    for (i = 0; i < shim_args->stackSize; i++) {
        if (shim_args->myStack[i] != STACK_CHECK_VAL) {
            break;
        }
    }

    free(shim_args->myStack);
#ifdef HAVE_STACK_SIZE_VERBOSE
    printf("stack used = %lu\n",
        *shim_args->stackSizeHWM_ptr > (shim_args->stackSize - i)
        ? (unsigned long)*shim_args->stackSizeHWM_ptr
        : (unsigned long)(shim_args->stackSize - i));
#else
    {
      size_t used = shim_args->stackSize - i;
      printf("stack used = %lu\n", (unsigned long)used);
    }
#endif
    free(shim_args);

    return (int)((size_t)status);
}


#endif /* HAVE_STACK_SIZE */

#ifndef STACK_SIZE_CHECKPOINT
#define STACK_SIZE_CHECKPOINT(...) (__VA_ARGS__)
#endif
#ifndef STACK_SIZE_INIT
#define STACK_SIZE_INIT()
#endif

#ifdef STACK_TRAP

/* good settings
   --enable-debug --disable-shared C_EXTRA_FLAGS="-DUSER_TIME -DTFM_TIMING_RESISTANT -DPOSITIVE_EXP_ONLY -DSTACK_TRAP"

*/

#ifdef HAVE_STACK_SIZE
    /* client only for now, setrlimit will fail if pthread_create() called */
    /* STACK_SIZE does pthread_create() on client */
    #error "can't use STACK_TRAP with STACK_SIZE, setrlimit will fail"
#endif /* HAVE_STACK_SIZE */

static WC_INLINE void StackTrap(void)
{
    struct rlimit  rl;
    if (getrlimit(RLIMIT_STACK, &rl) != 0)
        err_sys_with_errno("getrlimit failed");
    printf("rlim_cur = %llu\n", rl.rlim_cur);
    rl.rlim_cur = 1024*21;  /* adjust trap size here */
    if (setrlimit(RLIMIT_STACK, &rl) != 0)
        err_sys_with_errno("setrlimit failed");
}

#else /* STACK_TRAP */

static WC_INLINE void StackTrap(void)
{
}

#endif /* STACK_TRAP */


#if defined(ATOMIC_USER) && !defined(WOLFSSL_AEAD_ONLY)

/* Atomic Encrypt Context example */
typedef struct AtomicEncCtx {
    int  keySetup;           /* have we done key setup yet */
    Aes  aes;                /* for aes example */
} AtomicEncCtx;


/* Atomic Decrypt Context example */
typedef struct AtomicDecCtx {
    int  keySetup;           /* have we done key setup yet */
    Aes  aes;                /* for aes example */
} AtomicDecCtx;

#if !defined(NO_HMAC) && !defined(NO_AES) && defined(HAVE_AES_CBC)
static WC_INLINE int myMacEncryptCb(WOLFSSL* ssl, unsigned char* macOut,
       const unsigned char* macIn, unsigned int macInSz, int macContent,
       int macVerify, unsigned char* encOut, const unsigned char* encIn,
       unsigned int encSz, void* ctx)
{
    int  ret;
    Hmac hmac;
    byte myInner[WOLFSSL_TLS_HMAC_INNER_SZ];
    AtomicEncCtx* encCtx = (AtomicEncCtx*)ctx;
    const char* tlsStr = "TLS";

    /* example supports (d)tls aes */
    if (wolfSSL_GetBulkCipher(ssl) != wolfssl_aes) {
        printf("myMacEncryptCb not using AES\n");
        return -1;
    }

    if (strstr(wolfSSL_get_version(ssl), tlsStr) == NULL) {
        printf("myMacEncryptCb not using (D)TLS\n");
        return -1;
    }

    /* hmac, not needed if aead mode */
    wolfSSL_SetTlsHmacInner(ssl, myInner, macInSz, macContent, macVerify);

    ret = wc_HmacInit(&hmac, NULL, INVALID_DEVID);
    if (ret != 0)
        return ret;
    ret = wc_HmacSetKey(&hmac, wolfSSL_GetHmacType(ssl),
               wolfSSL_GetMacSecret(ssl, macVerify), wolfSSL_GetHmacSize(ssl));
    if (ret != 0)
        return ret;
    ret = wc_HmacUpdate(&hmac, myInner, sizeof(myInner));
    if (ret != 0)
        return ret;
    ret = wc_HmacUpdate(&hmac, macIn, macInSz);
    if (ret != 0)
        return ret;
    ret = wc_HmacFinal(&hmac, macOut);
    if (ret != 0)
        return ret;


    /* encrypt setup on first time */
    if (encCtx->keySetup == 0) {
        int   keyLen = wolfSSL_GetKeySize(ssl);
        const byte* key;
        const byte* iv;

        if (wolfSSL_GetSide(ssl) == WOLFSSL_CLIENT_END) {
            key = wolfSSL_GetClientWriteKey(ssl);
            iv  = wolfSSL_GetClientWriteIV(ssl);
        }
        else {
            key = wolfSSL_GetServerWriteKey(ssl);
            iv  = wolfSSL_GetServerWriteIV(ssl);
        }

        ret = wc_AesSetKey(&encCtx->aes, key, keyLen, iv, AES_ENCRYPTION);
        if (ret != 0) {
            printf("AesSetKey failed in myMacEncryptCb\n");
            return ret;
        }
        encCtx->keySetup = 1;
    }

    /* encrypt */
    return wc_AesCbcEncrypt(&encCtx->aes, encOut, encIn, encSz);
}

static WC_INLINE int myDecryptVerifyCb(WOLFSSL* ssl,
       unsigned char* decOut, const unsigned char* decIn,
       unsigned int decSz, int macContent, int macVerify,
       unsigned int* padSz, void* ctx)
{
    AtomicDecCtx* decCtx = (AtomicDecCtx*)ctx;
    int ret      = 0;
    int macInSz  = 0;
    int ivExtra  = 0;
    int digestSz = wolfSSL_GetHmacSize(ssl);
    unsigned int pad     = 0;
    unsigned int padByte = 0;
    Hmac hmac;
    byte myInner[WOLFSSL_TLS_HMAC_INNER_SZ];
    byte verify[WC_MAX_DIGEST_SIZE];
    const char* tlsStr = "TLS";

    /* example supports (d)tls aes */
    if (wolfSSL_GetBulkCipher(ssl) != wolfssl_aes) {
        printf("myMacEncryptCb not using AES\n");
        return -1;
    }

    if (strstr(wolfSSL_get_version(ssl), tlsStr) == NULL) {
        printf("myMacEncryptCb not using (D)TLS\n");
        return -1;
    }

    /*decrypt */
    if (decCtx->keySetup == 0) {
        int   keyLen = wolfSSL_GetKeySize(ssl);
        const byte* key;
        const byte* iv;

        /* decrypt is from other side (peer) */
        if (wolfSSL_GetSide(ssl) == WOLFSSL_SERVER_END) {
            key = wolfSSL_GetClientWriteKey(ssl);
            iv  = wolfSSL_GetClientWriteIV(ssl);
        }
        else {
            key = wolfSSL_GetServerWriteKey(ssl);
            iv  = wolfSSL_GetServerWriteIV(ssl);
        }

        ret = wc_AesSetKey(&decCtx->aes, key, keyLen, iv, AES_DECRYPTION);
        if (ret != 0) {
            printf("AesSetKey failed in myDecryptVerifyCb\n");
            return ret;
        }
        decCtx->keySetup = 1;
    }

    /* decrypt */
    ret = wc_AesCbcDecrypt(&decCtx->aes, decOut, decIn, decSz);
    if (ret != 0)
        return ret;

    if (wolfSSL_GetCipherType(ssl) == WOLFSSL_AEAD_TYPE) {
        *padSz = wolfSSL_GetAeadMacSize(ssl);
        return 0; /* hmac, not needed if aead mode */
    }

    if (wolfSSL_GetCipherType(ssl) == WOLFSSL_BLOCK_TYPE) {
        pad     = *(decOut + decSz - 1);
        padByte = 1;
        if (wolfSSL_IsTLSv1_1(ssl))
            ivExtra = wolfSSL_GetCipherBlockSize(ssl);
    }

    *padSz  = wolfSSL_GetHmacSize(ssl) + pad + padByte;
    macInSz = decSz - ivExtra - digestSz - pad - padByte;

    wolfSSL_SetTlsHmacInner(ssl, myInner, macInSz, macContent, macVerify);

    ret = wc_HmacInit(&hmac, NULL, INVALID_DEVID);
    if (ret != 0)
        return ret;
    ret = wc_HmacSetKey(&hmac, wolfSSL_GetHmacType(ssl),
               wolfSSL_GetMacSecret(ssl, macVerify), digestSz);
    if (ret != 0)
        return ret;
    ret = wc_HmacUpdate(&hmac, myInner, sizeof(myInner));
    if (ret != 0)
        return ret;
    ret = wc_HmacUpdate(&hmac, decOut + ivExtra, macInSz);
    if (ret != 0)
        return ret;
    ret = wc_HmacFinal(&hmac, verify);
    if (ret != 0)
        return ret;

    if (XMEMCMP(verify, decOut + decSz - digestSz - pad - padByte,
               digestSz) != 0) {
        printf("myDecryptVerify verify failed\n");
        return -1;
    }

    return ret;
}

#ifdef HAVE_ENCRYPT_THEN_MAC

static WC_INLINE int myEncryptMacCb(WOLFSSL* ssl, unsigned char* macOut,
       int content, int macVerify, unsigned char* encOut,
       const unsigned char* encIn, unsigned int encSz, void* ctx)
{
    int  ret;
    Hmac hmac;
    AtomicEncCtx* encCtx = (AtomicEncCtx*)ctx;
    byte myInner[WOLFSSL_TLS_HMAC_INNER_SZ];
    const char* tlsStr = "TLS";

    /* example supports (d)tls aes */
    if (wolfSSL_GetBulkCipher(ssl) != wolfssl_aes) {
        printf("myMacEncryptCb not using AES\n");
        return -1;
    }

    if (strstr(wolfSSL_get_version(ssl), tlsStr) == NULL) {
        printf("myMacEncryptCb not using (D)TLS\n");
        return -1;
    }

    /* encrypt setup on first time */
    if (encCtx->keySetup == 0) {
        int   keyLen = wolfSSL_GetKeySize(ssl);
        const byte* key;
        const byte* iv;

        if (wolfSSL_GetSide(ssl) == WOLFSSL_CLIENT_END) {
            key = wolfSSL_GetClientWriteKey(ssl);
            iv  = wolfSSL_GetClientWriteIV(ssl);
        }
        else {
            key = wolfSSL_GetServerWriteKey(ssl);
            iv  = wolfSSL_GetServerWriteIV(ssl);
        }

        ret = wc_AesSetKey(&encCtx->aes, key, keyLen, iv, AES_ENCRYPTION);
        if (ret != 0) {
            printf("AesSetKey failed in myMacEncryptCb\n");
            return ret;
        }
        encCtx->keySetup = 1;
    }

    /* encrypt */
    ret = wc_AesCbcEncrypt(&encCtx->aes, encOut, encIn, encSz);
    if (ret != 0)
        return ret;

    /* Reconstruct record header. */
    wolfSSL_SetTlsHmacInner(ssl, myInner, encSz, content, macVerify);

    ret = wc_HmacInit(&hmac, NULL, INVALID_DEVID);
    if (ret != 0)
        return ret;
    ret = wc_HmacSetKey(&hmac, wolfSSL_GetHmacType(ssl),
               wolfSSL_GetMacSecret(ssl, macVerify), wolfSSL_GetHmacSize(ssl));
    if (ret != 0)
        return ret;
    ret = wc_HmacUpdate(&hmac, myInner, sizeof(myInner));
    if (ret != 0)
        return ret;
    ret = wc_HmacUpdate(&hmac, encOut, encSz);
    if (ret != 0)
        return ret;
    return wc_HmacFinal(&hmac, macOut);
}


static WC_INLINE int myVerifyDecryptCb(WOLFSSL* ssl,
       unsigned char* decOut, const unsigned char* decIn,
       unsigned int decSz, int content, int macVerify,
       unsigned int* padSz, void* ctx)
{
    AtomicDecCtx* decCtx = (AtomicDecCtx*)ctx;
    int ret      = 0;
    int digestSz = wolfSSL_GetHmacSize(ssl);
    Hmac hmac;
    byte myInner[WOLFSSL_TLS_HMAC_INNER_SZ];
    byte verify[WC_MAX_DIGEST_SIZE];
    const char* tlsStr = "TLS";

    /* example supports (d)tls aes */
    if (wolfSSL_GetBulkCipher(ssl) != wolfssl_aes) {
        printf("myMacEncryptCb not using AES\n");
        return -1;
    }

    if (strstr(wolfSSL_get_version(ssl), tlsStr) == NULL) {
        printf("myMacEncryptCb not using (D)TLS\n");
        return -1;
    }

    /* Reconstruct record header. */
    wolfSSL_SetTlsHmacInner(ssl, myInner, decSz, content, macVerify);

    ret = wc_HmacInit(&hmac, NULL, INVALID_DEVID);
    if (ret != 0)
        return ret;
    ret = wc_HmacSetKey(&hmac, wolfSSL_GetHmacType(ssl),
               wolfSSL_GetMacSecret(ssl, macVerify), digestSz);
    if (ret != 0)
        return ret;
    ret = wc_HmacUpdate(&hmac, myInner, sizeof(myInner));
    if (ret != 0)
        return ret;
    ret = wc_HmacUpdate(&hmac, decIn, decSz);
    if (ret != 0)
        return ret;
    ret = wc_HmacFinal(&hmac, verify);
    if (ret != 0)
        return ret;

    if (XMEMCMP(verify, decOut + decSz, digestSz) != 0) {
        printf("myDecryptVerify verify failed\n");
        return -1;
    }

    /* decrypt */
    if (decCtx->keySetup == 0) {
        int   keyLen = wolfSSL_GetKeySize(ssl);
        const byte* key;
        const byte* iv;

        /* decrypt is from other side (peer) */
        if (wolfSSL_GetSide(ssl) == WOLFSSL_SERVER_END) {
            key = wolfSSL_GetClientWriteKey(ssl);
            iv  = wolfSSL_GetClientWriteIV(ssl);
        }
        else {
            key = wolfSSL_GetServerWriteKey(ssl);
            iv  = wolfSSL_GetServerWriteIV(ssl);
        }

        ret = wc_AesSetKey(&decCtx->aes, key, keyLen, iv, AES_DECRYPTION);
        if (ret != 0) {
            printf("AesSetKey failed in myDecryptVerifyCb\n");
            return ret;
        }
        decCtx->keySetup = 1;
    }

    /* decrypt */
    ret = wc_AesCbcDecrypt(&decCtx->aes, decOut, decIn, decSz);
    if (ret != 0)
        return ret;

    *padSz  = *(decOut + decSz - 1) + 1;

    return 0;
}

#endif /* HAVE_ENCRYPT_THEN_MAC */
#endif /* !NO_HMAC && !NO_AES && HAVE_AES_CBC */


static WC_INLINE void SetupAtomicUser(WOLFSSL_CTX* ctx, WOLFSSL* ssl)
{
    AtomicEncCtx* encCtx;
    AtomicDecCtx* decCtx;

    encCtx = (AtomicEncCtx*)malloc(sizeof(AtomicEncCtx));
    if (encCtx == NULL)
        err_sys_with_errno("AtomicEncCtx malloc failed");
    XMEMSET(encCtx, 0, sizeof(AtomicEncCtx));

    decCtx = (AtomicDecCtx*)malloc(sizeof(AtomicDecCtx));
    if (decCtx == NULL) {
        free(encCtx);
        err_sys_with_errno("AtomicDecCtx malloc failed");
    }
    XMEMSET(decCtx, 0, sizeof(AtomicDecCtx));

#if !defined(NO_HMAC) && !defined(NO_AES) && defined(HAVE_AES_CBC)
    wolfSSL_CTX_SetMacEncryptCb(ctx, myMacEncryptCb);
    wolfSSL_SetMacEncryptCtx(ssl, encCtx);

    wolfSSL_CTX_SetDecryptVerifyCb(ctx, myDecryptVerifyCb);
    wolfSSL_SetDecryptVerifyCtx(ssl, decCtx);

    #ifdef HAVE_ENCRYPT_THEN_MAC
    wolfSSL_CTX_SetEncryptMacCb(ctx, myEncryptMacCb);
    wolfSSL_SetEncryptMacCtx(ssl, encCtx);

    wolfSSL_CTX_SetVerifyDecryptCb(ctx, myVerifyDecryptCb);
    wolfSSL_SetVerifyDecryptCtx(ssl, decCtx);
    #endif
#else
    (void)ctx;
    (void)ssl;
#endif
}


static WC_INLINE void FreeAtomicUser(WOLFSSL* ssl)
{
    AtomicEncCtx* encCtx = (AtomicEncCtx*)wolfSSL_GetMacEncryptCtx(ssl);
    AtomicDecCtx* decCtx = (AtomicDecCtx*)wolfSSL_GetDecryptVerifyCtx(ssl);

    /* Encrypt-Then-MAC callbacks use same contexts. */

    free(decCtx);
    free(encCtx);
}

#endif /* ATOMIC_USER */

#ifdef WOLFSSL_STATIC_MEMORY
static WC_INLINE int wolfSSL_PrintStats(WOLFSSL_MEM_STATS* stats)
{
    word16 i;

    if (stats == NULL) {
        return 0;
    }

    /* print to stderr so is on the same pipe as WOLFSSL_DEBUG */
    fprintf(stderr, "Total mallocs   = %d\n", stats->totalAlloc);
    fprintf(stderr, "Total frees     = %d\n", stats->totalFr);
    fprintf(stderr, "Current mallocs = %d\n", stats->curAlloc);
    fprintf(stderr, "Available IO    = %d\n", stats->avaIO);
    fprintf(stderr, "Max con. handshakes  = %d\n", stats->maxHa);
    fprintf(stderr, "Max con. IO          = %d\n", stats->maxIO);
    fprintf(stderr, "State of memory blocks: size   : available \n");
    for (i = 0; i < WOLFMEM_MAX_BUCKETS; i++) {
       fprintf(stderr, "                      : %d\t : %d\n", stats->blockSz[i],
                                                            stats->avaBlock[i]);
    }

    return 1;
}

static WC_INLINE int wolfSSL_PrintStatsConn(WOLFSSL_MEM_CONN_STATS* stats)
{
    if (stats == NULL) {
        return 0;
    }

    fprintf(stderr, "peak connection memory = %d\n", stats->peakMem);
    fprintf(stderr, "current memory in use  = %d\n", stats->curMem);
    fprintf(stderr, "peak connection allocs = %d\n", stats->peakAlloc);
    fprintf(stderr, "current connection allocs = %d\n",stats->curAlloc);
    fprintf(stderr, "total connection allocs   = %d\n", stats->totalAlloc);
    fprintf(stderr, "total connection frees    = %d\n\n", stats->totalFr);

    return 1;
}
#endif /* WOLFSSL_STATIC_MEMORY */

#ifdef HAVE_PK_CALLBACKS

typedef struct PkCbInfo {
    const char* ourKey;
#ifdef TEST_PK_PRIVKEY
    union {
    #ifdef HAVE_ECC
        ecc_key ecc;
    #endif
    #ifdef HAVE_CURVE25519
        curve25519_key curve;
    #endif
    #ifdef HAVE_CURVE448
        curve448_key curve;
    #endif
    } keyGen;
    int hasKeyGen;
#endif
} PkCbInfo;

#if defined(DEBUG_PK_CB) || defined(TEST_PK_PRIVKEY)
    #define WOLFSSL_PKMSG(_f_, ...) printf(_f_, ##__VA_ARGS__)
#else
    #define WOLFSSL_PKMSG(_f_, ...)
#endif

#ifdef HAVE_ECC

static WC_INLINE int myEccKeyGen(WOLFSSL* ssl, ecc_key* key, word32 keySz,
    int ecc_curve, void* ctx)
{
    int       ret;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;
    ecc_key*  new_key;

#ifdef TEST_PK_PRIVKEY
    new_key = cbInfo ? &cbInfo->keyGen.ecc : key;
#else
    new_key = key;
#endif

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK ECC KeyGen: keySz %d, Curve ID %d\n", keySz, ecc_curve);

    ret = wc_ecc_init(new_key);
    if (ret == 0) {
        WC_RNG *rng = wolfSSL_GetRNG(ssl);

        /* create new key */
        ret = wc_ecc_make_key_ex(rng, keySz, new_key, ecc_curve);

    #ifdef TEST_PK_PRIVKEY
        if (ret == 0 && new_key != key) {
            byte qx[MAX_ECC_BYTES], qy[MAX_ECC_BYTES];
            word32 qxLen = sizeof(qx), qyLen = sizeof(qy);

            /* extract public portion from new key into `key` arg */
            ret = wc_ecc_export_public_raw(new_key, qx, &qxLen, qy, &qyLen);
            if (ret == 0) {
                /* load public portion only into key */
                ret = wc_ecc_import_unsigned(key, qx, qy, NULL, ecc_curve);
            }
            (void)qxLen;
            (void)qyLen;
        }
        if (ret == 0 && cbInfo != NULL) {
            cbInfo->hasKeyGen = 1;
        }
    #endif
    }

    WOLFSSL_PKMSG("PK ECC KeyGen: ret %d\n", ret);

    return ret;
}

static WC_INLINE int myEccSign(WOLFSSL* ssl, const byte* in, word32 inSz,
        byte* out, word32* outSz, const byte* key, word32 keySz, void* ctx)
{
    int       ret;
    word32    idx = 0;
    ecc_key   myKey;
    byte*     keyBuf = (byte*)key;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK ECC Sign: inSz %d, keySz %d\n", inSz, keySz);

#ifdef TEST_PK_PRIVKEY
    ret = load_key_file(cbInfo->ourKey, &keyBuf, &keySz);
    if (ret != 0)
        return ret;
#endif

    ret = wc_ecc_init(&myKey);
    if (ret == 0) {
        ret = wc_EccPrivateKeyDecode(keyBuf, &idx, &myKey, keySz);
        if (ret == 0) {
            WC_RNG *rng = wolfSSL_GetRNG(ssl);

            WOLFSSL_PKMSG("PK ECC Sign: Curve ID %d\n", myKey.dp->id);
            ret = wc_ecc_sign_hash(in, inSz, out, outSz, rng, &myKey);
        }
        wc_ecc_free(&myKey);
    }

#ifdef TEST_PK_PRIVKEY
    free(keyBuf);
#endif

    WOLFSSL_PKMSG("PK ECC Sign: ret %d outSz %d\n", ret, *outSz);

    return ret;
}


static WC_INLINE int myEccVerify(WOLFSSL* ssl, const byte* sig, word32 sigSz,
        const byte* hash, word32 hashSz, const byte* key, word32 keySz,
        int* result, void* ctx)
{
    int       ret;
    word32    idx = 0;
    ecc_key   myKey;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK ECC Verify: sigSz %d, hashSz %d, keySz %d\n", sigSz, hashSz, keySz);

    ret = wc_ecc_init(&myKey);
    if (ret == 0) {
        ret = wc_EccPublicKeyDecode(key, &idx, &myKey, keySz);
        if (ret == 0)
            ret = wc_ecc_verify_hash(sig, sigSz, hash, hashSz, result, &myKey);
        wc_ecc_free(&myKey);
    }

    WOLFSSL_PKMSG("PK ECC Verify: ret %d, result %d\n", ret, *result);

    return ret;
}

static WC_INLINE int myEccSharedSecret(WOLFSSL* ssl, ecc_key* otherKey,
        unsigned char* pubKeyDer, unsigned int* pubKeySz,
        unsigned char* out, unsigned int* outlen,
        int side, void* ctx)
{
    int       ret;
    ecc_key*  privKey = NULL;
    ecc_key*  pubKey = NULL;
    ecc_key   tmpKey;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK ECC PMS: Side %s, Peer Curve %d\n",
        side == WOLFSSL_CLIENT_END ? "client" : "server", otherKey->dp->id);

    ret = wc_ecc_init(&tmpKey);
    if (ret != 0) {
        return ret;
    }

    /* for client: create and export public key */
    if (side == WOLFSSL_CLIENT_END) {
    #ifdef TEST_PK_PRIVKEY
        privKey = cbInfo ? &cbInfo->keyGen.ecc : &tmpKey;
    #else
        privKey = &tmpKey;
    #endif
        pubKey = otherKey;

        /* TLS v1.2 and older we must generate a key here for the client ony.
         * TLS v1.3 calls key gen early with key share */
        if (wolfSSL_GetVersion(ssl) < WOLFSSL_TLSV1_3) {
            ret = myEccKeyGen(ssl, privKey, 0, otherKey->dp->id, ctx);
            if (ret == 0) {
                ret = wc_ecc_export_x963(privKey, pubKeyDer, pubKeySz);
            }
        }
    }

    /* for server: import public key */
    else if (side == WOLFSSL_SERVER_END) {
    #ifdef TEST_PK_PRIVKEY
        privKey = cbInfo ? &cbInfo->keyGen.ecc : otherKey;
    #else
        privKey = otherKey;
    #endif
        pubKey = &tmpKey;

        ret = wc_ecc_import_x963_ex(pubKeyDer, *pubKeySz, pubKey,
            otherKey->dp->id);
    }
    else {
        ret = BAD_FUNC_ARG;
    }

    if (privKey == NULL || pubKey == NULL) {
        ret = BAD_FUNC_ARG;
    }

#if defined(ECC_TIMING_RESISTANT) && !defined(HAVE_FIPS) && \
                                                         !defined(HAVE_SELFTEST)
    if (ret == 0) {
        ret = wc_ecc_set_rng(privKey, wolfSSL_GetRNG(ssl));
    }
#endif

    /* generate shared secret and return it */
    if (ret == 0) {
        ret = wc_ecc_shared_secret(privKey, pubKey, out, outlen);

    #ifdef WOLFSSL_ASYNC_CRYPT
        if (ret == WC_PENDING_E) {
            ret = wc_AsyncWait(ret, &privKey->asyncDev, WC_ASYNC_FLAG_CALL_AGAIN);
        }
    #endif
    }

#ifdef TEST_PK_PRIVKEY
    if (cbInfo && cbInfo->hasKeyGen) {
        wc_ecc_free(&cbInfo->keyGen.ecc);
        cbInfo->hasKeyGen = 0;
    }
#endif

    wc_ecc_free(&tmpKey);

    WOLFSSL_PKMSG("PK ECC PMS: ret %d, PubKeySz %d, OutLen %d\n", ret, *pubKeySz, *outlen);

    return ret;
}

#endif /* HAVE_ECC */

#if defined(HAVE_ED25519) && defined(HAVE_ED25519_KEY_IMPORT)
#ifdef HAVE_ED25519_SIGN
static WC_INLINE int myEd25519Sign(WOLFSSL* ssl, const byte* in, word32 inSz,
        byte* out, word32* outSz, const byte* key, word32 keySz, void* ctx)
{
    int         ret;
    word32      idx = 0;
    ed25519_key myKey;
    byte*       keyBuf = (byte*)key;
    PkCbInfo*   cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK 25519 Sign: inSz %d, keySz %d\n", inSz, keySz);

#ifdef TEST_PK_PRIVKEY
    ret = load_key_file(cbInfo->ourKey, &keyBuf, &keySz);
    if (ret != 0)
        return ret;
#endif

    ret = wc_ed25519_init(&myKey);
    if (ret == 0) {
        ret = wc_Ed25519PrivateKeyDecode(keyBuf, &idx, &myKey, keySz);
        if (ret == 0)
            ret = wc_ed25519_sign_msg(in, inSz, out, outSz, &myKey);
        wc_ed25519_free(&myKey);
    }

#ifdef TEST_PK_PRIVKEY
    free(keyBuf);
#endif

    WOLFSSL_PKMSG("PK 25519 Sign: ret %d, outSz %d\n", ret, *outSz);

    return ret;
}
#endif /* HAVE_ED25519_SIGN */


#ifdef HAVE_ED25519_VERIFY
static WC_INLINE int myEd25519Verify(WOLFSSL* ssl, const byte* sig, word32 sigSz,
        const byte* msg, word32 msgSz, const byte* key, word32 keySz,
        int* result, void* ctx)
{
    int         ret;
    ed25519_key myKey;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK 25519 Verify: sigSz %d, msgSz %d, keySz %d\n", sigSz, msgSz, keySz);

    ret = wc_ed25519_init(&myKey);
    if (ret == 0) {
        ret = wc_ed25519_import_public(key, keySz, &myKey);
        if (ret == 0) {
            ret = wc_ed25519_verify_msg(sig, sigSz, msg, msgSz, result, &myKey);
        }
        wc_ed25519_free(&myKey);
    }

    WOLFSSL_PKMSG("PK 25519 Verify: ret %d, result %d\n", ret, *result);

    return ret;
}
#endif /* HAVE_ED25519_VERIFY */
#endif /* HAVE_ED25519 && HAVE_ED25519_KEY_IMPORT */

#ifdef HAVE_CURVE25519
static WC_INLINE int myX25519KeyGen(WOLFSSL* ssl, curve25519_key* key,
    unsigned int keySz, void* ctx)
{
    int       ret;
    WC_RNG    rng;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK 25519 KeyGen: keySz %d\n", keySz);

    ret = wc_InitRng(&rng);
    if (ret != 0)
        return ret;

    ret = wc_curve25519_make_key(&rng, keySz, key);

    wc_FreeRng(&rng);

    WOLFSSL_PKMSG("PK 25519 KeyGen: ret %d\n", ret);

    return ret;
}

static WC_INLINE int myX25519SharedSecret(WOLFSSL* ssl, curve25519_key* otherKey,
        unsigned char* pubKeyDer, unsigned int* pubKeySz,
        unsigned char* out, unsigned int* outlen,
        int side, void* ctx)
{
    int      ret;
    curve25519_key* privKey = NULL;
    curve25519_key* pubKey = NULL;
    curve25519_key  tmpKey;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK 25519 PMS: side %s\n",
        side == WOLFSSL_CLIENT_END ? "client" : "server");

    ret = wc_curve25519_init(&tmpKey);
    if (ret != 0) {
        return ret;
    }

    /* for client: create and export public key */
    if (side == WOLFSSL_CLIENT_END) {
        WC_RNG rng;

        privKey = &tmpKey;
        pubKey = otherKey;

        ret = wc_InitRng(&rng);
        if (ret == 0) {
            ret = wc_curve25519_make_key(&rng, CURVE25519_KEYSIZE, privKey);
            if (ret == 0) {
                ret = wc_curve25519_export_public_ex(privKey, pubKeyDer,
                    pubKeySz, EC25519_LITTLE_ENDIAN);
            }
            wc_FreeRng(&rng);
        }
    }

    /* for server: import public key */
    else if (side == WOLFSSL_SERVER_END) {
        privKey = otherKey;
        pubKey = &tmpKey;

        ret = wc_curve25519_import_public_ex(pubKeyDer, *pubKeySz, pubKey,
            EC25519_LITTLE_ENDIAN);
    }
    else {
        ret = BAD_FUNC_ARG;
    }

    /* generate shared secret and return it */
    if (ret == 0) {
        ret = wc_curve25519_shared_secret_ex(privKey, pubKey, out, outlen,
            EC25519_LITTLE_ENDIAN);
    }

    wc_curve25519_free(&tmpKey);

    WOLFSSL_PKMSG("PK 25519 PMS: ret %d, pubKeySz %d, outLen %d\n",
        ret, *pubKeySz, *outlen);

    return ret;
}
#endif /* HAVE_CURVE25519 */

#if defined(HAVE_ED448) && defined(HAVE_ED448_KEY_IMPORT)
#ifdef HAVE_ED448_SIGN
static WC_INLINE int myEd448Sign(WOLFSSL* ssl, const byte* in, word32 inSz,
        byte* out, word32* outSz, const byte* key, word32 keySz, void* ctx)
{
    int         ret;
    word32      idx = 0;
    ed448_key   myKey;
    byte*       keyBuf = (byte*)key;
    PkCbInfo*   cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK 448 Sign: inSz %d, keySz %d\n", inSz, keySz);

#ifdef TEST_PK_PRIVKEY
    ret = load_key_file(cbInfo->ourKey, &keyBuf, &keySz);
    if (ret != 0)
        return ret;
#endif

    ret = wc_ed448_init(&myKey);
    if (ret == 0) {
        ret = wc_Ed448PrivateKeyDecode(keyBuf, &idx, &myKey, keySz);
        if (ret == 0)
            ret = wc_ed448_sign_msg(in, inSz, out, outSz, &myKey, NULL, 0);
        wc_ed448_free(&myKey);
    }

#ifdef TEST_PK_PRIVKEY
    free(keyBuf);
#endif

    WOLFSSL_PKMSG("PK 448 Sign: ret %d, outSz %d\n", ret, *outSz);

    return ret;
}
#endif /* HAVE_ED448_SIGN */


#ifdef HAVE_ED448_VERIFY
static WC_INLINE int myEd448Verify(WOLFSSL* ssl, const byte* sig, word32 sigSz,
        const byte* msg, word32 msgSz, const byte* key, word32 keySz,
        int* result, void* ctx)
{
    int         ret;
    ed448_key   myKey;
    PkCbInfo*   cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK 448 Verify: sigSz %d, msgSz %d, keySz %d\n", sigSz, msgSz,
                  keySz);

    ret = wc_ed448_init(&myKey);
    if (ret == 0) {
        ret = wc_ed448_import_public(key, keySz, &myKey);
        if (ret == 0) {
            ret = wc_ed448_verify_msg(sig, sigSz, msg, msgSz, result, &myKey,
                                                                       NULL, 0);
        }
        wc_ed448_free(&myKey);
    }

    WOLFSSL_PKMSG("PK 448 Verify: ret %d, result %d\n", ret, *result);

    return ret;
}
#endif /* HAVE_ED448_VERIFY */
#endif /* HAVE_ED448 && HAVE_ED448_KEY_IMPORT */

#ifdef HAVE_CURVE448
static WC_INLINE int myX448KeyGen(WOLFSSL* ssl, curve448_key* key,
    unsigned int keySz, void* ctx)
{
    int       ret;
    WC_RNG    rng;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK 448 KeyGen: keySz %d\n", keySz);

    ret = wc_InitRng(&rng);
    if (ret != 0)
        return ret;

    ret = wc_curve448_make_key(&rng, keySz, key);

    wc_FreeRng(&rng);

    WOLFSSL_PKMSG("PK 448 KeyGen: ret %d\n", ret);

    return ret;
}

static WC_INLINE int myX448SharedSecret(WOLFSSL* ssl, curve448_key* otherKey,
        unsigned char* pubKeyDer, unsigned int* pubKeySz,
        unsigned char* out, unsigned int* outlen,
        int side, void* ctx)
{
    int           ret;
    curve448_key* privKey = NULL;
    curve448_key* pubKey = NULL;
    curve448_key  tmpKey;
    PkCbInfo*     cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK 448 PMS: side %s\n",
        side == WOLFSSL_CLIENT_END ? "client" : "server");

    ret = wc_curve448_init(&tmpKey);
    if (ret != 0) {
        return ret;
    }

    /* for client: create and export public key */
    if (side == WOLFSSL_CLIENT_END) {
        WC_RNG rng;

        privKey = &tmpKey;
        pubKey = otherKey;

        ret = wc_InitRng(&rng);
        if (ret == 0) {
            ret = wc_curve448_make_key(&rng, CURVE448_KEY_SIZE, privKey);
            if (ret == 0) {
                ret = wc_curve448_export_public_ex(privKey, pubKeyDer,
                    pubKeySz, EC448_LITTLE_ENDIAN);
            }
            wc_FreeRng(&rng);
        }
    }

    /* for server: import public key */
    else if (side == WOLFSSL_SERVER_END) {
        privKey = otherKey;
        pubKey = &tmpKey;

        ret = wc_curve448_import_public_ex(pubKeyDer, *pubKeySz, pubKey,
            EC448_LITTLE_ENDIAN);
    }
    else {
        ret = BAD_FUNC_ARG;
    }

    /* generate shared secret and return it */
    if (ret == 0) {
        ret = wc_curve448_shared_secret_ex(privKey, pubKey, out, outlen,
            EC448_LITTLE_ENDIAN);
    }

    wc_curve448_free(&tmpKey);

    WOLFSSL_PKMSG("PK 448 PMS: ret %d, pubKeySz %d, outLen %d\n",
        ret, *pubKeySz, *outlen);

    return ret;
}
#endif /* HAVE_CURVE448 */

#ifndef NO_DH
static WC_INLINE int myDhCallback(WOLFSSL* ssl, struct DhKey* key,
        const unsigned char* priv, unsigned int privSz,
        const unsigned char* pubKeyDer, unsigned int pubKeySz,
        unsigned char* out, unsigned int* outlen,
        void* ctx)
{
    int ret;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    /* return 0 on success */
    ret = wc_DhAgree(key, out, outlen, priv, privSz, pubKeyDer, pubKeySz);

    WOLFSSL_PKMSG("PK ED Agree: ret %d, privSz %d, pubKeySz %d, outlen %d\n",
        ret, privSz, pubKeySz, *outlen);

    return ret;
}

#endif /* !NO_DH */

#ifndef NO_RSA

static WC_INLINE int myRsaSign(WOLFSSL* ssl, const byte* in, word32 inSz,
        byte* out, word32* outSz, const byte* key, word32 keySz, void* ctx)
{
    WC_RNG  rng;
    int     ret;
    word32  idx = 0;
    RsaKey  myKey;
    byte*   keyBuf = (byte*)key;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK RSA Sign: inSz %d, keySz %d\n", inSz, keySz);

#ifdef TEST_PK_PRIVKEY
    ret = load_key_file(cbInfo->ourKey, &keyBuf, &keySz);
    if (ret != 0)
        return ret;
#endif

    ret = wc_InitRng(&rng);
    if (ret != 0)
        return ret;

    ret = wc_InitRsaKey(&myKey, NULL);
    if (ret == 0) {
        ret = wc_RsaPrivateKeyDecode(keyBuf, &idx, &myKey, keySz);
        if (ret == 0)
            ret = wc_RsaSSL_Sign(in, inSz, out, *outSz, &myKey, &rng);
        if (ret > 0) {  /* save and convert to 0 success */
            *outSz = ret;
            ret = 0;
        }
        wc_FreeRsaKey(&myKey);
    }
    wc_FreeRng(&rng);

#ifdef TEST_PK_PRIVKEY
    free(keyBuf);
#endif

    WOLFSSL_PKMSG("PK RSA Sign: ret %d, outSz %d\n", ret, *outSz);

    return ret;
}


static WC_INLINE int myRsaVerify(WOLFSSL* ssl, byte* sig, word32 sigSz,
        byte** out, const byte* key, word32 keySz, void* ctx)
{
    int     ret;
    word32  idx = 0;
    RsaKey  myKey;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK RSA Verify: sigSz %d, keySz %d\n", sigSz, keySz);

    ret = wc_InitRsaKey(&myKey, NULL);
    if (ret == 0) {
        ret = wc_RsaPublicKeyDecode(key, &idx, &myKey, keySz);
        if (ret == 0)
            ret = wc_RsaSSL_VerifyInline(sig, sigSz, out, &myKey);
        wc_FreeRsaKey(&myKey);
    }

    WOLFSSL_PKMSG("PK RSA Verify: ret %d\n", ret);

    return ret;
}

static WC_INLINE int myRsaSignCheck(WOLFSSL* ssl, byte* sig, word32 sigSz,
        byte** out, const byte* key, word32 keySz, void* ctx)
{
    int     ret;
    word32  idx = 0;
    RsaKey  myKey;
    byte*   keyBuf = (byte*)key;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK RSA SignCheck: sigSz %d, keySz %d\n", sigSz, keySz);

#ifdef TEST_PK_PRIVKEY
    ret = load_key_file(cbInfo->ourKey, &keyBuf, &keySz);
    if (ret != 0)
        return ret;
#endif

    ret = wc_InitRsaKey(&myKey, NULL);
    if (ret == 0) {
        ret = wc_RsaPrivateKeyDecode(keyBuf, &idx, &myKey, keySz);
        if (ret == 0)
            ret = wc_RsaSSL_VerifyInline(sig, sigSz, out, &myKey);
        wc_FreeRsaKey(&myKey);
    }
#ifdef TEST_PK_PRIVKEY
    free(keyBuf);
#endif

    WOLFSSL_PKMSG("PK RSA SignCheck: ret %d\n", ret);

    return ret;
}

#ifdef WC_RSA_PSS
static WC_INLINE int myRsaPssSign(WOLFSSL* ssl, const byte* in, word32 inSz,
        byte* out, word32* outSz, int hash, int mgf, const byte* key,
        word32 keySz, void* ctx)
{
    enum wc_HashType hashType = WC_HASH_TYPE_NONE;
    WC_RNG           rng;
    int              ret;
    word32           idx = 0;
    RsaKey           myKey;
    byte*            keyBuf = (byte*)key;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK RSA PSS Sign: inSz %d, hash %d, mgf %d, keySz %d\n",
        inSz, hash, mgf, keySz);

#ifdef TEST_PK_PRIVKEY
    ret = load_key_file(cbInfo->ourKey, &keyBuf, &keySz);
    if (ret != 0)
        return ret;
#endif

    switch (hash) {
#ifndef NO_SHA256
        case SHA256h:
            hashType = WC_HASH_TYPE_SHA256;
            break;
#endif
#ifdef WOLFSSL_SHA384
        case SHA384h:
            hashType = WC_HASH_TYPE_SHA384;
            break;
#endif
#ifdef WOLFSSL_SHA512
        case SHA512h:
            hashType = WC_HASH_TYPE_SHA512;
            break;
#endif
    }

    ret = wc_InitRng(&rng);
    if (ret != 0)
        return ret;

    ret = wc_InitRsaKey(&myKey, NULL);
    if (ret == 0) {
        ret = wc_RsaPrivateKeyDecode(keyBuf, &idx, &myKey, keySz);
        if (ret == 0) {
            ret = wc_RsaPSS_Sign(in, inSz, out, *outSz, hashType, mgf, &myKey,
                                 &rng);
        }
        if (ret > 0) {  /* save and convert to 0 success */
            *outSz = ret;
            ret = 0;
        }
        wc_FreeRsaKey(&myKey);
    }
    wc_FreeRng(&rng);

#ifdef TEST_PK_PRIVKEY
    free(keyBuf);
#endif

    WOLFSSL_PKMSG("PK RSA PSS Sign: ret %d, outSz %d\n", ret, *outSz);

    return ret;
}


static WC_INLINE int myRsaPssVerify(WOLFSSL* ssl, byte* sig, word32 sigSz,
        byte** out, int hash, int mgf, const byte* key, word32 keySz, void* ctx)
{
    int       ret;
    word32    idx = 0;
    RsaKey    myKey;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;
    enum wc_HashType hashType = WC_HASH_TYPE_NONE;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK RSA PSS Verify: sigSz %d, hash %d, mgf %d, keySz %d\n",
        sigSz, hash, mgf, keySz);

    switch (hash) {
#ifndef NO_SHA256
        case SHA256h:
            hashType = WC_HASH_TYPE_SHA256;
            break;
#endif
#ifdef WOLFSSL_SHA384
        case SHA384h:
            hashType = WC_HASH_TYPE_SHA384;
            break;
#endif
#ifdef WOLFSSL_SHA512
        case SHA512h:
            hashType = WC_HASH_TYPE_SHA512;
            break;
#endif
    }

    ret = wc_InitRsaKey(&myKey, NULL);
    if (ret == 0) {
        ret = wc_RsaPublicKeyDecode(key, &idx, &myKey, keySz);
        if (ret == 0) {
            ret = wc_RsaPSS_VerifyInline(sig, sigSz, out, hashType, mgf,
                                         &myKey);
            }
        wc_FreeRsaKey(&myKey);
    }

    WOLFSSL_PKMSG("PK RSA PSS Verify: ret %d\n", ret);

    return ret;
}

static WC_INLINE int myRsaPssSignCheck(WOLFSSL* ssl, byte* sig, word32 sigSz,
        byte** out, int hash, int mgf, const byte* key, word32 keySz, void* ctx)
{
    int       ret;
    word32    idx = 0;
    RsaKey    myKey;
    byte*     keyBuf = (byte*)key;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;
    enum wc_HashType hashType = WC_HASH_TYPE_NONE;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK RSA PSS SignCheck: sigSz %d, hash %d, mgf %d, keySz %d\n",
        sigSz, hash, mgf, keySz);

#ifdef TEST_PK_PRIVKEY
    ret = load_key_file(cbInfo->ourKey, &keyBuf, &keySz);
    if (ret != 0)
        return ret;
#endif

    switch (hash) {
#ifndef NO_SHA256
        case SHA256h:
            hashType = WC_HASH_TYPE_SHA256;
            break;
#endif
#ifdef WOLFSSL_SHA384
        case SHA384h:
            hashType = WC_HASH_TYPE_SHA384;
            break;
#endif
#ifdef WOLFSSL_SHA512
        case SHA512h:
            hashType = WC_HASH_TYPE_SHA512;
            break;
#endif
    }

    ret = wc_InitRsaKey(&myKey, NULL);
    if (ret == 0) {
        ret = wc_RsaPrivateKeyDecode(keyBuf, &idx, &myKey, keySz);
        if (ret == 0) {
            ret = wc_RsaPSS_VerifyInline(sig, sigSz, out, hashType, mgf,
                                         &myKey);
            }
        wc_FreeRsaKey(&myKey);
    }

#ifdef TEST_PK_PRIVKEY
    free(keyBuf);
#endif

    WOLFSSL_PKMSG("PK RSA PSS SignCheck: ret %d\n", ret);

    return ret;
}
#endif


static WC_INLINE int myRsaEnc(WOLFSSL* ssl, const byte* in, word32 inSz,
                           byte* out, word32* outSz, const byte* key,
                           word32 keySz, void* ctx)
{
    int       ret;
    word32    idx = 0;
    RsaKey    myKey;
    WC_RNG    rng;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK RSA Enc: inSz %d, keySz %d\n", inSz, keySz);

    ret = wc_InitRng(&rng);
    if (ret != 0)
        return ret;

    ret = wc_InitRsaKey(&myKey, NULL);
    if (ret == 0) {
        ret = wc_RsaPublicKeyDecode(key, &idx, &myKey, keySz);
        if (ret == 0) {
            ret = wc_RsaPublicEncrypt(in, inSz, out, *outSz, &myKey, &rng);
            if (ret > 0) {
                *outSz = ret;
                ret = 0;  /* reset to success */
            }
        }
        wc_FreeRsaKey(&myKey);
    }
    wc_FreeRng(&rng);

    WOLFSSL_PKMSG("PK RSA Enc: ret %d, outSz %d\n", ret, *outSz);

    return ret;
}

static WC_INLINE int myRsaDec(WOLFSSL* ssl, byte* in, word32 inSz,
                           byte** out,
                           const byte* key, word32 keySz, void* ctx)
{
    int       ret;
    word32    idx = 0;
    RsaKey    myKey;
    byte*     keyBuf = (byte*)key;
    PkCbInfo* cbInfo = (PkCbInfo*)ctx;

    (void)ssl;
    (void)cbInfo;

    WOLFSSL_PKMSG("PK RSA Dec: inSz %d, keySz %d\n", inSz, keySz);

#ifdef TEST_PK_PRIVKEY
    ret = load_key_file(cbInfo->ourKey, &keyBuf, &keySz);
    if (ret != 0)
        return ret;
#endif

    ret = wc_InitRsaKey(&myKey, NULL);
    if (ret == 0) {
        ret = wc_RsaPrivateKeyDecode(keyBuf, &idx, &myKey, keySz);
        if (ret == 0) {
            #ifdef WC_RSA_BLINDING
                ret = wc_RsaSetRNG(&myKey, wolfSSL_GetRNG(ssl));
                if (ret != 0) {
                    wc_FreeRsaKey(&myKey);
                    return ret;
                }
            #endif
            ret = wc_RsaPrivateDecryptInline(in, inSz, out, &myKey);
        }
        wc_FreeRsaKey(&myKey);
    }

#ifdef TEST_PK_PRIVKEY
    free(keyBuf);
#endif

    WOLFSSL_PKMSG("PK RSA Dec: ret %d\n", ret);

    return ret;
}

#endif /* NO_RSA */

static WC_INLINE void SetupPkCallbacks(WOLFSSL_CTX* ctx)
{
    (void)ctx;

    #ifdef HAVE_ECC
        wolfSSL_CTX_SetEccKeyGenCb(ctx, myEccKeyGen);
        wolfSSL_CTX_SetEccSignCb(ctx, myEccSign);
        wolfSSL_CTX_SetEccVerifyCb(ctx, myEccVerify);
        wolfSSL_CTX_SetEccSharedSecretCb(ctx, myEccSharedSecret);
    #endif /* HAVE_ECC */
    #ifndef NO_DH
        wolfSSL_CTX_SetDhAgreeCb(ctx, myDhCallback);
    #endif
    #if defined(HAVE_ED25519) && defined(HAVE_ED25519_KEY_IMPORT)
        #ifdef HAVE_ED25519_SIGN
        wolfSSL_CTX_SetEd25519SignCb(ctx, myEd25519Sign);
        #endif
        #ifdef HAVE_ED25519_VERIFY
        wolfSSL_CTX_SetEd25519VerifyCb(ctx, myEd25519Verify);
        #endif
    #endif
    #ifdef HAVE_CURVE25519
        wolfSSL_CTX_SetX25519KeyGenCb(ctx, myX25519KeyGen);
        wolfSSL_CTX_SetX25519SharedSecretCb(ctx, myX25519SharedSecret);
    #endif
    #if defined(HAVE_ED448) && defined(HAVE_ED448_KEY_IMPORT)
        #if defined(HAVE_ED448_SIGN)
        wolfSSL_CTX_SetEd448SignCb(ctx, myEd448Sign);
        #endif
        #if defined(HAVE_ED448_VERIFY)
        wolfSSL_CTX_SetEd448VerifyCb(ctx, myEd448Verify);
        #endif
    #endif
    #ifdef HAVE_CURVE448
        wolfSSL_CTX_SetX448KeyGenCb(ctx, myX448KeyGen);
        wolfSSL_CTX_SetX448SharedSecretCb(ctx, myX448SharedSecret);
    #endif
    #ifndef NO_RSA
        wolfSSL_CTX_SetRsaSignCb(ctx, myRsaSign);
        wolfSSL_CTX_SetRsaVerifyCb(ctx, myRsaVerify);
        wolfSSL_CTX_SetRsaSignCheckCb(ctx, myRsaSignCheck);
        #ifdef WC_RSA_PSS
            wolfSSL_CTX_SetRsaPssSignCb(ctx, myRsaPssSign);
            wolfSSL_CTX_SetRsaPssVerifyCb(ctx, myRsaPssVerify);
            wolfSSL_CTX_SetRsaPssSignCheckCb(ctx, myRsaPssSignCheck);
        #endif
        wolfSSL_CTX_SetRsaEncCb(ctx, myRsaEnc);
        wolfSSL_CTX_SetRsaDecCb(ctx, myRsaDec);
    #endif /* NO_RSA */
}

static WC_INLINE void SetupPkCallbackContexts(WOLFSSL* ssl, void* myCtx)
{
    #ifdef HAVE_ECC
        wolfSSL_SetEccKeyGenCtx(ssl, myCtx);
        wolfSSL_SetEccSignCtx(ssl, myCtx);
        wolfSSL_SetEccVerifyCtx(ssl, myCtx);
        wolfSSL_SetEccSharedSecretCtx(ssl, myCtx);
    #endif /* HAVE_ECC */
    #ifndef NO_DH
        wolfSSL_SetDhAgreeCtx(ssl, myCtx);
    #endif
    #ifdef HAVE_ED25519
        wolfSSL_SetEd25519SignCtx(ssl, myCtx);
        wolfSSL_SetEd25519VerifyCtx(ssl, myCtx);
    #endif
    #ifdef HAVE_CURVE25519
        wolfSSL_SetX25519KeyGenCtx(ssl, myCtx);
        wolfSSL_SetX25519SharedSecretCtx(ssl, myCtx);
    #endif
    #ifdef HAVE_ED448
        wolfSSL_SetEd448SignCtx(ssl, myCtx);
        wolfSSL_SetEd448VerifyCtx(ssl, myCtx);
    #endif
    #ifdef HAVE_CURVE448
        wolfSSL_SetX448KeyGenCtx(ssl, myCtx);
        wolfSSL_SetX448SharedSecretCtx(ssl, myCtx);
    #endif
    #ifndef NO_RSA
        wolfSSL_SetRsaSignCtx(ssl, myCtx);
        wolfSSL_SetRsaVerifyCtx(ssl, myCtx);
        #ifdef WC_RSA_PSS
            wolfSSL_SetRsaPssSignCtx(ssl, myCtx);
            wolfSSL_SetRsaPssVerifyCtx(ssl, myCtx);
        #endif
        wolfSSL_SetRsaEncCtx(ssl, myCtx);
        wolfSSL_SetRsaDecCtx(ssl, myCtx);
    #endif /* NO_RSA */
}

#endif /* HAVE_PK_CALLBACKS */

#ifdef USE_WOLFSSL_IO
static WC_INLINE int SimulateWantWriteIOSendCb(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    static int wantWriteFlag = 1;

    int sent;
    int sd = *(int*)ctx;

    (void)ssl;

    if (!wantWriteFlag)
    {
        wantWriteFlag = 1;

        sent = wolfIO_Send(sd, buf, sz, 0);
        if (sent < 0) {
            int err = errno;

            if (err == SOCKET_EWOULDBLOCK || err == SOCKET_EAGAIN) {
                return WOLFSSL_CBIO_ERR_WANT_WRITE;
            }
            else if (err == SOCKET_ECONNRESET) {
                return WOLFSSL_CBIO_ERR_CONN_RST;
            }
            else if (err == SOCKET_EINTR) {
                return WOLFSSL_CBIO_ERR_ISR;
            }
            else if (err == SOCKET_EPIPE) {
                return WOLFSSL_CBIO_ERR_CONN_CLOSE;
            }
            else {
                return WOLFSSL_CBIO_ERR_GENERAL;
            }
        }

        return sent;
    }
    else
    {
        wantWriteFlag = 0;
        return WOLFSSL_CBIO_ERR_WANT_WRITE;
    }
}
#endif /* USE_WOLFSSL_IO */

#if defined(__hpux__) || defined(__MINGW32__) || defined (WOLFSSL_TIRTOS) \
                      || defined(_MSC_VER)

/* HP/UX doesn't have strsep, needed by test/suites.c */
static WC_INLINE char* strsep(char **stringp, const char *delim)
{
    char* start;
    char* end;

    start = *stringp;
    if (start == NULL)
        return NULL;

    if ((end = strpbrk(start, delim))) {
        *end++ = '\0';
        *stringp = end;
    } else {
        *stringp = NULL;
    }

    return start;
}

#endif /* __hpux__ and others */

/* Create unique filename, len is length of tempfn name, assuming
   len does not include null terminating character,
   num is number of characters in tempfn name to randomize */
static WC_INLINE const char* mymktemp(char *tempfn, int len, int num)
{
    int x, size;
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";
    WC_RNG rng;
    byte   out;

    if (tempfn == NULL || len < 1 || num < 1 || len <= num) {
        printf("Bad input\n");
        return NULL;
    }

    size = len - 1;

    if (wc_InitRng(&rng) != 0) {
        printf("InitRng failed\n");
        return NULL;
    }

    for (x = size; x > size - num; x--) {
        if (wc_RNG_GenerateBlock(&rng,(byte*)&out, sizeof(out)) != 0) {
            printf("RNG_GenerateBlock failed\n");
            return NULL;
        }
        tempfn[x] = alphanum[out % (sizeof(alphanum) - 1)];
    }
    tempfn[len] = '\0';

    wc_FreeRng(&rng);
    (void)rng; /* for WC_NO_RNG case */

    return tempfn;
}



#if defined(HAVE_SESSION_TICKET) && defined(WOLFSSL_NO_DEF_TICKET_ENC_CB) && \
    ((defined(HAVE_CHACHA) && defined(HAVE_POLY1305)) || \
      defined(HAVE_AESGCM))

#if defined(HAVE_CHACHA) && defined(HAVE_POLY1305)
    #include <wolfssl/wolfcrypt/chacha20_poly1305.h>
    #define WOLFSSL_TICKET_KEY_SZ CHACHA20_POLY1305_AEAD_KEYSIZE
#elif defined(HAVE_AESGCM)
    #include <wolfssl/wolfcrypt/aes.h>
    #include <wolfssl/wolfcrypt/wc_encrypt.h> /* AES IV sizes in FIPS mode */
    #define WOLFSSL_TICKET_KEY_SZ AES_256_KEY_SIZE
#endif

    typedef struct key_ctx {
        byte name[WOLFSSL_TICKET_NAME_SZ]; /* name for this context */
        byte key[WOLFSSL_TICKET_KEY_SZ];   /* cipher key */
    } key_ctx;

    static THREAD_LS_T key_ctx myKey_ctx;
    static THREAD_LS_T WC_RNG myKey_rng;

    static WC_INLINE int TicketInit(void)
    {
        int ret = wc_InitRng(&myKey_rng);
        if (ret != 0) return ret;

        ret = wc_RNG_GenerateBlock(&myKey_rng, myKey_ctx.key, sizeof(myKey_ctx.key));
        if (ret != 0) return ret;

        ret = wc_RNG_GenerateBlock(&myKey_rng, myKey_ctx.name,sizeof(myKey_ctx.name));
        if (ret != 0) return ret;

        return 0;
    }

    static WC_INLINE void TicketCleanup(void)
    {
        wc_FreeRng(&myKey_rng);
    }

    static WC_INLINE int myTicketEncCb(WOLFSSL* ssl,
                             byte key_name[WOLFSSL_TICKET_NAME_SZ],
                             byte iv[WOLFSSL_TICKET_IV_SZ],
                             byte mac[WOLFSSL_TICKET_MAC_SZ],
                             int enc, byte* ticket, int inLen, int* outLen,
                             void* userCtx)
    {
        int ret;
        word16 sLen = XHTONS(inLen);
        byte aad[WOLFSSL_TICKET_NAME_SZ + WOLFSSL_TICKET_IV_SZ + 2];
        int  aadSz = WOLFSSL_TICKET_NAME_SZ + WOLFSSL_TICKET_IV_SZ + 2;
        byte* tmp = aad;
    #if defined(HAVE_CHACHA) && defined(HAVE_POLY1305)
        /* chahca20/poly1305 */
    #elif defined(HAVE_AESGCM)
        Aes aes;
    #endif

        (void)ssl;
        (void)userCtx;

        /* encrypt */
        if (enc) {
            XMEMCPY(key_name, myKey_ctx.name, WOLFSSL_TICKET_NAME_SZ);

            ret = wc_RNG_GenerateBlock(&myKey_rng, iv, WOLFSSL_TICKET_IV_SZ);
            if (ret != 0) return WOLFSSL_TICKET_RET_REJECT;

            /* build aad from key name, iv, and length */
            XMEMCPY(tmp, key_name, WOLFSSL_TICKET_NAME_SZ);
            tmp += WOLFSSL_TICKET_NAME_SZ;
            XMEMCPY(tmp, iv, WOLFSSL_TICKET_IV_SZ);
            tmp += WOLFSSL_TICKET_IV_SZ;
            XMEMCPY(tmp, &sLen, sizeof(sLen));

        #if defined(HAVE_CHACHA) && defined(HAVE_POLY1305)
            ret = wc_ChaCha20Poly1305_Encrypt(myKey_ctx.key, iv,
                                              aad, aadSz,
                                              ticket, inLen,
                                              ticket,
                                              mac);
        #elif defined(HAVE_AESGCM)
            ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
            if (ret != 0) return WOLFSSL_TICKET_RET_REJECT;

            ret = wc_AesGcmSetKey(&aes, myKey_ctx.key, sizeof(myKey_ctx.key));
            if (ret == 0) {
                ret = wc_AesGcmEncrypt(&aes, ticket, ticket, inLen,
                                       iv, GCM_NONCE_MID_SZ, mac, AES_BLOCK_SIZE,
                                       aad, aadSz);
            }
            wc_AesFree(&aes);
        #endif
            
            if (ret != 0) return WOLFSSL_TICKET_RET_REJECT;
            *outLen = inLen;  /* no padding in this mode */
        }
        /* decrypt */
        else {
            /* see if we know this key */
            if (XMEMCMP(key_name, myKey_ctx.name, WOLFSSL_TICKET_NAME_SZ) != 0){
                printf("client presented unknown ticket key name %s\n", key_name);
                return WOLFSSL_TICKET_RET_FATAL;
            }

            /* build aad from key name, iv, and length */
            XMEMCPY(tmp, key_name, WOLFSSL_TICKET_NAME_SZ);
            tmp += WOLFSSL_TICKET_NAME_SZ;
            XMEMCPY(tmp, iv, WOLFSSL_TICKET_IV_SZ);
            tmp += WOLFSSL_TICKET_IV_SZ;
            XMEMCPY(tmp, &sLen, sizeof(sLen));

        #if defined(HAVE_CHACHA) && defined(HAVE_POLY1305)
            ret = wc_ChaCha20Poly1305_Decrypt(myKey_ctx.key, iv,
                                              aad, aadSz,
                                              ticket, inLen,
                                              mac,
                                              ticket);
        #elif defined(HAVE_AESGCM)
            ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
            if (ret != 0) return WOLFSSL_TICKET_RET_REJECT;

            ret = wc_AesGcmSetKey(&aes, myKey_ctx.key, sizeof(myKey_ctx.key));
            if (ret == 0) {
                ret = wc_AesGcmDecrypt(&aes, ticket, ticket, inLen,
                                        iv, GCM_NONCE_MID_SZ, mac, AES_BLOCK_SIZE,
                                        aad, aadSz);
            }
            wc_AesFree(&aes);
        #endif

            if (ret != 0) return WOLFSSL_TICKET_RET_REJECT;
            *outLen = inLen;  /* no padding in this mode */
        }

        return WOLFSSL_TICKET_RET_OK;
    }

#endif /* HAVE_SESSION_TICKET && ((HAVE_CHACHA && HAVE_POLY1305) || HAVE_AESGCM) */


static WC_INLINE word16 GetRandomPort(void)
{
    word16 port = 0;

    /* Generate random port for testing */
    WC_RNG rng;
    if (wc_InitRng(&rng) == 0) {
        if (wc_RNG_GenerateBlock(&rng, (byte*)&port, sizeof(port)) == 0) {
            port |= 0xC000; /* Make sure its in the 49152 - 65535 range */
        }
        wc_FreeRng(&rng);
    }
    (void)rng; /* for WC_NO_RNG case */
    return port;
}

#ifdef WOLFSSL_EARLY_DATA
static WC_INLINE void EarlyDataStatus(WOLFSSL* ssl)
{
    int earlyData_status;
#ifdef OPENSSL_EXTRA
    earlyData_status = SSL_get_early_data_status(ssl);
#else
    earlyData_status = wolfSSL_get_early_data_status(ssl);
#endif
    if (earlyData_status < 0) return;
    
    printf("Early Data was ");
    
    switch(earlyData_status) {
        case WOLFSSL_EARLY_DATA_NOT_SENT:
                printf("not sent.\n");
                break;
        case WOLFSSL_EARLY_DATA_REJECTED:
                printf("rejected.\n");
                break;
        case WOLFSSL_EARLY_DATA_ACCEPTED:
                printf("accepted\n");
                break;
        default:
                printf("unknown...\n");
    }
}
#endif /* WOLFSSL_EARLY_DATA */

#endif /* wolfSSL_TEST_H */
