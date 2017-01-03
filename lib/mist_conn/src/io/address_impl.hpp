#ifndef __MIST_SRC_IO_ADDRESS_IMPL_HPP__
#define __MIST_SRC_IO_ADDRESS_IMPL_HPP__

#include <cstddef>
#include <memory>
#include <string>

#include "prnetdb.h"

#include "memory/nss.hpp"
#include "error/nss.hpp"

namespace mist
{
namespace io
{

class AddressImpl
{
public:
  AddressImpl();
  AddressImpl(PRNetAddr* addr);
  AddressImpl(const AddressImpl&);
  AddressImpl& operator=(const AddressImpl&);
  ~AddressImpl();

  static AddressImpl fromLoopback(std::uint16_t port);
  static AddressImpl fromHostname(const std::string& hostname,
    std::uint16_t port);
  static AddressImpl fromIpAddr(const std::string& ip, std::uint16_t port);
  static AddressImpl fromAny(const std::string& str, std::uint16_t port);

  std::string toString() const;

  const PRNetAddr* prAddr() const;
  PRNetAddr* prAddr();

private:
  PRNetAddr addr;

};

} // namespace io
} // namespace mist

#endif
