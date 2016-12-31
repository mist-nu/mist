/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#ifndef __MIST_INCLUDE_IO_ADDRESS_HPP__
#define __MIST_INCLUDE_IO_ADDRESS_HPP__

#include <cstddef>
#include <string>
#include <memory>

namespace mist
{
namespace io
{

class AddressImpl;

class Address
{
public:
  Address();
  Address(const Address&);
  Address(const AddressImpl&);
  Address& operator=(const Address&);
  ~Address();

  static Address fromLoopback(std::uint16_t port);
  static Address fromHostname(const std::string& hostname, std::uint16_t port);
  static Address fromIpAddr(const std::string& ip, std::uint16_t port);
  static Address fromAny(const std::string& str, std::uint16_t port);

  std::unique_ptr<AddressImpl> _impl;
};

} // namespace io
} // namespace mist

#endif
