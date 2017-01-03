/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include "h2/types.hpp"

namespace mist
{
namespace h2
{

class MistConnApi Session
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
