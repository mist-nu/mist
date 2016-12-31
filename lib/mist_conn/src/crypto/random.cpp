/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#include "pk11pub.h"
#include "nss.h"

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include "error/nss.hpp"
#include "error/mist.hpp"

namespace mist
{
namespace crypto
{

std::uint64_t getRandomUInt53()
{
  std::uint64_t rand = 0;
  if (PK11_GenerateRandom(reinterpret_cast<unsigned char*>(&rand), 4)
      != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(), "Unable to generate random number"));
  return rand % 0x20000000000000;
}
  
double getRandomDouble53()
{
  return static_cast<double>(getRandomUInt53());
}
  
} // namespace crypto
} // namespace mist
