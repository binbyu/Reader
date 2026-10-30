#pragma once
#include "cJSON.h"
#include "conv.h"
#include <string>
#include <vector>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// basic types
#define CJSON_BASE_TYPE_NUMBER(T) \
    inline void to_json(cJSON* j, const char* k, const T& v) { \
        if (!j) return; \
        if (k) cJSON_AddNumberToObject(j, k, (double)v); \
        else { cJSON_SetNumberValue(j, (double)v); j->type = cJSON_Number; } \
    } \
    inline void from_json(const cJSON* j, const char* k, T& v) { \
        if (!j) return; \
        if (k) j = cJSON_GetObjectItem(j, k); \
        if (j && cJSON_IsNumber(j)) v = (T)j->valuedouble; \
    }

#define CJSON_BASE_TYPE_BOOL(T) \
    inline void to_json(cJSON* j, const char* k, const T& v) { \
        if (!j) return; \
        if (k) cJSON_AddBoolToObject(j, k, (cJSON_bool)v); \
        else { cJSON_SetBoolValue(j, (cJSON_bool)v); j->type = (v ? cJSON_True : cJSON_False); } \
    } \
    inline void from_json(const cJSON* j, const char* k, T& v) { \
        if (!j) return; \
        if (k) j = cJSON_GetObjectItem(j, k); \
        if (j && cJSON_IsBool(j)) v = (T)cJSON_IsTrue(j); \
    }

#define CJSON_BASE_TYPE_INT(T) \
    inline void to_json(cJSON* j, const char* k, const T& v) { \
        if (!j) return; \
        if (k) cJSON_AddNumberToObject(j, k, (int)v); \
        else { cJSON_SetNumberValue(j, (double)v); j->type = cJSON_Number; } \
    } \
    inline void from_json(const cJSON* j, const char* k, T& v) { \
        if (!j) return; \
        if (k) j = cJSON_GetObjectItem(j, k); \
        if (j && cJSON_IsNumber(j)) v = (T)j->valueint; \
    }

CJSON_BASE_TYPE_BOOL(bool)
CJSON_BASE_TYPE_NUMBER(char)
CJSON_BASE_TYPE_NUMBER(short)
CJSON_BASE_TYPE_NUMBER(int)
CJSON_BASE_TYPE_NUMBER(long)
CJSON_BASE_TYPE_NUMBER(long long)
CJSON_BASE_TYPE_NUMBER(unsigned char)
CJSON_BASE_TYPE_NUMBER(unsigned short)
CJSON_BASE_TYPE_NUMBER(unsigned int)
CJSON_BASE_TYPE_NUMBER(unsigned long)
CJSON_BASE_TYPE_NUMBER(unsigned long long)
CJSON_BASE_TYPE_NUMBER(float)
CJSON_BASE_TYPE_NUMBER(double)

inline void to_json(cJSON* j, const char* k, const char* v)
{
    if (!j) return;
    if (k) cJSON_AddStringToObject(j, k, v);
    else { cJSON_SetValuestring(j, v); j->type = cJSON_String; }
}
inline void from_json(const cJSON* j, const char* k, char*& v)
{
    if (!j) return;
    if (k) j = cJSON_GetObjectItem(j, k);
    if (j && cJSON_IsString(j)) strcpy(v, j->valuestring);
}
inline void to_json(cJSON* j, const char* k, const std::string& v)
{
    to_json(j, k, v.c_str());
}
inline void from_json(const cJSON* j, const char* k, std::string& v)
{
    if (!j) return;
    if (k) j = cJSON_GetObjectItem(j, k);
    if (j && cJSON_IsString(j)) v = j->valuestring;
}
inline void to_json(cJSON* j, const char* k, const std::wstring& v)
{
    std::string s;
    char* t = nullptr;
    conv_utf16_to_utf8(v.c_str(), (int)v.size(), &t);
    if (t)
    {
        s = t;
        conv_free(t);
    }
    to_json(j, k, s);
}
inline void from_json(const cJSON* j, const char* k, std::wstring& v)
{
    if (!j) return;
    if (k) j = cJSON_GetObjectItem(j, k);
    if (j && cJSON_IsString(j))
    {
        wchar_t* t = nullptr;
        conv_utf8_to_utf16(j->valuestring, -1, &t);
        if (t)
        {
            v = t;
            conv_free(t);
        }
    }
}

template<std::size_t N>
inline void to_json(cJSON* j, const char* k, const char(&v)[N])
{
    to_json(j, k, std::string(v, N - 1));
}

template<std::size_t N>
inline void from_json(const cJSON* j, const char* k, char(&v)[N])
{
    if (!j) return;
    if (k) j = cJSON_GetObjectItem(j, k);
    if (j && cJSON_IsString(j))
    {
        strncpy(v, j->valuestring, N - 1);
        v[N - 1] = '\0';
    }
}

template<std::size_t N>
inline void to_json(cJSON* j, const char* k, const wchar_t(&v)[N])
{
    to_json(j, k, std::wstring(v, N - 1));
}

template<std::size_t N>
inline void from_json(const cJSON* j, const char* k, wchar_t(&v)[N])
{
    if (!j) return;
    if (k) j = cJSON_GetObjectItem(j, k);
    if (j && cJSON_IsString(j))
    {
        wchar_t* t = nullptr;
        conv_utf8_to_utf16(j->valuestring, -1, &t);
        if (t)
        {
            wcsncpy(v, t, N - 1);
            v[N - 1] = L'\0';
            conv_free(t);
        }
        else
        {
            memset(v, 0, sizeof(wchar_t) * N);
        }
    }
}

template<typename T>
inline void to_json(cJSON* j, const char* k, const std::vector<T>& v)
{
    if (!j) return;
    cJSON* arr = j;
    if (k) arr = cJSON_CreateArray();
    if (arr && cJSON_IsArray(arr))
    {
        for (const T& i : v)
        {
            cJSON* item = cJSON_CreateObject();
            to_json(item, nullptr, i);
            cJSON_AddItemToArray(arr, item);
        }
        if (k) cJSON_AddItemToObject(j, k, arr);
    }
}

template<typename T>
inline void from_json(const cJSON* j, const char* k, std::vector<T>& v)
{
    if (!j) return;
    if (k) j = cJSON_GetObjectItem(j, k);
    if (j && cJSON_IsArray(j))
    {
        int n = cJSON_GetArraySize(j);
        v.resize(max(n, (int)v.size()));
        for (int i = 0; i < n; i++)
        {
            const cJSON* item = cJSON_GetArrayItem(j, i);
            from_json(item, nullptr, v[i]);
        }
    }
}

template<typename T, std::size_t N>
inline void to_json(cJSON* j, const char* k, const T(&v)[N])
{
    to_json(j, k, std::vector<T>(v, v + N));
}

template<typename T, std::size_t N>
inline void from_json(const cJSON* j, const char* k, T(&v)[N])
{
    if (!j) return;
    if (k) j = cJSON_GetObjectItem(j, k);
    if (j && cJSON_IsArray(j))
    {
        int n = min(cJSON_GetArraySize(j), N);
        for (int i = 0; i < n; i++)
        {
            const cJSON* item = cJSON_GetArrayItem(j, i);
            from_json(item, nullptr, v[i]);
        }
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// custom types
#define CJSON_EXPAND(x) x
#define CJSON_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, N, ...) N
#define CJSON_PASTE(...) CJSON_EXPAND(CJSON_GET_MACRO(__VA_ARGS__, \
        CJSON_PASTE32, \
        CJSON_PASTE31, \
        CJSON_PASTE30, \
        CJSON_PASTE29, \
        CJSON_PASTE28, \
        CJSON_PASTE27, \
        CJSON_PASTE26, \
        CJSON_PASTE25, \
        CJSON_PASTE24, \
        CJSON_PASTE23, \
        CJSON_PASTE22, \
        CJSON_PASTE21, \
        CJSON_PASTE20, \
        CJSON_PASTE19, \
        CJSON_PASTE18, \
        CJSON_PASTE17, \
        CJSON_PASTE16, \
        CJSON_PASTE15, \
        CJSON_PASTE14, \
        CJSON_PASTE13, \
        CJSON_PASTE12, \
        CJSON_PASTE11, \
        CJSON_PASTE10, \
        CJSON_PASTE9, \
        CJSON_PASTE8, \
        CJSON_PASTE7, \
        CJSON_PASTE6, \
        CJSON_PASTE5, \
        CJSON_PASTE4, \
        CJSON_PASTE3, \
        CJSON_PASTE2, \
        CJSON_PASTE1)(__VA_ARGS__))
#define CJSON_PASTE2(func, v1) func(v1)
#define CJSON_PASTE3(func, v1, v2) CJSON_PASTE2(func, v1) CJSON_PASTE2(func, v2)
#define CJSON_PASTE4(func, v1, v2, v3) CJSON_PASTE2(func, v1) CJSON_PASTE3(func, v2, v3)
#define CJSON_PASTE5(func, v1, v2, v3, v4) CJSON_PASTE2(func, v1) CJSON_PASTE4(func, v2, v3, v4)
#define CJSON_PASTE6(func, v1, v2, v3, v4, v5) CJSON_PASTE2(func, v1) CJSON_PASTE5(func, v2, v3, v4, v5)
#define CJSON_PASTE7(func, v1, v2, v3, v4, v5, v6) CJSON_PASTE2(func, v1) CJSON_PASTE6(func, v2, v3, v4, v5, v6)
#define CJSON_PASTE8(func, v1, v2, v3, v4, v5, v6, v7) CJSON_PASTE2(func, v1) CJSON_PASTE7(func, v2, v3, v4, v5, v6, v7)
#define CJSON_PASTE9(func, v1, v2, v3, v4, v5, v6, v7, v8) CJSON_PASTE2(func, v1) CJSON_PASTE8(func, v2, v3, v4, v5, v6, v7, v8)
#define CJSON_PASTE10(func, v1, v2, v3, v4, v5, v6, v7, v8, v9) CJSON_PASTE2(func, v1) CJSON_PASTE9(func, v2, v3, v4, v5, v6, v7, v8, v9)
#define CJSON_PASTE11(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) CJSON_PASTE2(func, v1) CJSON_PASTE10(func, v2, v3, v4, v5, v6, v7, v8, v9, v10)
#define CJSON_PASTE12(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11) CJSON_PASTE2(func, v1) CJSON_PASTE11(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11)
#define CJSON_PASTE13(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) CJSON_PASTE2(func, v1) CJSON_PASTE12(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12)
#define CJSON_PASTE14(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13) CJSON_PASTE2(func, v1) CJSON_PASTE13(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13)
#define CJSON_PASTE15(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) CJSON_PASTE2(func, v1) CJSON_PASTE14(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14)
#define CJSON_PASTE16(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15) CJSON_PASTE2(func, v1) CJSON_PASTE15(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15)
#define CJSON_PASTE17(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) CJSON_PASTE2(func, v1) CJSON_PASTE16(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16)
#define CJSON_PASTE18(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17) CJSON_PASTE2(func, v1) CJSON_PASTE17(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17)
#define CJSON_PASTE19(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18) CJSON_PASTE2(func, v1) CJSON_PASTE18(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18)
#define CJSON_PASTE20(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19) CJSON_PASTE2(func, v1) CJSON_PASTE19(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19)
#define CJSON_PASTE21(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20) CJSON_PASTE2(func, v1) CJSON_PASTE20(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20)
#define CJSON_PASTE22(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21) CJSON_PASTE2(func, v1) CJSON_PASTE21(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21)
#define CJSON_PASTE23(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22) CJSON_PASTE2(func, v1) CJSON_PASTE22(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22)
#define CJSON_PASTE24(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23) CJSON_PASTE2(func, v1) CJSON_PASTE23(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23)
#define CJSON_PASTE25(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24) CJSON_PASTE2(func, v1) CJSON_PASTE24(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24)
#define CJSON_PASTE26(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25) CJSON_PASTE2(func, v1) CJSON_PASTE25(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25)
#define CJSON_PASTE27(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26) CJSON_PASTE2(func, v1) CJSON_PASTE26(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26)
#define CJSON_PASTE28(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27) CJSON_PASTE2(func, v1) CJSON_PASTE27(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27)
#define CJSON_PASTE29(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28) CJSON_PASTE2(func, v1) CJSON_PASTE28(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28)
#define CJSON_PASTE30(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29) CJSON_PASTE2(func, v1) CJSON_PASTE29(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29)
#define CJSON_PASTE31(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30) CJSON_PASTE2(func, v1) CJSON_PASTE30(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30)
#define CJSON_PASTE32(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31) CJSON_PASTE2(func, v1) CJSON_PASTE31(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31)

#define CJSON_TO_JSON_FIELD(field) \
    to_json(J_, #field, V_.field);

#define CJSON_FROM_JSON_FIELD(field) \
    from_json(J_, #field, V_.field);

#define CJSON_DEFINE_TYPE(T, ...) \
    inline void to_json(cJSON* j, const char* k, const T& v) { \
        const T& V_ = v; \
        cJSON* J_ = j; \
        if (k) J_ = cJSON_CreateObject(); \
        if (!J_) return; \
        CJSON_EXPAND(CJSON_PASTE(CJSON_TO_JSON_FIELD, __VA_ARGS__)) \
        if (k) cJSON_AddItemToObject(j, k, J_); \
    } \
    inline void from_json(const cJSON* j, const char* k, T& v) { \
        T& V_ = v; \
        const cJSON* J_ = j; \
        if (k) J_ = cJSON_GetObjectItem(j, k); \
        if (J_ && cJSON_IsObject(J_)) { \
            CJSON_EXPAND(CJSON_PASTE(CJSON_FROM_JSON_FIELD, __VA_ARGS__)) \
        } \
    }

#define CJSON_DEFINE_MENU(E) CJSON_BASE_TYPE_INT(E)