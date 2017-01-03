/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <cstddef>

#include <boost/optional.hpp>

#include "io/socket.hpp"
#include "io/ssl_socket.hpp"

namespace mist
{
namespace io
{

class MistConnApi RdvSocket : public FileDescriptor
{
public:

  using connection_callback = std::function<void(std::shared_ptr<SSLSocket>)>;

private:

  SSLContext &_sslCtx;

  c_unique_ptr<PRFileDesc> _fd;

  connection_callback _cb;
  
  bool _open;

public:

  RdvSocket(SSLContext &sslCtx, c_unique_ptr<PRFileDesc> fd,
            connection_callback cb);

  IOContext &ioCtx();
  
  SSLContext &sslCtx();
  
  virtual PRFileDesc *fileDesc() override;

  virtual boost::optional<PRInt16> inFlags() const override;
  
  virtual void process(PRInt16 inFlags, PRInt16 outFlags) override;

};

} // namespace io
} // namespace mist
