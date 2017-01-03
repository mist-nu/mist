/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <string>
#include <memory>
#include <vector>

#include "io/io_context.hpp"

namespace mist
{
namespace io
{

class SSLSocket;
class SSLContextImpl;

class MistConnApi SSLContext
{
public:

  using connection_callback = std::function<void(std::shared_ptr<SSLSocket>)>;

  void loadPKCS12(const std::string& data, const std::string& password);

  void loadPKCS12File(const std::string& path, const std::string& password);

  void serve(std::uint16_t servPort, connection_callback cb);
  std::uint16_t serve(port_range_list servPort, connection_callback cb);

  std::vector<std::uint8_t> derPublicKey() const;

  std::vector<std::uint8_t> publicKeyHash() const;

  std::shared_ptr<SSLSocket> openSocket();

  std::vector<std::uint8_t> sign(const std::uint8_t* hashData,
    std::size_t length) const;

  bool verify(const std::vector<std::uint8_t>& derPublicKey,
    const std::uint8_t* hashData, std::size_t hashLength,
    const std::uint8_t* signData, std::size_t signLength) const;

  IOContext& ioCtx();

  SSLContext(IOContext& ioCtx, const std::string& dbdir);
  SSLContext(IOContext& ioCtx);
  ~SSLContext();

  std::shared_ptr<SSLContextImpl> _impl;

};

} // namespace io
} // namespace mist
