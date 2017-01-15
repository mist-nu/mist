/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <cstddef>
#include <string>
#include <memory>

#include <boost/asio/ip/address.hpp>

#include "io/address.hpp"
#include "io/address_impl.hpp"

namespace mist
{
namespace io
{

/* Address */
Address::Address()
  : _impl()
{
}

Address::~Address()
{
}

Address::Address(const Address& other)
  : _impl(std::unique_ptr<AddressImpl>(new AddressImpl(*other._impl)))
{
}

Address::Address(const AddressImpl& impl)
  : _impl(std::unique_ptr<AddressImpl>(new AddressImpl(impl)))
{
}

Address&
Address::operator=(const Address& other)
{
  _impl = std::unique_ptr<AddressImpl>(new AddressImpl(*other._impl));
  return *this;
}

Address
Address::fromLoopback(std::uint16_t port)
{
  return Address(AddressImpl::fromLoopback(port));
}

Address
Address::fromHostname(const std::string& hostname, std::uint16_t port)
{
  return Address(AddressImpl::fromHostname(hostname, port));
}

Address
Address::fromIpAddr(const std::string& ip, std::uint16_t port)
{
    return Address(AddressImpl::fromIpAddr(ip, port));
}

Address
Address::fromAny(const std::string& str, std::uint16_t port)
{
    return Address(AddressImpl::fromAny(str, port));
}

std::string
Address::toString() const
{
    return _impl->toString();
}

/* AddressImpl */
AddressImpl::AddressImpl()
{
}

AddressImpl::AddressImpl(PRNetAddr* addr)
  : addr(*addr)
{
}

AddressImpl::AddressImpl(const AddressImpl& other)
  : addr(other.addr)
{
}

AddressImpl::~AddressImpl()
{
}

AddressImpl&
AddressImpl::operator=(const AddressImpl& other)
{
  addr = other.addr;
  return *this;
};

PRNetAddr*
AddressImpl::prAddr()
{
  return &addr;
}

const PRNetAddr*
AddressImpl::prAddr() const
{
  return &addr;
}

AddressImpl
AddressImpl::fromLoopback(std::uint16_t port)
{
  PRNetAddr prAddr;
  PR_InitializeNetAddr(PR_IpAddrLoopback, port, &prAddr);
  return AddressImpl(&prAddr);
};

namespace
{
boost::system::error_code
netAddrFromHostname(const std::string& hostname, std::uint16_t port,
  PRNetAddr* prAddr)
{
  auto addrInfo = to_unique(PR_GetAddrInfoByName(
    hostname.c_str(), PR_AF_UNSPEC, PR_AI_ADDRCONFIG));

  if (!addrInfo)
    return make_nss_error();

  {
    void* enumPtr = nullptr;
    enumPtr = PR_EnumerateAddrInfo(enumPtr, addrInfo.get(), port, prAddr);
  }

  return boost::system::error_code();
}
} // namespace

AddressImpl
AddressImpl::fromHostname(const std::string& hostname, std::uint16_t port)
{
  AddressImpl impl;
  if (netAddrFromHostname(hostname, port, &impl.addr))
    throw std::runtime_error("Unable to obtain IP from hostname");
  return AddressImpl(&impl.addr);
};

namespace
{
boost::system::error_code
netAddrFromIpAddress(const std::string& ipAddr, std::uint16_t port,
  PRNetAddr* prAddr)
{
  boost::system::error_code ec;
  auto parsedAddr = boost::asio::ip::address::from_string(ipAddr, ec);
  if (ec)
    return ec;
  if (parsedAddr.is_v4()) {
    boost::asio::ip::address_v4 addrV4(parsedAddr.to_v4());
    auto bytes(addrV4.to_bytes());
    prAddr->inet.family = PR_AF_INET;
    prAddr->inet.port = PR_htons(port);
    std::copy(bytes.begin(), bytes.end(),
      reinterpret_cast<unsigned char*>(&prAddr->inet.ip));
  } else {
    boost::asio::ip::address_v6 addrV6(parsedAddr.to_v6());
    auto bytes(addrV6.to_bytes());
    prAddr->ipv6.family = PR_AF_INET6;
    prAddr->ipv6.flowinfo = 0;
    prAddr->ipv6.scope_id = 0;
    prAddr->ipv6.port = PR_htons(port);
    std::copy(bytes.begin(), bytes.end(),
      reinterpret_cast<unsigned char*>(&prAddr->ipv6.ip));
  }
}
} // namespace

AddressImpl
AddressImpl::fromIpAddr(const std::string& ip, std::uint16_t port)
{
  AddressImpl impl;
  if (netAddrFromIpAddress(ip, port, &impl.addr))
    throw std::runtime_error("Unable to parse IP address");
  return impl;
};

AddressImpl
AddressImpl::fromAny(const std::string& str, std::uint16_t port)
{
  AddressImpl impl;
  if (netAddrFromIpAddress(str, port, &impl.addr))
    if (netAddrFromHostname(str, port, &impl.addr))
      throw std::runtime_error("Unable to obtain IP from address");
  return impl;
};

std::string
AddressImpl::toString() const
{
  if (addr.inet.family == AF_INET) {
      return  std::to_string(addr.inet.ip & 0xff)
          + '.' + std::to_string((addr.inet.ip >> 8) & 0xff)
          + '.' + std::to_string((addr.inet.ip >> 16) & 0xff)
          + '.' + std::to_string(addr.inet.ip >> 24)
          + ':' + std::to_string(PR_ntohs(addr.inet.port));
  } else {
      return std::string();
  }
}

} // namespace io
} // namespace mist
