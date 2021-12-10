/* x509.h
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* x509.h for openssl */

#ifndef WOLFSSL_OPENSSL_509_H_
#define WOLFSSL_OPENSSL_509_H_

#include <wolfssl/openssl/ssl.h>
#include <wolfssl/openssl/crypto.h>
#include <wolfssl/openssl/dh.h>
#include <wolfssl/openssl/ec.h>
#include <wolfssl/openssl/ecdsa.h>
#include <wolfssl/openssl/pkcs7.h>

/* wolfSSL_X509_print_ex flags */
#define X509_FLAG_COMPAT        (0UL)
#define X509_FLAG_NO_HEADER     (1UL << 0)
#define X509_FLAG_NO_VERSION    (1UL << 1)
#define X509_FLAG_NO_SERIAL     (1UL << 2)
#define X509_FLAG_NO_SIGNAME    (1UL << 3)
#define X509_FLAG_NO_ISSUER     (1UL << 4)
#define X509_FLAG_NO_VALIDITY   (1UL << 5)
#define X509_FLAG_NO_SUBJECT    (1UL << 6)
#define X509_FLAG_NO_PUBKEY     (1UL << 7)
#define X509_FLAG_NO_EXTENSIONS (1UL << 8)
#define X509_FLAG_NO_SIGDUMP    (1UL << 9)
#define X509_FLAG_NO_AUX        (1UL << 10)
#define X509_FLAG_NO_ATTRIBUTES (1UL << 11)
#define X509_FLAG_NO_IDS        (1UL << 12)

#define XN_FLAG_FN_SN           0
#define XN_FLAG_ONELINE         0
#define XN_FLAG_COMPAT          0
#define XN_FLAG_RFC2253         1
#define XN_FLAG_SEP_COMMA_PLUS  (1 << 16)
#define XN_FLAG_SEP_CPLUS_SPC   (2 << 16)
#define XN_FLAG_SEP_SPLUS_SPC   (3 << 16)
#define XN_FLAG_SEP_MULTILINE   (4 << 16)
#define XN_FLAG_SEP_MASK        (0xF << 16)
#define XN_FLAG_DN_REV          (1 << 20)
#define XN_FLAG_FN_LN           (1 << 21)
#define XN_FLAG_FN_OID          (2 << 21)
#define XN_FLAG_FN_NONE         (3 << 21)
#define XN_FLAG_FN_MASK         (3 << 21)
#define XN_FLAG_SPC_EQ          (1 << 23)
#define XN_FLAG_DUMP_UNKNOWN_FIELDS (1 << 24)
#define XN_FLAG_FN_ALIGN        (1 << 25)

#define XN_FLAG_MULTILINE       0xFFFF

#define X509_EXTENSION_set_critical wolfSSL_X509_EXTENSION_set_critical
#define X509_EXTENSION_set_object   wolfSSL_X509_EXTENSION_set_object
#define X509_EXTENSION_set_data     wolfSSL_X509_EXTENSION_set_data

#endif /* WOLFSSL_OPENSSL_509_H_ */
