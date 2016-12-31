/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#ifndef __MIST_INCLUDE_IO_IO_CONTEXT_HPP__
#define __MIST_INCLUDE_IO_IO_CONTEXT_HPP__

#include <functional>
#include <list>
#include <memory>

namespace mist
{
namespace io
{

using port_range = std::pair<std::uint16_t, std::uint16_t>;
using port_range_list = std::list<port_range>;

struct port_range_enumerator
{
public:

  port_range_enumerator()
    : _i0(0), _l({}), _i1(_l.begin()), _e1(_l.end())
  {}

  port_range_enumerator(port_range_list l)
    : _i0(0), _l(std::move(l)), _i1(_l.begin()), _e1(_l.end())
  {}

  std::uint16_t get() const {
    assert(!at_end());
    return _i0 + _i1->first;
  }

  void next() {
    assert(!at_end());
    ++_i0;
    if (_i0 == _i1->second) {
      _i0 = 0;
      ++_i1;
    }
  }

  bool at_end() const {
    return _i1 == _e1;
  }

private:

  std::uint16_t _i0;
  port_range_list _l;
  port_range_list::const_iterator _i1;
  port_range_list::const_iterator _e1;

};

class FileDescriptor;
class IOContextImpl;
class TCPSocket;

class IOContext
{
public:

  using job_callback = std::function<void()>;

  /* Wait for one round of I/O events and process them
     Timeout in milliseconds */
  void ioStep(unsigned int maxTimeout);

  void exec();

  void queueJob(job_callback callback);

  void signal();

  void setTimeout(unsigned int interval, job_callback callback);

  std::shared_ptr<TCPSocket> openSocket();

  void addDescriptor(std::shared_ptr<FileDescriptor> descriptor);

  IOContext();
  ~IOContext();

  std::unique_ptr<IOContextImpl> _impl;

};

} // namespace io
} // namespace mist

#endif
