/* ---------------------------------------------------------------------------
 * zlib_cdecl_bridge.c  (added to make the shipped prebuilt libs linkable)
 *
 * Why this file exists
 * --------------------
 * opensrc/libmobi/lib/libmobi.lib was built by its author against a *cdecl*
 * zlib, so its objects reference the plain C symbols  _crc32  and  _uncompress.
 * The zlib that ships with this project (opensrc/zlib/lib/zlibstat.lib) was
 * built with ZLIB_WINAPI, so it only exports the __stdcall decorations
 * _crc32@12 / _uncompress@16.  Those two names can never match, which is what
 * produces:
 *
 *   libmobi.lib(util.obj)       : error LNK2001: unresolved external symbol _uncompress
 *   libmobi.lib(encryption.obj) : error LNK2001: unresolved external symbol _crc32
 *
 * This translation unit supplies exactly those two cdecl symbols, implemented
 * with miniz (already vendored in opensrc/miniz), so the prebuilt libmobi,
 * which is also a /MD library, links cleanly into this /MT application.
 * ------------------------------------------------------------------------ */

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.c"

/* Z_OK / Z_DATA_ERROR, zlib status codes that libmobi checks */
#define BRIDGE_Z_OK 0
#define BRIDGE_Z_DATA_ERROR (-3)

/* cdecl, name must be exactly _crc32 */
unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned int len)
{
    return (unsigned long)mz_crc32((mz_ulong)crc, buf, (size_t)len);
}

/* cdecl, name must be exactly _uncompress */
int uncompress(unsigned char *dest, unsigned long *destLen,
               const unsigned char *source, unsigned long sourceLen)
{
    mz_ulong dst_len = destLen ? (mz_ulong)(*destLen) : 0;
    int ret = mz_uncompress(dest, &dst_len, source, (mz_ulong)sourceLen);

    if (destLen)
        *destLen = (unsigned long)dst_len;

    return (ret == MZ_OK) ? BRIDGE_Z_OK : BRIDGE_Z_DATA_ERROR;
}
