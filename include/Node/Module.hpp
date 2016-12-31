#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <map>
#include <memory>
#include <mutex>

#include <node.h>
#include <v8.h>
#include <nan.h>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include "conn/conn.hpp"
#include "conn/peer.hpp"
#include "conn/service.hpp"

#include "h2/client_stream.hpp"
#include "h2/client_request.hpp"
#include "h2/client_response.hpp"
#include "h2/server_stream.hpp"
#include "h2/server_request.hpp"
#include "h2/server_response.hpp"

#include "io/address.hpp"
#include "io/io_context.hpp"
#include "io/socket.hpp"
#include "io/ssl_context.hpp"

#include "Central.h"

#include "Node/Async.hpp"
#include "Node/Convert.hpp"
#include "Node/Wrap.hpp"

namespace Mist
{
namespace Node
{

extern v8::Isolate* isolate;

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
 
v8::Local<v8::Object>
require(v8::Local<v8::Object> module, const std::string& path);

} // namespace Node
} // namespace Mist
