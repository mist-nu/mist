#ifndef __MIST_INCLUDE_CRYPTO_HASH_HPP__
#define __MIST_INCLUDE_CRYPTO_HASH_HPP__

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace mist
{
namespace crypto
{

class Hasher
{
public:
  virtual ~Hasher();
  virtual void update(const std::uint8_t* begin, const std::uint8_t* end) = 0;
  virtual std::vector<std::uint8_t> finalize() = 0;

  std::vector<std::uint8_t> finalize(const std::uint8_t* begin,
    const std::uint8_t* end);
};

std::unique_ptr<Hasher> hash_md5();

std::unique_ptr<Hasher> hash_sha3_256();

std::unique_ptr<Hasher> hash_sha3_384();

std::unique_ptr<Hasher> hash_sha3_512();

} // namespace crypto
} // namespace mist

#endif
