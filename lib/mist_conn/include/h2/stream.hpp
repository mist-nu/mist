/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <memory>

#include <boost/system/error_code.hpp>

#include "h2/types.hpp"

namespace mist
{
namespace h2
{

class MistConnApi Stream
{
public:

  virtual ~Stream();

  virtual void setOnClose(close_callback cb) = 0;

  virtual void close(boost::system::error_code ec) = 0;

  virtual void resume() = 0;

  virtual boost::system::error_code submitTrailers(
    const header_map& trailers) = 0;

};

} // namespace h2
} // namespace mist
