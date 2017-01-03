/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace mist
{
namespace crypto
{

class MistConnApi Hasher
{
public:
  virtual ~Hasher();
  virtual void update(const std::uint8_t* begin, const std::uint8_t* end) = 0;
  virtual std::vector<std::uint8_t> finalize() = 0;

  std::vector<std::uint8_t> finalize(const std::uint8_t* begin,
    const std::uint8_t* end);
};

MistConnApi std::unique_ptr<Hasher> hash_md5();

MistConnApi std::unique_ptr<Hasher> hash_sha3_256();

MistConnApi std::unique_ptr<Hasher> hash_sha3_384();

MistConnApi std::unique_ptr<Hasher> hash_sha3_512();

} // namespace crypto
} // namespace mist
