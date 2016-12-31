/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "Node/Module.hpp"

namespace Mist
{
namespace Node
{

class PeerWrap : public NodeWrapSingleton<PeerWrap, mist::Peer&>
{
public:

  PeerWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  PeerWrap(mist::Peer& peer);

  static const char* ClassName() { return "Peer"; }

  static v8::Local<v8::FunctionTemplate> Init();

  void derPublicKey(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void publicKeyHash(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void setAuthenticated(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
struct NodeValueConverter<const mist::Peer::ConnectionStatus>
{
  static v8::Local<v8::Value> conv(const mist::Peer::ConnectionStatus v)
  {
    return Nan::New(static_cast<int>(v));
  }
};

template<>
struct NodeValueConverter<const mist::Peer*>
{
  static v8::Local<v8::Value> conv(const mist::Peer* v)
  {
    mist::Peer& ptr = *const_cast<mist::Peer*>(v);
    v8::Local<v8::Object> obj(PeerWrap::object(ptr));
    return obj;
  }
};

} // namespace detail
} // namespace Node
} // namespace Mist
