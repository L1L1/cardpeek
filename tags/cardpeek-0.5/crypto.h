/**********************************************************************
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009 by 'L1L1'
*
* Cardpeek is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Cardpeek is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Cardpeek.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef CRYPTO_H
#define CRYPTO_H

#include "bytestring.h"

typedef enum {
  CRYPTO_OK,
  CRYPTO_ERROR_BAD_KEY_FORMAT,
  CRYPTO_ERROR_UNKNOWN_KEY_TYPE,
  CRYPTO_ERROR_BAD_CLEARTEXT_LENGTH,
  CRYPTO_ERROR_BAD_IV_LENGTH,
  CRYPTO_ERROR_UNKNOWN_PADDING_METHOD,
  CRYPTO_ERROR_UNKNOWN_ALGORITHM,

  CRYPTO_ERROR_UNKNOWN
} crypto_error_t;

typedef unsigned crypto_alg_t;

#define  CRYPTO_ALG_DES_ECB           (0x0000)
#define  CRYPTO_ALG_DES_CBC           (0x0001)
#define  CRYPTO_ALG_DES2_EDE_ECB      (0x0010)
#define  CRYPTO_ALG_DES2_EDE_CBC      (0x0011)
#define  CRYPTO_ALG_ISO9797_M3        (0x0021)
#define  CRYPTO_ALG_SHA1              (0x0030)

#define  CRYPTO_PAD_ZERO              (0x0000)
#define  CRYPTO_PAD_OPT_80_ZERO       (0x0100)
#define  CRYPTO_PAD_ISO9797_P2        (0x0200)


crypto_error_t crypto_create_context(bytestring_t *ctx, 
				     crypto_alg_t alg_type, 
				     const bytestring_t* key_bin);

crypto_error_t crypto_encrypt(bytestring_t* dst, 
			      const bytestring_t* ctx, 
			      const bytestring_t* src, 
			      const bytestring_t* iv);

crypto_error_t crypto_decrypt(bytestring_t* dst, 
			      const bytestring_t* ctx, 
			      const bytestring_t* src, 
			      const bytestring_t* iv);

crypto_error_t crypto_mac(bytestring_t* dst, 
			  const bytestring_t* ctx, 
			  const bytestring_t* src);

crypto_error_t crypto_digest(bytestring_t* dst,
			     const bytestring_t* ctx,
			     const bytestring_t* src);

const char *crypto_stringify_error(crypto_error_t err);
#endif

