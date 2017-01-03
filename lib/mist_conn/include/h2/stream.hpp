#ifndef __MIST_INCLUDE_H2_STREAM_HPP__
#define __MIST_INCLUDE_H2_STREAM_HPP__

#include <memory>

#include <boost/system/error_code.hpp>

#include "h2/types.hpp"

namespace mist
{
namespace h2
{

class Stream
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

#endif
