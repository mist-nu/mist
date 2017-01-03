/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <cstddef>
#include <functional>

#include <boost/system/error_code.hpp>

namespace mist
{
namespace io
{

class MistConnApi Socket
{
public:

  /* Callback types */
  using write_callback = std::function<void(std::size_t,
    boost::system::error_code)>;
  using read_callback = std::function<void(const uint8_t *, std::size_t,
    boost::system::error_code)>;

  /* Read a fixed-length packet. */
  virtual void readOnce(std::size_t length, read_callback cb) = 0;

  /* Read indefinitely. */
  virtual void read(read_callback cb) = 0;

  /* Write. */
  virtual void write(const std::uint8_t *data, std::size_t length,
    write_callback cb = nullptr) = 0;

  /* Close the socket. */
  virtual void close(boost::system::error_code ec
    = boost::system::error_code()) = 0;
};

} // namespace io
} // namespace mist
