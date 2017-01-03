#ifndef __MIST_INCLUDE_H2_SESSION_HPP__
#define __MIST_INCLUDE_H2_SESSION_HPP__

#include "h2/types.hpp"

namespace mist
{
namespace h2
{

class Session
{
public:

  virtual ~Session();

  virtual void shutdown() = 0;

  virtual void start() = 0;

  virtual void stop() = 0;

  virtual bool isStopped() const = 0;

  virtual void setOnError(error_callback cb) = 0;

};

} // namespace h2
} // namespace mist

#endif
