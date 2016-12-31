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

  ServerStreamWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  ServerStreamWrap(mist::h2::ServerStream _self)
    : NodeWrapSingleton(_self) {}

  static v8::Local<v8::FunctionTemplate> Init()
  {
    v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

    Nan::SetPrototypeMethod(tpl, "request",
      Method<&ServerStreamWrap::request>);
    Nan::SetPrototypeMethod(tpl, "response",
      Method<&ServerStreamWrap::response>);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

    return tpl;
  }

private:

  void request(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void response(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

class ServerRequestWrap
  : public NodeWrapSingleton<ServerRequestWrap, mist::h2::ServerRequest>
{
public:

  static const char *ClassName() { return "ServerRequest"; }

  ServerRequestWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  ServerRequestWrap(mist::h2::ServerRequest _self)
    : NodeWrapSingleton(_self) {}

  static v8::Local<v8::FunctionTemplate> Init()
  {
    v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

    Nan::SetPrototypeMethod(tpl, "setOnData",
      Method<&ServerRequestWrap::setOnData>);
    Nan::SetPrototypeMethod(tpl, "headers",
      Method<&ServerRequestWrap::headers>);
    Nan::SetPrototypeMethod(tpl, "stream",
      Method<&ServerRequestWrap::stream>);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

    return tpl;
  }

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

  ServerResponseWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  ServerResponseWrap(mist::h2::ServerResponse _self)
    : NodeWrapSingleton(_self),
    _inWrite(false), _lengthToWrite(0), _dataToWrite(nullptr)
  {
    using namespace std::placeholders;
    self().setOnRead(std::bind(&ServerResponseWrap::onRead, this, _1, _2, _3));
  }

  static v8::Local<v8::FunctionTemplate> Init()
  {
    v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

    Nan::SetPrototypeMethod(tpl, "_write",
      Method<&ServerResponseWrap::_write>);
    Nan::SetPrototypeMethod(tpl, "headers",
      Method<&ServerResponseWrap::headers>);
    Nan::SetPrototypeMethod(tpl, "stream",
      Method<&ServerResponseWrap::stream>);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

    return tpl;
  }

private:

  void _write(const Nan::FunctionCallbackInfo<v8::Value>& info);
  ssize_t onRead(std::uint8_t* data, std::size_t length, std::uint32_t* flags);
  void headers(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void stream(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
struct NodeValueConverter<const mist::h2::ServerRequest>
{
  static v8::Local<v8::Value> conv(const mist::h2::ServerRequest v)
  {
    return ServerRequestWrap::object(v);
  }
};

template<>
struct NodeValueConverter<const mist::h2::ServerResponse>
{
  static v8::Local<v8::Value> conv(const mist::h2::ServerResponse v)
  {
    return ServerResponseWrap::object(v);
  }
};

template<>
struct NodeValueConverter<const mist::h2::ServerStream>
{
  static v8::Local<v8::Value> conv(const mist::h2::ServerStream v)
  {
    return ServerStreamWrap::object(v);
  }
};

} // namespace detail

} // namespace Node
} // namespace Mist
