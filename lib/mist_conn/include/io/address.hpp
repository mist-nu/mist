/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <cstddef>
#include <string>
#include <memory>

namespace mist
{
namespace io
{

class AddressImpl;

class MistConnApi Address
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

  std::string toString() const;
  std::unique_ptr<AddressImpl> _impl;
};

} // namespace io
} // namespace mist
