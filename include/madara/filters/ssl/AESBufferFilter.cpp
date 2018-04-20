
#ifdef  _USE_SSL_

#include <algorithm>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include "AESBufferFilter.h"

#include "madara/utility/Utility.h"
#include "madara/utility/LogMacros.h"

#include "madara/logger/GlobalLogger.h"

madara::filters::AESBufferFilter::AESBufferFilter ()
  : key_ (new unsigned char[32]), iv_ (new unsigned char[16])
{
  memset ((void *)key_.get_ptr (), 0, 32);
  memset ((void *)iv_.get_ptr (), 0, 16);
}

madara::filters::AESBufferFilter::AESBufferFilter (
  const AESBufferFilter & input)
  : key_ (new unsigned char[32]), iv_ (new unsigned char[16])
{
  memcpy (key_.get_ptr (), input.key_.get_ptr (), 32);
  memset ((void *)iv_.get_ptr (), 0, 16);
}

madara::filters::AESBufferFilter::AESBufferFilter (
  unsigned char * key, int key_length)
: key_ (new unsigned char[32]), iv_ (new unsigned char[16])
{
  memcpy (key_.get_ptr (), key, 32);
  memset ((void *)iv_.get_ptr (), 0, 16);
}

madara::filters::AESBufferFilter::~AESBufferFilter ()
{
}

int madara::filters::AESBufferFilter::generate_key (
  const std::string & password)
{
  int i, rounds = 5;

  // use a random salt and a zero'd initialization vector
  int64_t salt = 0;

  i = EVP_BytesToKey (EVP_aes_256_cbc (), EVP_sha1 (),
    (unsigned char *)&salt, (unsigned char *)password.c_str (),
    (int)password.length (), rounds,
    (unsigned char *)key_.get_ptr (),
    (unsigned char *)iv_.get_ptr ());

  if (i != 32)
  {
    madara_logger_ptr_log (logger::global_logger.get_ptr (), logger::LOG_ERROR,
      " Unable to initialize 256 bit AES. Only received %d bytes.\n",
      i);

    return -1;
  }

  return 0;
}


int
madara::filters::AESBufferFilter::encode (
  unsigned char * source, int size, int max_size) const
{
  int result = 0;

  /// encryption context
  EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new ();
  
  int len = 0;
  int ciphertext_len = 0;

  // size == plaintext_len
  size = std::min (size, max_size);

  // initialize the context
  
  EVP_CIPHER_CTX_init (ctx);

  // init the encryption with our key and IV
  result = EVP_EncryptInit_ex (ctx, EVP_aes_256_cbc (), NULL,
    key_.get_ptr (), iv_.get_ptr ());

  if (result != 1)
  {
    madara_logger_ptr_log (logger::global_logger.get_ptr (),
      logger::LOG_ERROR,
      "AESBufferFilter::encode: Cannot init key/iv. Result=%d.\n",
      result);

    return 0;
  }


  // encrypt the buffer
  result = EVP_EncryptUpdate (ctx,
    source, &len,
    source, size);

  if (result != 1)
  {
    madara_logger_ptr_log (logger::global_logger.get_ptr (),
      logger::LOG_ERROR,
      "AESBufferFilter::encode: Cannot perform encrypt. Result=%d.\n",
      result);

    return 0;
  }

  ciphertext_len = len;

  madara_logger_ptr_log (logger::global_logger.get_ptr (), logger::LOG_MINOR,
    "AESBufferFilter::encode:EVP_EncryptUpdate:2: len=%d, ciphertext_len=%d.\n",
    len, ciphertext_len);


  // finalize the encryption with padding
  result = EVP_EncryptFinal_ex (ctx, source + len, &len);

  if (result != 1)
  {
    madara_logger_ptr_log (logger::global_logger.get_ptr (),
      logger::LOG_ERROR,
      "AESBufferFilter::encode: Cannot finalize encrypt. Result=%d.\n",
      result);

    return 0;
  }

  ciphertext_len += len;

  madara_logger_ptr_log (logger::global_logger.get_ptr (), logger::LOG_MINOR,
    "AESBufferFilter::encode:EVP_EncryptFinal_ex: "
    "len=%d, ciphertext_len=%d.\n",
    len, ciphertext_len);

  EVP_CIPHER_CTX_free (ctx);

  return ciphertext_len;
}

int
madara::filters::AESBufferFilter::decode (
  unsigned char * source, int size, int max_size) const
{
  int result = 0;

  /// decryption context
  EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new ();

  int len (0);
  int plaintext_len (0);

  // size == cyphertext_len
  size = std::min (size, max_size);

  // initialize the context
  EVP_CIPHER_CTX_init (ctx);

  result = EVP_DecryptInit_ex (ctx, EVP_aes_256_cbc (), NULL,
    key_.get_ptr (), iv_.get_ptr ());

  if (result != 1)
  {
    madara_logger_ptr_log (logger::global_logger.get_ptr (),
      logger::LOG_ERROR,
      "AESBufferFilter::decode: Cannot init key/iv. Result=%d.\n",
      result);

    return 0;
  }

  madara_logger_ptr_log (logger::global_logger.get_ptr (), logger::LOG_MINOR,
    "AESBufferFilter::decode:EVP_DecryptUpdate:1: len=%d, size=%d.\n",
    len, size);

  result = EVP_DecryptUpdate (ctx, source, &len,
    source, size);
  plaintext_len = len;

  if (result != 1)
  {
    madara_logger_ptr_log (logger::global_logger.get_ptr (),
      logger::LOG_ERROR,
      "AESBufferFilter::decode: Cannot perform decrypt. Result=%d.\n",
      result);

    return 0;
  }

  madara_logger_ptr_log (logger::global_logger.get_ptr (), logger::LOG_MINOR,
    "AESBufferFilter::decode:EVP_DecryptUpdate:2: len=%d, plaintext_len=%d.\n",
    len, plaintext_len);

  result = EVP_DecryptFinal_ex (ctx, source + len, &len);

  if (result != 1)
  {
    madara_logger_ptr_log (logger::global_logger.get_ptr (),
      logger::LOG_ERROR,
      "AESBufferFilter::decode: Cannot finalize decrypt. Result=%d.\n",
      result);

    return 0;
  }

  plaintext_len += len;

  madara_logger_ptr_log (logger::global_logger.get_ptr (), logger::LOG_MINOR,
    "AESBufferFilter::decode:EVP_DecryptFinal_ex: "
    "len=%d, plaintext_len=%d.\n",
    len, plaintext_len);

  EVP_CIPHER_CTX_free (ctx);

  return plaintext_len;
}

#endif // _USE_SSL_
