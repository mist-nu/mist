#ifndef __MIST_INCLUDE_IO_FILE_DESCRIPTOR_HPP__
#define __MIST_INCLUDE_IO_FILE_DESCRIPTOR_HPP__

#include <boost/optional.hpp>

#include <prtypes.h>
#include <prio.h>

namespace mist
{
namespace io
{

class FileDescriptor
{
public:

  virtual PRFileDesc *fileDesc() = 0;
  
  virtual boost::optional<PRInt16> inFlags() const = 0;
  
  virtual void process(PRInt16 inFlags, PRInt16 outFlags) = 0;

};

} // namespace io
} // namespace mist

#endif
