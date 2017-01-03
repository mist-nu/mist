/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include "memory/nss.hpp"

#include "io/io_context.hpp"

struct PRProcess;

namespace mist
{

class SSLContext;

namespace tor
{

class TorController;

class TorHiddenService
{
public:

  using onion_address_callback = std::function<void(const std::string&)>;

  TorHiddenService(io::IOContext& ioCtx, TorController& ctrl,
    std::uint16_t port, std::string path);

  std::uint16_t port() const;
  const std::string& path() const;
  void onionAddress(onion_address_callback cb);

private:

  friend class TorController;

  io::IOContext& _ioCtx;
  TorController& _ctrl;

  std::uint16_t _port;
  std::string _path;
  boost::optional<std::string> _onionAddress;

  boost::optional<const std::string &> tryGetOnionAddress();

};

class TorController
  : public std::enable_shared_from_this<TorController>
{
public:

  TorController(io::IOContext &ioCtx, std::string execName,
    std::string workingDir);

  TorHiddenService& addHiddenService(std::uint16_t port,
    std::string name);

  using start_callback = std::function<void(boost::system::error_code)>;
  using exit_callback = std::function<void(boost::system::error_code)>;
  void start(io::port_range_list socksPort, io::port_range_list ctrlPort,
    start_callback startCb, exit_callback exitCb);

  void stop();

  void fail(boost::system::error_code ec = boost::system::error_code());

  bool isRunning() const;

  /* Connect the given socket to the given hostname and port using the
     Tor SOCKS proxy*/
  using connect_callback = std::function<void(boost::system::error_code)>;
  void connect(io::TCPSocket &socket, std::string hostname, std::uint16_t port,
    connect_callback cb);
  
private:

  std::recursive_mutex _mutex;

  enum State {
    Stopped,
    Launching,
    Authenticating,
    Relaunch,
    SetEvents,
    Started,
    Shutdown,
    BadState,
  } _state;

  io::IOContext &_ioCtx;
  std::string _execName;
  boost::filesystem::path _workingDir;

  io::port_range_enumerator _socksPort;
  io::port_range_enumerator _ctrlPort;

  start_callback _startCb;
  exit_callback _exitCb;

  std::string _password;
  std::string _passwordHash;

  std::list<TorHiddenService> _hiddenServices;

  c_unique_ptr<PRProcess> _torProcess;

  std::vector<std::string> _bridges;

  std::shared_ptr<io::TCPSocket> _ctrlSocket;
  std::string _pendingResponse;

  std::uint16_t _responseCode;
  std::vector<std::string> _lineCache;
  char _responseType;

  void runTorProcess(std::vector<std::string> processArgs,
    std::function<void(std::int32_t)> cb);

  void connectControlPort();

  void attemptLaunch();

  void onStart(boost::system::error_code ec, std::uint16_t socksPort = 0,
    std::uint16_t ctrlPort = 0);

  void onExit(boost::system::error_code ec);

  void launchPostMortem(std::int32_t exitCode);

  void sendCommand(std::string cmd);

  void readResponse(const std::uint8_t *data, std::size_t length,
    boost::system::error_code ec);

  void responseLineReady(std::string response);

  void synchronousResponse(std::int16_t code,
    std::vector<std::string> response);

  void asynchronousResponse(std::int16_t code,
    std::vector<std::string> response);

};

} // namespace tor
} // namespace mist
