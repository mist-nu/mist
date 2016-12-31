/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#ifndef __MIST_SRC_IO_IO_CONTEXT_IMPL_HPP__
#define __MIST_SRC_IO_IO_CONTEXT_IMPL_HPP__

#include <cstddef>
#include <memory>
#include <list>
#include <mutex>

#include <prio.h>

#include <boost/optional.hpp>

#include "memory/nss.hpp"

#include "io/io_context.hpp"
#include "io/file_descriptor.hpp"
#include "io/tcp_socket.hpp"

namespace mist
{
namespace io
{

c_unique_ptr<PRFileDesc> openTCPSocket();

struct Timeout
{
  PRIntervalTime established;
  PRIntervalTime interval;
  std::function<void()> callback;

  Timeout(PRIntervalTime interval, std::function<void()> callback);
};

class IOContextImpl
{
public:

  using job_callback = IOContext::job_callback;

  /* Wait for one round of I/O events and process them
     Timeout in milliseconds */
  void ioStep(unsigned int maxTimeout);

  void exec();

  PRJob* queueJob(job_callback callback);

  void setTimeout(unsigned int interval, job_callback callback);

  void addDescriptor(std::shared_ptr<FileDescriptor> descriptor);

  void signal();

  std::shared_ptr<TCPSocket> openSocket();

  ~IOContextImpl();

private:

  friend class IOContext;

  IOContextImpl(IOContext& facade);

  IOContext& _facade;

  std::list<Timeout> _timeouts;
  std::recursive_mutex _timeoutMutex;

  std::list<std::shared_ptr<FileDescriptor>> _descriptors;
  std::recursive_mutex _descriptorMutex;

  c_unique_ptr<PRFileDesc> _signalEvent;

  c_unique_ptr<PRThreadPool> _threadPool;

  const std::size_t initialThreadCount = 4;
  const std::size_t maxThreadCount = 192;

};

} // namespace io
} // namespace mist

#endif
