/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <sstream>
#include <vector>

#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/random/random_device.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include <prproces.h>

#include "error/mist.hpp"
#include "error/nss.hpp"
#include "io/file_descriptor.hpp"
#include "io/io_context.hpp"
#include "io/tcp_socket.hpp"
#include "memory/nss.hpp"
#include "tor/tor.hpp"
#include "io/address.hpp"

namespace mist
{
namespace tor
{

/*
 * TorHiddenService
 */
TorHiddenService::TorHiddenService(io::IOContext &ioCtx, TorController &ctrl,
  std::uint16_t port, std::string path)
  : _ioCtx(ioCtx), _ctrl(ctrl), _port(port), _path(path)
  {}

std::uint16_t
TorHiddenService::port() const
{
  return _port;
}

const std::string &
TorHiddenService::path() const
{
  return _path;
}

boost::optional<const std::string &>
TorHiddenService::tryGetOnionAddress()
{
  if (_onionAddress)
    return *_onionAddress;

  /* Try to read the hostname file */
  std::string hostnameFilename = path() + "/hostname";
  auto inFile = to_unique(PR_Open(hostnameFilename.c_str(), PR_RDONLY, 0));
  if (inFile) {
    std::array<std::uint8_t, 128> buf;
    std::size_t nread = 0;
    
    while (1) {
      auto n
        = PR_Read(inFile.get(), reinterpret_cast<void*>(buf.data() + nread),
            static_cast<PRInt32>(buf.size() - nread));
      if (n < 0) {
        return boost::none;
      } else if (n == 0) {
        std::string hostname;
        std::copy_if(buf.data(), buf.data() + nread,
                     std::back_inserter(hostname),
                     [](const char c){ return c != '\n' && c != '\r'; });
        _onionAddress = std::move(hostname);
        return *_onionAddress;
      } else {
        nread += n;
      }
    }
  }
  return boost::none;
}

void
TorHiddenService::onionAddress(onion_address_callback cb)
{
  auto addr = tryGetOnionAddress();
  if (addr) {
    cb(*addr);
  } else {
    _ioCtx.setTimeout(1000, std::bind(&TorHiddenService::onionAddress, this,
                                     std::move(cb)));
  }
}

namespace
{
boost::filesystem::path canonize(std::string s)
{
    std::string path(
        boost::filesystem::canonical(boost::filesystem::path(s)).string());
#if defined(_WIN32)||defined(_WIN64)
    std::replace(path.begin(), path.end(), '/', '\\');
#endif
    return boost::filesystem::path(path);
}
} // namespace

/*
 * TorController
 */
TorController::TorController(io::IOContext &ioCtx, std::string execName,
                             std::string workingDir)
  : _ioCtx(ioCtx), _execName(execName), _workingDir(canonize(workingDir)),
    _state(State::Stopped), _responseType(' ')
  {}

namespace
{

std::string
to_hex(uint8_t byte)
{
  static const char *digits = "0123456789abcdef";
  std::array<char, 2> text{ digits[byte >> 4], digits[byte & 0xf] };
  return std::string(text.begin(), text.end());
}

template<typename It>
std::string
to_hex(It begin, It end)
{
  std::string text;
  while (begin != end)
    text += to_hex(uint8_t(*(begin++)));
  return text;
}

std::string
to_hex(std::string str)
{
  return to_hex((uint8_t *)str.data(), (uint8_t *)(str.data() + str.size()));
}

std::string
generateRandomId(std::size_t numDwords)
{
  std::vector<std::uint32_t> out(numDwords);
  boost::random::random_device rng;
  rng.generate(out.begin(), out.end());
  return std::string(reinterpret_cast<const char *>(out.data()),
    4 * out.size());
}

std::string
generateRandomHexString(std::size_t numBytes)
{
  return to_hex(generateRandomId((numBytes + 3) / 4)).substr(0, 2 * numBytes);
}

std::string
readAll(PRFileDesc *fd)
{
  std::array<char, 80> buf;
  std::string out;
  while (1) {
    auto n = PR_Read(fd, buf.data(), static_cast<PRInt32>(buf.size()));
    if (n <= 0)
      break;
    out += std::string(buf.data(), buf.data() + n);
  }
  return out;
}

void
writeAll(PRFileDesc *fd, std::string contents)
{
  std::size_t nwritten = 0;
  while (1) {
    auto n = PR_Write(fd, contents.data() + nwritten,
      static_cast<PRInt32>(contents.length() - nwritten));
    if (n < 0) {
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to write to torrc file"));
    }
    else {
      nwritten += n;
      if (nwritten == contents.length())
        break;
    }
  }
}

bool
isCrlf(char c)
{
  return c == '\n' || c == '\r';
};

void
writeLaunchpadFile(const std::string& path)
{
  #if defined(_WIN32)||defined(_WIN64)
    std::string contents = "@%* > out.log 2>&1\r\n";
  #else
    std::string contents = "#!/bin/sh\n$* > out.log 2>&1\n";
  #endif

  auto launchpadFile = to_unique(PR_Open(path.c_str(),
    PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, PR_IRUSR | PR_IWUSR | PR_IXUSR));

  if (!launchpadFile)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to open launchpad file for writing"));

  writeAll(launchpadFile.get(), contents);

  if (PR_Sync(launchpadFile.get()) != PR_SUCCESS)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to sync launchpad file"));
}

} // namespace

void
TorController::runTorProcess(std::vector<std::string> processArgs,
  std::function<void(std::int32_t)> cb)
{
  auto workingDir(boost::filesystem::canonical(
    boost::filesystem::path(_workingDir)));

  _ioCtx.queueJob([=]() mutable
  {
    /* Due to difficulties with redirecting Tor's STDOUT/STDERR,
    we need to use a launchpad script to recover its output */
    std::string executable;
    {
#if defined(_WIN32)||defined(_WIN64)
      std::string launchpadName = "launchpad.cmd";
#else
      std::string launchpadName = "launchpad.sh";
#endif
      auto launchpadPath
        = workingDir / launchpadName;

      writeLaunchpadFile(launchpadPath.string());

      executable = launchpadPath.string();

      processArgs.insert(processArgs.begin(), _execName);
      processArgs.insert(processArgs.begin(), executable);
    }

    /* Convert arguments to argv format */
    std::vector<char *> argv;
    {
      for (auto &arg : processArgs) {
        argv.push_back(const_cast<char *>(arg.c_str()));
      }
      argv.push_back(nullptr);
    }

    /* Process attributes */
    auto attr = to_unique(PR_NewProcessAttr());
    {
      PR_ProcessAttrSetCurrentDirectory(attr.get(), _workingDir.string().c_str());
    }

    /* Create the process */
    {
      _torProcess = to_unique(PR_CreateProcess(executable.c_str(),
        argv.data(), nullptr, attr.get()));

      if (!_torProcess)
        BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
          "Unable to launch tor process"));
    }

    /* Wait for the process to finish */
    {
      PRInt32 exitCode = 0xCCCCCCCC;
      /* Release the process pointer here, PR_WaitProcess takes care of it */
      PR_WaitProcess(_torProcess.release(), &exitCode);
      cb(exitCode);
    }
  });
}

void
TorController::start(io::port_range_list socksPort,
  io::port_range_list ctrlPort, start_callback startCb, exit_callback exitCb)
{
  if (_state != State::Stopped) {
    fail(make_mist_error(MIST_ERR_ASSERTION));
    return;
  }

  _state = State::Launching;
  _password = generateRandomHexString(32);

  _socksPort = io::port_range_enumerator(socksPort);
  _ctrlPort = io::port_range_enumerator(ctrlPort);

  _startCb = std::move(startCb);
  _exitCb = std::move(exitCb);

  auto anchor = shared_from_this();

  /* Launch tor to create the password hash */
  runTorProcess({"--hash-password", _password, "--quiet"},
    [this, anchor, startCb](std::int32_t exitCode)
  {
    boost::filesystem::path workingDir(_workingDir);

    if (exitCode) {
      fail(make_mist_error(MIST_ERR_TOR_EXECUTION));
      return;
    }

    /* Read the password hash */
    auto logPath = workingDir / "out.log";
    {
      auto logFile = to_unique(PR_Open(logPath.string().c_str(),
        PR_RDONLY, 0));
      if (!logFile) {
        fail(make_mist_error(MIST_ERR_TOR_LOG_FILE));
        return;
      }
      std::string passwordHash = readAll(logFile.get());

      /* Remove CR/LF */
      passwordHash.erase(std::remove_if(passwordHash.begin(),
        passwordHash.end(), isCrlf), passwordHash.end());
        
      /* Make sure that the password hash was created */
      if (passwordHash.empty()) {
        fail(make_mist_error(MIST_ERR_TOR_LOG_FILE));
        return;
      }

      _passwordHash = passwordHash;

      attemptLaunch();
    }
  });
}

void
TorController::attemptLaunch()
{
  std::uint16_t socksPort = _socksPort.get();
  std::uint16_t ctrlPort = _ctrlPort.get();

  /* Construct the contents of the torrc file */
  std::string torrcContents;
  {
    auto torDataDir = _workingDir / "tordata";
    std::ostringstream buf;
    buf << "SocksPort " << socksPort << std::endl;
    buf << "ControlPort " << ctrlPort << std::endl;
    buf << "DataDirectory " << torDataDir.string() << std::endl;
    buf << "HashedControlPassword " << _passwordHash << std::endl;

    /* Bridge configuration */
    if (_bridges.size()) {
      buf << "UseBridges 1" << std::endl;
      for (auto &bridge : _bridges) {
        buf << "Bridge " << bridge << std::endl;
      }
    }

    for (auto &hiddenService : _hiddenServices) {
      buf << "HiddenServiceDir " << hiddenService.path() << std::endl;
      buf << "HiddenServicePort 443"
        << " 127.0.0.1:" << hiddenService.port() << std::endl;
    }

    torrcContents = buf.str();
  }

  /* Write contents to the torrc file */
  boost::filesystem::path torrcPath(_workingDir / "torrc");
  {
    auto rcFile = to_unique(PR_Open(torrcPath.string().c_str(),
      PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE, PR_IRUSR | PR_IWUSR));

    if (!rcFile) {
      fail(make_nss_error(MIST_ERR_TOR_GENERAL_FAILURE));
      return;
    }

    writeAll(rcFile.get(), torrcContents);

    if (PR_Sync(rcFile.get()) != PR_SUCCESS) {
      fail(make_nss_error(MIST_ERR_TOR_GENERAL_FAILURE));
      return;
    }
  }

  /* Launch Tor "for real" */
  {
    auto anchor(shared_from_this());

    runTorProcess({ "-f", torrcPath.string() },
      [anchor, this](std::int32_t exitCode)
    {
      std::lock_guard<std::recursive_mutex> lock(_mutex);

      /* Close the control socket */
      if (_ctrlSocket)
        _ctrlSocket->close();

      if (_state == Shutdown) {
        /* Tor exited normally */
        onExit(boost::system::error_code());
        _state = Stopped;
      } else {
        /* Tor exited unexpectedly */
        launchPostMortem(exitCode);
      }
    });
  }

  connectControlPort();

}

void
TorController::onStart()
{
  if (_startCb) {
    auto cb(_startCb);
    _startCb = nullptr;
    cb();
  }
}

void
TorController::onExit(boost::system::error_code ec)
{
  _startCb = nullptr;
  if (_exitCb) {
    auto cb(_exitCb);
    _exitCb = nullptr;
    cb(ec);
  }
}

namespace
{
boost::optional<std::uint16_t> findAlreadyInUsePort(const std::string& log)
{
  // [warn] Could not bind to $IP:$PORT: Address already in use
  // [WSAEADDRINUSE ]. Is Tor already running?
  std::size_t j(log.find(": Address already in use"));
  std::size_t i(j);
  if (i == std::string::npos) {
    return boost::none;
  }
  while (1) {
    if (!i || log[i - 1] == ':')
      break;
    --i;
  }
  std::string portStr = log.substr(i, j - i);
  try {
    return boost::lexical_cast<std::uint16_t>(portStr);
  } catch (const boost::bad_lexical_cast&) {
    return boost::none;
  }
}
} // namespace

void
TorController::launchPostMortem(std::int32_t exitCode)
{
  std::lock_guard<std::recursive_mutex> lock(_mutex);

  boost::filesystem::path workingDir(_workingDir);

  /* Read the log */
  auto logPath = workingDir / "out.log";
  std::string log;
  {
    auto logFile = to_unique(PR_Open(logPath.string().c_str(),
      PR_RDONLY, 0));
    if (!logFile) {
      /* Unable to read log file */
      fail(make_mist_error(MIST_ERR_TOR_LOG_FILE));
      return;
    }
    log = readAll(logFile.get());
  }

  /* Search the log for any hints of what made Tor fail; notably control port
  or SOCKS port in use in which case we try a new port */

  auto port = findAlreadyInUsePort(log);
  if (!port) {
    fail(make_mist_error(MIST_ERR_PORT_TAKEN));
    return;
  }

  if (port == _socksPort.get()) {
    _socksPort.next();
  } else if (port == _ctrlPort.get()) {
    _ctrlPort.next();
  } else {
    /* Unable to find any indication of a port error. Something else went
    wrong. */
    fail(make_mist_error(MIST_ERR_TOR_GENERAL_FAILURE));
    return;
  }

  _state = State::Launching;

  attemptLaunch();
}

void
TorController::sendCommand(std::string cmd)
{
  assert(_ctrlSocket);

  cmd += "\r\n";
  /* We need to keep the data alive throughout the write call. */
  auto buffer = std::make_shared<std::vector<uint8_t>>(
    reinterpret_cast<const std::uint8_t*>(cmd.data()),
    reinterpret_cast<const std::uint8_t*>(cmd.data() + cmd.length()));

  auto anchor(shared_from_this());
  auto socket(_ctrlSocket);

  _ctrlSocket->write(buffer->data(), buffer->size(),
    [anchor, buffer, socket, this]
  (std::size_t nwritten, boost::system::error_code ec)
  {
    if (socket != _ctrlSocket) {
      /* The control socket has changed; response not valid */
      return;
    }
 
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_state == State::Started) {
      if (ec) {
        /* TODO: Does an error here mean fail? Maybe we retried the connection, 
        in which case this write in particular failed, but it does not mean
        that this Tor session has failed as a whole! */
        fail(make_mist_error(MIST_ERR_TOR_GENERAL_FAILURE));
        return;
      }
    }
  });
}

void
TorController::readResponse(const std::uint8_t *data, std::size_t length,
  boost::system::error_code ec)
{
  if (ec) {
    /* Error reading response */
    if (_state == State::Relaunch || _state == State::Shutdown) {
      // Read error is expected here
      return;
    }

    fail(make_mist_error(MIST_ERR_TOR_GENERAL_FAILURE));
    return;
  }

  auto begin = data;
  auto end = data + length;

  while (1) {
    auto crlfPos = std::find_if(begin, end, isCrlf);
    _pendingResponse += std::string(begin, crlfPos);
  
    /* No CR/LF found; store the text for the next read */
    if (crlfPos == end)
      break;
  
    /* Non-empty line found */
    if (!_pendingResponse.empty()) {
      responseLineReady(std::move(_pendingResponse));
    }

    begin = std::next(crlfPos);
  }
}

void
TorController::responseLineReady(std::string response)
{
  std::lock_guard<std::recursive_mutex> lock(_mutex);

  if (_state == State::BadState)
    return;

  std::int16_t code;
  char lineType;
  {
    std::string codeStr;
    std::size_t sepIndex = response.find_first_of(" +-");
    if (sepIndex == std::string::npos) {
      /* No separator character found */
      _state = State::BadState;
      return;
    }
    codeStr = response.substr(0, sepIndex);
    lineType = response[sepIndex];
    response = response.substr(sepIndex + 1);
    try {
      code = boost::lexical_cast<std::int16_t>(codeStr);
    } catch (const boost::bad_lexical_cast&) {
      /* Unable to parse the response code */
      fail(make_mist_error(MIST_ERR_TOR_GENERAL_FAILURE));
      return;
    }
  }

  if (_responseType != ' ') {
    _lineCache.emplace_back(std::move(response));
    if (_responseType == '-' && lineType == '-') {
      /* Still a mid-response line; continue reading */
      return;
    } else if (_responseType == '+' && code != 1) {
      /* Wait for further response lines */
      return;
    }
    /* We expect nothing more; reset the _responseType */
    _responseType = ' ';
  } else {
    assert(_lineCache.empty());
    _lineCache.emplace_back(std::move(response));
    _responseCode = code;
    _responseType = lineType;
    if (_responseType != ' ') {
      /* There are response lines to follow */
      return;
    }
  }

  if (_responseCode / 100 == 6) {
    /* Asynchronous response */
    asynchronousResponse(_responseCode, std::move(_lineCache));
  } else {
    /* Synchronous response */
    synchronousResponse(_responseCode, std::move(_lineCache));
  }
}

void
TorController::fail(boost::system::error_code ec)
{
  std::lock_guard<std::recursive_mutex> lock(_mutex);
  bool starting(_state != State::Started);
  _state = State::BadState;
  onExit(ec);
}

void
TorController::synchronousResponse(std::int16_t code,
  std::vector<std::string> response)
{
  std::lock_guard<std::recursive_mutex> lock(_mutex);

  bool ok = code / 100 == 2;

  if (_state == State::Authenticating) {

    if (!ok) {
      /* Unable to authenticate */
      _state = State::Relaunch;
      _torProcess.reset();
      return;
    }

    _state = State::SetEvents;

    sendCommand("SETEVENTS CIRC STREAM ORCONN NOTICE WARN ERR");

  } else if (_state == State::SetEvents) {

    /* We accept "OK" or "Unrecognized event" */
    if (!ok && code != 552) {
      /* Unable to set events */
      fail(make_mist_error(MIST_ERR_TOR_GENERAL_FAILURE));
      return;
    }

    _state = State::Started;

  } else {

    /* Unexpected response */
    fail(make_mist_error(MIST_ERR_TOR_GENERAL_FAILURE));
    return;

  }
}

void
TorController::asynchronousResponse(std::int16_t code,
  std::vector<std::string> response)
{
  std::lock_guard<std::recursive_mutex> lock(_mutex);
  if (code == 650) {
    for (auto& line : response) {
      if (line.find("Bootstrapped 100") != std::string::npos) {
        // 650 NOTICE Bootstrapped 100 % : Done
        /* Here we know that Tor is up and running and happy with life */
        onStart();
      }
    }
  }
}

void
TorController::connectControlPort()
{
  std::lock_guard<std::recursive_mutex> lock(_mutex);

  _ctrlSocket = _ioCtx.openSocket();
  auto addr = io::Address::fromLoopback(_ctrlPort.get());

  /* Connect */
  auto anchor = shared_from_this();
  auto socket(_ctrlSocket);
  _ctrlSocket->connect(addr,
    [this, anchor, socket]
    (boost::system::error_code ec)
  {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    
    if (socket != _ctrlSocket) {
      /* The control socket has changed */
      return;
    }

    if (ec) {
      /* Unable to connect; retry in a second */
      _ioCtx.setTimeout(1000,
        std::bind(&TorController::connectControlPort, this));
      return;
    }

    _state = State::Authenticating;

    /* Set the socket read callback */
    {
      using namespace std::placeholders;
      _ctrlSocket->read(std::bind(&TorController::readResponse, this,
        _1, _2, _3));
    }

    /* Authenticate with our password */
    sendCommand("AUTHENTICATE \"" + _password + "\"");
  });
}

void
TorController::stop()
{
  // TODO: Use sendCommand to tell Tor to shutdown cleanly.
  _state = Shutdown;
  _torProcess.reset();
}

bool
TorController::isRunning() const
{
  return static_cast<bool>(_torProcess);
}

TorHiddenService&
TorController::addHiddenService(std::uint16_t port, std::string name)
{
  boost::filesystem::path workingDir(_workingDir);
  _hiddenServices.emplace_back(_ioCtx, *this, port,
    (workingDir / name).string());
  return *(--_hiddenServices.end());
}

namespace
{

using socks_callback
  = std::function<void(std::string, boost::system::error_code)>;

/* Try to perform a SOCKS5 handshake to connect to the given
   domain name and port. */
void
handshakeSOCKS5(mist::io::TCPSocket &sock,
  std::string hostname, std::uint16_t port,
  socks_callback cb)
{
  std::array<std::uint8_t, 3> socksReq;
  socksReq[0] = 5; /* Version */
  socksReq[1] = 1;
  socksReq[2] = 0;

  sock.writeCopy(socksReq.data(), socksReq.size());

  sock.readOnce(2,
    [=, &sock]
  (const std::uint8_t *data, std::size_t length,
    boost::system::error_code ec) mutable
  {
    if (ec) {
      cb("", ec);
      return;
    }
    if (length != 2 || data[0] != 5 || data[1] != 0) {
      cb("", make_mist_error(MIST_ERR_SOCKS_HANDSHAKE));
      return;
    }

    /* Construct the SOCKS5 connect request */
    std::vector<std::uint8_t> connReq(5 + hostname.length() + 2);
    {
      auto outIt = connReq.begin();
      *(outIt++) = 5; /* Version */
      *(outIt++) = 1; /* Connect */
      *(outIt++) = 0; /* Must be zero */
      *(outIt++) = 3; /* Resolve domain name */
      *(outIt++) = static_cast<std::uint8_t>(hostname.length()); /* Domain name length */
      outIt = std::copy(hostname.begin(), hostname.end(), outIt); /* Domain name */
      *(outIt++) = static_cast<std::uint8_t>((port >> 8) & 0xff); /* Port MSB */
      *(outIt++) = static_cast<std::uint8_t>(port & 0xff); /* Port LSB */
                                                            /* Make sure that we can count */
      assert(outIt == connReq.end());
    }

    sock.writeCopy(connReq.data(), connReq.size());

    /* Read 5 bytes; these are all the bytes we need to determine the
    final packet size */
    sock.readOnce(5,
      [=, &sock]
    (const std::uint8_t *data, std::size_t length,
      boost::system::error_code ec) mutable
    {
      if (ec) {
        cb("", ec);
        return;
      }
      if (length != 5 || data[0] != 5) {
        cb("", make_mist_error(MIST_ERR_SOCKS_HANDSHAKE));
        return;
      }

      bool success = data[1] == 0;
      std::uint8_t type = data[3];
      std::uint8_t firstByte = data[4];

      std::size_t complLength;
      if (type == 1)
        complLength = 10 - 5;
      else if (type == 3)
        complLength = 7 + firstByte - 5;
      else if (type == 4)
        complLength = 22 - 5;
      else {
        cb("", make_mist_error(MIST_ERR_SOCKS_HANDSHAKE));
        return;
      }

      sock.readOnce(complLength,
        [=, &sock]
      (const std::uint8_t *data, std::size_t length,
        boost::system::error_code ec) mutable
      {
        if (ec) {
          cb("", ec);
          return;
        }
        if (complLength != length) {
          cb("", make_mist_error(MIST_ERR_SOCKS_HANDSHAKE));
          return;
        }

        std::string address;
        if (type == 1)
          address = std::to_string(firstByte) + '.'
          + std::to_string(data[0]) + '.'
          + std::to_string(data[1]) + '.'
          + std::to_string(data[2]) + ':'
          + std::to_string((data[3] << 8) | data[4]);
        else if (type == 3)
          address = std::string(reinterpret_cast<const char*>(data),
            firstByte) + ':'
          + std::to_string((data[firstByte] << 8) | data[firstByte + 1]);
        else if (type == 4)
          address = to_hex(firstByte) + to_hex(data[0]) + ':'
          + to_hex(data[1]) + to_hex(data[2]) + ':'
          + to_hex(data[3]) + to_hex(data[4]) + ':'
          + to_hex(data[5]) + to_hex(data[6]) + ':'
          + to_hex(data[7]) + to_hex(data[8]) + ':'
          + to_hex(data[9]) + to_hex(data[10]) + ':'
          + to_hex(data[11]) + to_hex(data[12]) + ':'
          + to_hex(data[13]) + to_hex(data[14]) + ':'
          + std::to_string((data[15] << 8) | data[16]);
        else {
          cb("", make_mist_error(MIST_ERR_SOCKS_HANDSHAKE));
          return;
        }
        assert(address.length());
        if (!success) {
          cb(address, make_mist_error(MIST_ERR_SOCKS_HANDSHAKE));
        } else {
          cb(address, boost::system::error_code());
        }
      });
    });
  });
}

} // namespace

void
TorController::connect(io::TCPSocket & socket, std::string hostname,
  std::uint16_t port, connect_callback cb)
{
  if (_state != State::Started) {
    cb(make_mist_error(MIST_ERR_TOR_GENERAL_FAILURE));
    return;
  }

  /* Initialize addr to localhost:torPort */
  auto addr = io::Address::fromLoopback(_socksPort.get());

  auto anchor = shared_from_this();
  socket.connect(addr,
    [cb, anchor, hostname, port, &socket]
    (boost::system::error_code ec)
  {
    if (ec) {
      cb(ec);
      return;
    }
    handshakeSOCKS5(socket, hostname, port,
      [cb, anchor]
      (std::string address, boost::system::error_code ec)
    {
      cb(ec);
    });
  });
}

} // namespace tor
} // namespace mist

