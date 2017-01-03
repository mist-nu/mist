/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <boost/optional.hpp>

#include <prtypes.h>
#include <prio.h>

namespace mist
{
namespace io
{

class MistConnApi FileDescriptor
{
public:

  virtual PRFileDesc *fileDesc() = 0;
  
  virtual boost::optional<PRInt16> inFlags() const = 0;
  
  virtual void process(PRInt16 inFlags, PRInt16 outFlags) = 0;

};

} // namespace io
} // namespace mist
