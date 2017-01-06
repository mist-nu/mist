#ifndef __MIST_HEADERS_H2_SESSION_HPP__
#define __MIST_HEADERS_H2_SESSION_HPP__

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <list>
#include <ostream>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include <nghttp2/nghttp2.h>

#include "error/nghttp2.hpp"
#include "error/mist.hpp"

#include "io/ssl_socket.hpp"

#include "memory/nghttp2.hpp"

#include "h2/types.hpp"
#include "h2/session.hpp"

namespace mist
{
namespace h2
{

class StreamImpl;

class SessionImpl : public std::enable_shared_from_this<SessionImpl>
{
public:

  /* Shutdown the entire session */
  void shutdown();

  /* Start reading from the socket and write the first piece of data */
  void start();

  /* Stop the entire session */
  void stop();

  /* Returns true iff the session is stopped */
  bool isStopped() const;

  /* Set the on error callback */
  void setOnError(error_callback cb);

  boost::system::error_code submitRequest(StreamImpl& stream,
    const std::vector<nghttp2_nv>& nvs);

  boost::system::error_code submitResponse(StreamImpl& stream,
    const std::vector<nghttp2_nv>& nvs);

  boost::system::error_code submitTrailers(StreamImpl& stream,
    const std::vector<nghttp2_nv>& nvs);

  /* Resume the data generation for the given stream */
  boost::system::error_code resumeData(StreamImpl& stream);

  void setName(const std::string& name);

protected:

  SessionImpl(std::shared_ptr<io::Socket> socket, bool isServer);

  virtual ~SessionImpl();

  /* Called when an error was detected when communicating */
  void error(boost::system::error_code ec);

  /* Write a chunk of data to the socket */
  void write();

  /* Returns true iff there are reads or writes pending */
  bool alive() const;
  
  /* Returns the raw nghttp2 struct */
  nghttp2_session* nghttp2Session();

  boost::optional<std::shared_ptr<StreamImpl>>
    findBaseStream(std::int32_t streamId);

  /* Lookup a stream from its stream id */
  template<typename StreamT>
  boost::optional<std::shared_ptr<StreamT>>
    findStream(std::int32_t streamId)
  {
    auto s = findBaseStream(streamId);
    if (!s) return boost::none;
    return std::static_pointer_cast<StreamT>(*s);
  }

  void insertBaseStream(std::shared_ptr<StreamImpl> strm);

  /* Insert a new stream. The stream must have a stream id assigned. */
  template<typename StreamT>
  StreamT&
    insertStream(std::shared_ptr<StreamT> strm)
  {
    insertBaseStream(std::static_pointer_cast<StreamImpl>(strm));
    return *strm;
  }

  template <typename T>
  std::shared_ptr<T> shared_from_base()
  {
    return std::static_pointer_cast<T>(shared_from_this());
  }

private:

  /* Disable copy constructor and copy assign */
  SessionImpl(SessionImpl&) = delete;
  SessionImpl& operator=(SessionImpl&) = delete;

  /* nghttp2 session struct */
  c_unique_ptr<nghttp2_session> _h2session;

  /* nghttp2_session is not thread-safe; use this mutex when using it */
  mutable std::recursive_mutex _sessionMutex;

  /* Underlying socket */
  std::shared_ptr<io::Socket> _socket;

  /* Map of all streams in the session */
  using stream_map = std::map<std::int32_t, std::shared_ptr<StreamImpl>>;
  //using stream_map = std::map<std::int32_t, std::weak_ptr<StreamImpl>>;
  stream_map _streams;

  /* Boolean signifying that we are already sending data on the socket */
  bool _sending;

  /* Boolean signifying that communication has stopped for this session */
  bool _stopped;

  /* Boolean signifying that we are inside of a callback context and must
  * not re-trigger a write */
  bool _insideCallback;

  error_callback _onError;

  /* Called by the socket when data has been read; forwards to nghttp2 */
  void readCallback(const std::uint8_t* data, std::size_t length,
    boost::system::error_code ec);

  std::string _name;

  std::ostream& logStream() const;

protected:

  /* nghttp2 callbacks */
  virtual int onBeginHeaders(const nghttp2_frame* frame) = 0;

  virtual int onHeader(const nghttp2_frame* frame, const std::uint8_t* name,
    std::size_t namelen, const std::uint8_t* value, std::size_t valuelen,
    std::uint8_t flags) = 0;

  virtual int onFrameSend(const nghttp2_frame* frame) = 0;

  virtual int onFrameNotSend(const nghttp2_frame* frame, int errorCode) = 0;

  virtual int onFrameRecv(const nghttp2_frame* frame) = 0;

  virtual int onDataChunkRecv(std::uint8_t flags, std::int32_t stream_id,
    const std::uint8_t* data, std::size_t len) = 0;

  virtual int onStreamClose(std::int32_t stream_id,
    std::uint32_t error_code) = 0;

  virtual ssize_t onRead(std::int32_t stream_id, std::uint8_t* data,
    std::size_t length, std::uint32_t* flags) = 0;

};

} // namespace h2
} // namespace mist

#endif
