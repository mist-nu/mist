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

/*****************************************************************************
 * ClientRequest, ClientResponse, ClientStream
 *****************************************************************************/

class ClientStreamWrap
  : public NodeWrapSingleton<ClientStreamWrap, mist::h2::ClientStream>
{
public:

  static const char* ClassName() { return "ClientStream"; }

  ClientStreamWrap(mist::h2::ClientStream _self);

  static void Init(v8::Local<v8::Object> target);

private:

  void request(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void response(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

class ClientRequestWrap
  : public NodeWrapSingleton<ClientRequestWrap, mist::h2::ClientRequest>
{
private:

  bool _inWrite;
  std::size_t _lengthToWrite;
  const char* _dataToWrite;
  CopyablePersistent<v8::Function> _callback;

public:

  static const char* ClassName() { return "ClientRequest"; }

  ClientRequestWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  ClientRequestWrap(mist::h2::ClientRequest& _self);

  static void Init(v8::Local<v8::Object> target);

private:

  void _write(const Nan::FunctionCallbackInfo<v8::Value>& info);
  mist::h2::generator_callback::result_type
    onRead(std::uint8_t* data, std::size_t length);
  void setOnResponse(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void setOnPush(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void headers(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void stream(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void end(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

class ClientResponseWrap
  : public NodeWrapSingleton<ClientResponseWrap, mist::h2::ClientResponse>
{
public:

  ClientResponseWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  ClientResponseWrap(mist::h2::ClientResponse _self);

  static const char *ClassName() { return "ClientResponse"; }

  static void Init(v8::Local<v8::Object> target);

private:

  void setOnData(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void headers(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void stream(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
struct NodeValueConverter<const mist::h2::ClientStream>
{
  static inline v8::Local<v8::Value> conv(const mist::h2::ClientStream v)
  {
    return ClientStreamWrap::object(v);
  }
};

template<>
struct NodeValueConverter<const mist::h2::ClientRequest>
{
  static inline v8::Local<v8::Value> conv(const mist::h2::ClientRequest v)
  {
    return ClientRequestWrap::object(v);
  }
};

template<>
struct NodeValueConverter<const mist::h2::ClientResponse>
{
  static inline v8::Local<v8::Value> conv(const mist::h2::ClientResponse v)
  {
    return ClientResponseWrap::object(v);
  }
};

} // namespace detail

} // namespace Node
} // namespace Mist
