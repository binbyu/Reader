#pragma once

#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBCONV_EXPORTS
#define CONV_API(type) __declspec(dllexport) type __stdcall
#else
#ifdef LIBCONV_STATIC
#define CONV_API(type) type __stdcall
#else
#define CONV_API(type) __declspec(dllimport) type __stdcall
#endif
#endif

    /**
     * @brief Check if the given data is valid UTF-8 encoded text
     *
     * @param src Pointer to the input data buffer
     * @param len Length of the input data in bytes, or -1 for null-terminated string
     * @return 1 if the data is valid UTF-8, 0 otherwise
     */
    CONV_API(int) conv_is_utf8(const char* src, int len);

    /**
     * @brief Convert text from its detected encoding to UTF-8
     *
     * @param src Pointer to the input text buffer
     * @param len Length of input text in bytes, or -1 for null-terminated string
     * @param dst Pointer to receive the allocated UTF-8 string
     * @return Number of bytes written to dst (excluding null terminator), or 0 on failure
     * @note The caller must call conv_free() on dst when done
     */
    CONV_API(int) conv_to_utf8(const char* src, int len, char** dst);

    /**
     * @brief Convert text from its detected encoding to system default ANSI code page
     *
     * @param src Pointer to the input text buffer
     * @param len Length of input text in bytes, or -1 for null-terminated string
     * @param dst Pointer to receive the allocated ANSI string
     * @return Number of bytes written to dst (excluding null terminator), or 0 on failure
     * @note The caller must call conv_free() on dst when done
     * @note Characters not representable in the target ANSI code page may be replaced
     */
    CONV_API(int) conv_to_ansi(const char* src, int len, char** dst);

    /**
     * @brief Convert text from its detected encoding to UTF-16 (Little Endian)
     *
     * @param src Pointer to the input text buffer
     * @param len Length of input text in bytes, or -1 for null-terminated string
     * @param dst Pointer to receive the allocated UTF-16 string
     * @return Number of wide characters written to dst (excluding null terminator), or 0 on failure
     * @note The caller must call conv_free() on dst when done
     * @note The output is always in Little Endian format regardless of system endianness
     */
    CONV_API(int) conv_to_utf16(const char* src, int len, wchar_t** dst);

    /**
     * @brief Free memory allocated by conversion functions
     *
     * @param data Pointer to memory allocated by conversion functions, or NULL
     * @note Do not use this function on memory not allocated by this library
     */
    CONV_API(void) conv_free(void* data);



    /**
     * @brief Convert UTF-8 text to UTF-16 (Little Endian)
     *
     * @param src Pointer to UTF-8 input text buffer
     * @param len Length of input text in bytes, or -1 for null-terminated string
     * @param dst Pointer to receive the allocated UTF-16 string
     * @return Number of wide characters written to dst (excluding null terminator)
     */
    CONV_API(int) conv_utf8_to_utf16(const char* src, int len, wchar_t** dst);

    /**
     * @brief Convert UTF-8 text to system default ANSI code page
     *
     * @param src Pointer to UTF-8 input text buffer
     * @param len Length of input text in bytes, or -1 for null-terminated string
     * @param dst Pointer to receive the allocated ANSI string
     * @return Number of bytes written to dst (excluding null terminator)
     */
    CONV_API(int) conv_utf8_to_ansi(const char* src, int len, char** dst);

    /**
     * @brief Convert ANSI text to UTF-16 (Little Endian)
     *
     * @param src Pointer to ANSI input text buffer
     * @param len Length of input text in bytes, or -1 for null-terminated string
     * @param dst Pointer to receive the allocated UTF-16 string
     * @return Number of wide characters written to dst (excluding null terminator)
     */
    CONV_API(int) conv_ansi_to_utf16(const char* src, int len, wchar_t** dst);

    /**
     * @brief Convert ANSI text to UTF-8
     *
     * @param src Pointer to ANSI input text buffer
     * @param len Length of input text in bytes, or -1 for null-terminated string
     * @param dst Pointer to receive the allocated UTF-8 string
     * @return Number of bytes written to dst (excluding null terminator)
     */
    CONV_API(int) conv_ansi_to_utf8(const char* src, int len, char** dst);

    /**
     * @brief Convert UTF-16 (Little Endian) text to UTF-8
     *
     * @param src Pointer to UTF-16 input text buffer
     * @param len Length of input text in wide characters, or -1 for null-terminated string
     * @param dst Pointer to receive the allocated UTF-8 string
     * @return Number of bytes written to dst (excluding null terminator)
     *
     * @note The len parameter is in wide characters, not bytes.
     *       If len = -1, the input string must be null-terminated.
     */
    CONV_API(int) conv_utf16_to_utf8(const wchar_t* src, int len, char** dst);

    /**
     * @brief Convert UTF-16 (Little Endian) text to system default ANSI code page
     *
     * @param src Pointer to UTF-16 input text buffer
     * @param len Length of input text in wide characters, or -1 for null-terminated string
     * @param dst Pointer to receive the allocated ANSI string
     * @return Number of bytes written to dst (excluding null terminator)
     *
     * @note The len parameter is in wide characters, not bytes.
     *       If len = -1, the input string must be null-terminated.
     */
    CONV_API(int) conv_utf16_to_ansi(const wchar_t* src, int len, char** dst);



    /**
     * @brief Convert UTF-8 to UTF-16 using shared buffer
     * @param src Null-terminated UTF-8 string
     * @return UTF-16 string (shared buffer), or NULL on failure
     * @note Result is valid only until next conversion call
     */
    CONV_API(wchar_t*) conv_shared_utf8_to_utf16(const char* src);

    /**
     * @brief Convert UTF-8 to ANSI using shared buffer
     * @param src Null-terminated UTF-8 string
     * @return ANSI string (shared buffer), or NULL on failure
     * @note Result is valid only until next conversion call
     */
    CONV_API(char*) conv_shared_utf8_to_ansi(const char* src);

    /**
     * @brief Convert ANSI to UTF-16 using shared buffer
     * @param src Null-terminated ANSI string
     * @return UTF-16 string (shared buffer), or NULL on failure
     * @note Result is valid only until next conversion call
     */
    CONV_API(wchar_t*) conv_shared_ansi_to_utf16(const char* src);

    /**
     * @brief Convert ANSI to UTF-8 using shared buffer
     * @param src Null-terminated ANSI string
     * @return UTF-8 string (shared buffer), or NULL on failure
     * @note Result is valid only until next conversion call
     */
    CONV_API(char*) conv_shared_ansi_to_utf8(const char* src);

    /**
     * @brief Convert UTF-16 to UTF-8 using shared buffer
     * @param src Null-terminated UTF-16 string
     * @return UTF-8 string (shared buffer), or NULL on failure
     * @note Result is valid only until next conversion call
     */
    CONV_API(char*) conv_shared_utf16_to_utf8(const wchar_t* src);

    /**
     * @brief Convert UTF-16 to ANSI using shared buffer
     * @param src Null-terminated UTF-16 string
     * @return ANSI string (shared buffer), or NULL on failure
     * @note Result is valid only until next conversion call
     */
    CONV_API(char*) conv_shared_utf16_to_ansi(const wchar_t* src);

    /**
     * @brief Clear the shared buffer manually
     */
    CONV_API(void) conv_shared_clear(void);

#ifdef __cplusplus
}
#endif