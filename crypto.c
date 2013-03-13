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

#include "crypto.h"
#include <openssl/des.h>
#include <string.h>
#include "bytestring.h"
#include <openssl/sha.h>


#define DES_KS_SIZE sizeof(DES_key_schedule)
#define ALG_TYPE(x) (x&0x00FF)
#define PAD_TYPE(x) (x&0xFF00)
#define KS_HEADER_SIZE 2

crypto_error_t crypto_create_context(bytestring_t *ctx, 
			   	     crypto_alg_t alg_type, 
			   	     const bytestring_t* key_bin)
{
  DES_key_schedule ks;

  bytestring_clear(ctx);
  bytestring_pushback(ctx,ALG_TYPE(alg_type));
  bytestring_pushback(ctx,PAD_TYPE(alg_type)>>8);

  switch (ALG_TYPE(alg_type)) {
    case CRYPTO_ALG_DES_ECB:
    case CRYPTO_ALG_DES_CBC:
      if (key_bin==NULL || bytestring_get_size(key_bin)!=8 || key_bin->width!=8)
	return CRYPTO_ERROR_BAD_KEY_FORMAT;

      DES_set_key_unchecked((const_DES_cblock *)key_bin->data,&ks);
      bytestring_append_data(ctx,DES_KS_SIZE,(unsigned char *)&ks);

      break;
    
    case CRYPTO_ALG_DES2_EDE_ECB:
    case CRYPTO_ALG_DES2_EDE_CBC:
    case CRYPTO_ALG_ISO9797_M3:

      if (key_bin==NULL || bytestring_get_size(key_bin)!=16 || key_bin->width!=8)
	return CRYPTO_ERROR_BAD_KEY_FORMAT;

      DES_set_key_unchecked((const_DES_cblock *)key_bin->data,&ks);
      bytestring_append_data(ctx,DES_KS_SIZE,(unsigned char *)&ks);

      DES_set_key_unchecked((const_DES_cblock *)(key_bin->data+8),&ks);
      bytestring_append_data(ctx,DES_KS_SIZE,(unsigned char *)&ks);

      break;
    case CRYPTO_ALG_SHA1:
      /* nothing else to do */
      break;
    default:
      return CRYPTO_ERROR_UNKNOWN_KEY_TYPE;
  }
  return CRYPTO_OK;
}

static crypto_error_t crypto_cipher(bytestring_t* dst, 
      			     const bytestring_t* ctx,
			     const bytestring_t* src, 
			     const bytestring_t* iv,
			     int enc)
{
  unsigned u;
  unsigned char alg;

  if (bytestring_get_element(&alg,ctx,0)!=BYTESTRING_OK)
    return CRYPTO_ERROR_UNKNOWN;

  if ((bytestring_get_size(src)&0x7)!=0)
    return CRYPTO_ERROR_BAD_CLEARTEXT_LENGTH;

  switch (alg) {
    case CRYPTO_ALG_DES_ECB:
      bytestring_resize(dst,bytestring_get_size(src));



      for (u=0;u<bytestring_get_size(src)/8;u++) 
	DES_ecb_encrypt((const_DES_cblock *)(src->data+u*8),
			(DES_cblock *)(dst->data+u*8),
			(DES_key_schedule *)(ctx->data+KS_HEADER_SIZE),
			enc);
      break;
    case CRYPTO_ALG_DES_CBC:
      if (iv==NULL || bytestring_get_size(iv)!=8)
	return CRYPTO_ERROR_BAD_IV_LENGTH;

      bytestring_resize(dst,bytestring_get_size(src));

      DES_ncbc_encrypt(src->data,
		       dst->data,
		       bytestring_get_size(src),
		       (DES_key_schedule *)(ctx->data+KS_HEADER_SIZE),
		       (DES_cblock *)(iv->data),
		       enc);
      break;
  case CRYPTO_ALG_DES2_EDE_ECB:
      bytestring_resize(dst,bytestring_get_size(src));

      for (u=0;u<bytestring_get_size(src)/8;u++)
        DES_ecb2_encrypt((const_DES_cblock *)(src->data+u*8),
		  	 (DES_cblock *)(dst->data+u*8),
		   	 (DES_key_schedule *)(ctx->data+KS_HEADER_SIZE),
			 (DES_key_schedule *)(ctx->data+KS_HEADER_SIZE+DES_KS_SIZE),
		    	 enc);
      break;
  case CRYPTO_ALG_DES2_EDE_CBC:
      if (iv==NULL || bytestring_get_size(iv)!=8)
	return CRYPTO_ERROR_BAD_IV_LENGTH;

      bytestring_resize(dst,bytestring_get_size(src));

      DES_ede2_cbc_encrypt(src->data,
	    		   dst->data,
			   bytestring_get_size(src),
			   (DES_key_schedule *)(ctx->data+KS_HEADER_SIZE),
			   (DES_key_schedule *)(ctx->data+KS_HEADER_SIZE+DES_KS_SIZE),
		    	   (DES_cblock *)(iv->data),
			   enc);
      break;
  default:
      return CRYPTO_ERROR_UNKNOWN_ALGORITHM;
  }
  return CRYPTO_OK;
}

static crypto_error_t crypto_pad(bytestring_t* dst, const bytestring_t* ctx,const bytestring_t* src)
{
  unsigned char e;
  
  bytestring_copy(dst,src);

  bytestring_get_element(&e,ctx,1);
  switch (((unsigned)e)<<8) {
    case CRYPTO_PAD_OPT_80_ZERO:
      if ((bytestring_get_size(dst)&0x7)==0)
	break;
    case CRYPTO_PAD_ISO9797_P2:
      bytestring_pushback(dst,0x80);
      bytestring_pad_right(dst,8,0);
      break;
     case CRYPTO_PAD_ZERO:
      bytestring_pad_right(dst,8,0);
      break;
   default:
      bytestring_clear(dst);
      return CRYPTO_ERROR_UNKNOWN_PADDING_METHOD;
  }
  return CRYPTO_OK;
}

crypto_error_t crypto_encrypt(bytestring_t* dst,
                              const bytestring_t* ctx,
                              const bytestring_t* src,
                              const bytestring_t* iv)
{
  unsigned pad_type;
  bytestring_t *padded_src;
  crypto_error_t retval;

  bytestring_get_element((unsigned char *)&pad_type,ctx,1);

  if (pad_type==0 && (bytestring_get_size(src)&0x7)==0)
    return crypto_cipher(dst,ctx,src,iv,DES_ENCRYPT);

  padded_src=bytestring_new(8);
  retval = crypto_pad(padded_src,ctx,src);
  if (retval==CRYPTO_OK)
  {
    retval = crypto_cipher(dst,ctx,padded_src,iv,DES_ENCRYPT);
  }
  bytestring_free(padded_src);
  return retval;
}


crypto_error_t crypto_decrypt(bytestring_t* dst, 
			      const bytestring_t* ctx,
			      const bytestring_t* src, 
			      const bytestring_t* iv)
{
  return crypto_cipher(dst,ctx,src,iv,DES_DECRYPT);
}

crypto_error_t crypto_mac(bytestring_t* dst, 
			  const bytestring_t* ctx,
			  const bytestring_t* src)
{
  unsigned u,v;
  unsigned char tmp[8];
  unsigned char clr[8];
  unsigned char alg;
  crypto_error_t retval;
  bytestring_t *padded_src;

  bytestring_get_element(&alg,ctx,0);

  if (alg==CRYPTO_ALG_ISO9797_M3)
  {
    padded_src = bytestring_new(8);
    retval = crypto_pad(padded_src,ctx,src);
    if (retval!=CRYPTO_OK) 
    {
      bytestring_free(padded_src);
      return retval;
    }
    memset(tmp,0,8);
    for (u=0;u<bytestring_get_size(padded_src)/8;u++)
    {
      for (v=0;v<8;v++) clr[v]=padded_src->data[u*8+v]^tmp[v];
      DES_ecb_encrypt((const_DES_cblock *)clr,
		      (DES_cblock *)tmp,
		      (DES_key_schedule *)(ctx->data+KS_HEADER_SIZE),
		      DES_ENCRYPT);
    }
    memcpy(clr,tmp,8);
    DES_ecb_encrypt((const_DES_cblock *)clr,
                      (DES_cblock *)tmp,
		      (DES_key_schedule *)(ctx->data+KS_HEADER_SIZE+DES_KS_SIZE),
		      DES_DECRYPT);
    memcpy(clr,tmp,8);
    DES_ecb_encrypt((const_DES_cblock *)clr,
  		    (DES_cblock *)tmp,
		    (DES_key_schedule *)(ctx->data+KS_HEADER_SIZE),
		    DES_ENCRYPT);
    bytestring_assign_data(dst,8,tmp);
    bytestring_free(padded_src);
  }
  else
    return CRYPTO_ERROR_UNKNOWN_ALGORITHM;
  return CRYPTO_OK;
}


crypto_error_t crypto_digest(bytestring_t* dst,
                             const bytestring_t* ctx,
                             const bytestring_t* src)
{
  unsigned char alg;

  bytestring_get_element(&alg,ctx,0);

  if (alg==CRYPTO_ALG_SHA1)
  {
    bytestring_resize(dst,SHA_DIGEST_LENGTH);
    SHA1(src->data,bytestring_get_size(src),dst->data);
  }
  else
    return CRYPTO_ERROR_UNKNOWN_ALGORITHM;
  return CRYPTO_OK;
}

const char *CRYTPO_ERROR_STRING[] = {
  "OK",
  "Bad key format",
  "Unknown key type",
  "Incorrect cleartext length",
  "Incorrect IV length",
  "Unknown padding method",
  "Unknown algorithm",
  "Unknown error"
};
  

const char *crypto_stringify_error(crypto_error_t err)
{
  if (err<CRYPTO_ERROR_UNKNOWN)
    return CRYTPO_ERROR_STRING[err];
  return CRYTPO_ERROR_STRING[CRYPTO_ERROR_UNKNOWN];
}

#ifdef TEST_CRYPTO_C
/* 
   Quick test vector:
   SRC             = 5468652071756663 "The quic"
   K1              = 0123456789ABCDEF
   DES-ENC(K1,SRC) = A28E91724C4BBA31
*/

int main()
{
  bytestring_t *key = bytestring_new_from_string("0123456789ABCDEFh");
  bytestring_t *ctx = bytestring_new(8); 
  bytestring_t *src = bytestring_new_from_string("5468652071756663h");
  bytestring_t *dst = bytestring_new(8);
  bytestring_t *clr = bytestring_new(8);

  if (crypto_create_key(ctx,0,key)==CRYPTO_OK)
  {
    fprintf(stderr,"Create key succeeded\n");
  }
  else
  {
    fprintf(stderr,"Create key failed\n");
    return -3;
  }

  if (crypto_encrypt(dst,ctx,src,NULL)==CRYPTO_OK)
  {
    fprintf(stderr,"Crypto: %s\n",bytestring_to_alloc_string(dst));
  }
  else
    fprintf(stderr,"Crypto failed\n");

  if (crypto_decrypt(clr,ctx,dst,NULL)==CRYPTO_OK)
  {
    if (bytestring_is_equal(clr,src))
      fprintf(stderr,"Decrypted clear text matches initial text\n");
    else
      fprintf(stderr,"Crypto: %s\n",bytestring_to_alloc_string(clr));
  }
  else
    fprintf(stderr,"Crypto failed\n");

  return 0;
}
#endif
