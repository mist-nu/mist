/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "Node/Module.hpp"

#include "Node/CentralWrap.hpp"
#include "Node/ClientStreamWrap.hpp"
#include "Node/DatabaseWrap.hpp"
#include "Node/PeerWrap.hpp"
#include "Node/PrivateKeyWrap.hpp"
#include "Node/PublicKeyWrap.hpp"
#include "Node/ServerStreamWrap.hpp"
#include "Node/ServiceWrap.hpp"
#include "Node/SignatureWrap.hpp"
#include "Node/SocketWrap.hpp"
#include "Node/SHA3HasherWrap.hpp"
#include "Node/SHA3Wrap.hpp"
#include "Node/TransactionWrap.hpp"

namespace Mist
{
namespace Node
{

v8::Isolate* isolate = nullptr;

v8::Local<v8::Object>
require(v8::Local<v8::Object> module, const std::string& path)
{
  Nan::EscapableHandleScope scope;

  v8::Local<v8::Function> require
    = module->Get(conv("require")).As<v8::Function>();

  v8::Local<v8::Value> args[] = { conv(path) };

  return scope.Escape(require->Call(module, 1, args).As<v8::Object>());
}

void
Init(v8::Local<v8::Object> target, v8::Local<v8::Object> module)
{
  isolate = target->GetIsolate();

  Nan::HandleScope scope;

  CentralWrap::Init(target);
  ClientStreamWrap::Init(target);
  ClientRequestWrap::Init(target);
  ClientResponseWrap::Init(target);
  DatabaseWrap::Init(target);
  PeerWrap::Init(target);
  PrivateKeyWrap::Init(target);
  PublicKeyWrap::Init(target);
  ServerStreamWrap::Init(target);
  ServerRequestWrap::Init(target);
  ServerResponseWrap::Init(target);
  ServiceWrap::Init(target);
  SocketWrap::Init(target);
  SHA3HasherWrap::Init(target);
  SHA3Wrap::Init(target);
  SignatureWrap::Init(target);
  TransactionWrap::Init(target);
}

NODE_MODULE(_mist_conn, Init)

} // namespace Node
} // namespace Mist
