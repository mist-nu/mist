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
 * ServerStream, ServerRequest, ServerResponse
 *****************************************************************************/

class ServerStreamWrap
  : public NodeWrapSingleton<ServerStreamWrap, mist::h2::ServerStream>
{
public:

  static const char *ClassName() { return "ServerStream"; }

  ServerStreamWrap(mist::h2::ServerStream _self);

  static void Init(v8::Local<v8::Object> target);

private:

  void request(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void response(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

class ServerRequestWrap
  : public NodeWrapSingleton<ServerRequestWrap, mist::h2::ServerRequest>
{
public:

  static const char *ClassName() { return "ServerRequest"; }

  ServerRequestWrap(mist::h2::ServerRequest _self);

  static void Init(v8::Local<v8::Object> target);

private:

  void setOnData(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void headers(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void stream(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

class ServerResponseWrap
  : public NodeWrapSingleton<ServerResponseWrap, mist::h2::ServerResponse>
{
private:

  bool _inWrite;
  std::size_t _lengthToWrite;
  const char* _dataToWrite;
  CopyablePersistent<v8::Function> _callback;

public:

  static const char* ClassName() { return "ServerResponse"; }

  ServerResponseWrap(mist::h2::ServerResponse _self);

  static void Init(v8::Local<v8::Object> target);

private:

  void _write(const Nan::FunctionCallbackInfo<v8::Value>& info);
  mist::h2::generator_callback::result_type
    onRead(std::uint8_t* data, std::size_t length);
  void headers(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void stream(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
struct NodeValueConverter<const mist::h2::ServerRequest>
{
  static inline v8::Local<v8::Value> conv(const mist::h2::ServerRequest v)
  {
    return ServerRequestWrap::object(v);
  }
};

template<>
struct NodeValueConverter<const mist::h2::ServerResponse>
{
  static inline v8::Local<v8::Value> conv(const mist::h2::ServerResponse v)
  {
    return ServerResponseWrap::object(v);
  }
};

template<>
struct NodeValueConverter<const mist::h2::ServerStream>
{
  static inline v8::Local<v8::Value> conv(const mist::h2::ServerStream v)
  {
    return ServerStreamWrap::object(v);
  }
};

} // namespace detail

} // namespace Node
} // namespace Mist
