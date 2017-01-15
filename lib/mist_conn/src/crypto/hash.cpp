/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include <sechash.h>

#include "crypto/hash.hpp"
#include "crypto/sha3.h"

#include "memory/nss.hpp"

namespace mist
{
namespace crypto
{

MistConnApi
Hasher::~Hasher() {}

MistConnApi
std::vector<std::uint8_t>
Hasher::finalize(const std::uint8_t* begin, const std::uint8_t* end)
{
  update(begin, end);
  return finalize();
}

namespace
{

class NssHasher : public Hasher
{
public:

  NssHasher(SECOidTag hashOIDTag)
  {
    HASH_HashType hashType = HASH_GetHashTypeByOidTag(hashOIDTag);
    _ctx = to_unique(HASH_Create(hashType));
    HASH_Begin(_ctx.get());
  }

  virtual void update(const std::uint8_t* begin,
    const std::uint8_t* end) override
  {
    HASH_Update(_ctx.get(),
      reinterpret_cast<const unsigned char *>(begin),
      static_cast<unsigned int>(end - begin));
  }

  virtual std::vector<std::uint8_t> finalize() override
  {
    auto length = HASH_ResultLenContext(_ctx.get());
    std::vector<std::uint8_t> digest(length);
    HASH_End(_ctx.get(),
      reinterpret_cast<unsigned char *>(digest.data()), &length,
      static_cast<unsigned int>(digest.size()));
    return digest;
  }

private:

  c_unique_ptr<HASHContext> _ctx;

};

class Sha3Hasher : public Hasher
{
public:

  Sha3Hasher(std::size_t digestBitCount)
    : _digestBitCount(digestBitCount)
  {
    if (digestBitCount == 256)
      sha3_Init256(&_c);
    else if (digestBitCount == 384)
      sha3_Init384(&_c);
    else if (digestBitCount == 512)
      sha3_Init512(&_c);
    else
      throw std::runtime_error("Invalid digest bit count");
  }

  virtual void update(const std::uint8_t* begin,
    const std::uint8_t* end) override
  {
    sha3_Update(&_c, begin, end - begin);
  }

  virtual std::vector<std::uint8_t> finalize() override
  {
    const char* digest
      = static_cast<const char*>(sha3_Finalize(&_c));
    return std::vector<std::uint8_t>(digest, digest + _digestBitCount / 8);
  }

private:

  sha3_context _c;
  std::size_t _digestBitCount;

};

std::unique_ptr<Hasher> hash_sha3(std::size_t digestBitCount)
{
  return std::unique_ptr<Hasher>(new Sha3Hasher(digestBitCount));
}

std::unique_ptr<Hasher> hash_nss(SECOidTag hashOIDTag)
{
  return std::unique_ptr<Hasher>(new NssHasher(hashOIDTag));
}

} // namespace

MistConnApi
std::unique_ptr<Hasher> hash_md5()
{
  return hash_nss(SEC_OID_MD5);
}

MistConnApi
std::unique_ptr<Hasher> hash_sha3_256()
{
  return hash_sha3(256);
}

MistConnApi
std::unique_ptr<Hasher> hash_sha3_384()
{
  return hash_sha3(384);
}

MistConnApi
std::unique_ptr<Hasher> hash_sha3_512()
{
  return hash_sha3(512);
}

} // namespace crypto
} // namespace mist
