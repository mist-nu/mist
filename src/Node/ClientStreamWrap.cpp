/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "Node/ClientStreamWrap.hpp"

namespace Mist
{
namespace Node
{

ClientStreamWrap::ClientStreamWrap(mist::h2::ClientStream _self)
  : NodeWrapSingleton(_self) {}

void
ClientStreamWrap::Init(v8::Local<v8::Object> target)
{
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "request",
        Method<&ClientStreamWrap::request>);
  Nan::SetPrototypeMethod(tpl, "response",
        Method<&ClientStreamWrap::response>);

  auto func(Nan::GetFunction(tpl).ToLocalChecked());

  constructor().Reset(func);

  Nan::Set(target, Nan::New(ClassName()).ToLocalChecked(), func);
}

void
ClientStreamWrap::request(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  info.GetReturnValue().Set(ClientRequestWrap::object(self().request()));
}

void
ClientStreamWrap::response(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  info.GetReturnValue().Set(ClientResponseWrap::object(self().response()));
}

ClientRequestWrap::ClientRequestWrap(mist::h2::ClientRequest& _self)
  : NodeWrapSingleton(_self),
    _inWrite(false), _lengthToWrite(0), _dataToWrite(nullptr)
{
  using namespace std::placeholders;
  mist::h2::generator_callback::result_type
    onRead(std::uint8_t* data, std::size_t length);
}

void
ClientRequestWrap::Init(v8::Local<v8::Object> target)
{
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "setOnResponse",
    Method<&ClientRequestWrap::setOnResponse>);
  Nan::SetPrototypeMethod(tpl, "setOnPush",
    Method<&ClientRequestWrap::setOnPush>);
  Nan::SetPrototypeMethod(tpl, "headers",
    Method<&ClientRequestWrap::headers>);
  Nan::SetPrototypeMethod(tpl, "_write",
    Method<&ClientRequestWrap::_write>);
  Nan::SetPrototypeMethod(tpl, "stream",
    Method<&ClientRequestWrap::stream>);

  auto func(Nan::GetFunction(tpl).ToLocalChecked());

  constructor().Reset(func);

  Nan::Set(target, Nan::New(ClassName()).ToLocalChecked(), func);
}

void
ClientRequestWrap::_write(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  v8::Local<v8::Object> chunk = info[0].As<v8::Object>();
  std::string encoding = convBack<std::string>(info[1]);
  v8::Local<v8::Function> callback = info[2].As<v8::Function>();

  assert(node::Buffer::HasInstance(chunk));

  assert(!_dataToWrite);
  assert(!_lengthToWrite);

  _dataToWrite = node::Buffer::Data(chunk);
  _lengthToWrite = node::Buffer::Length(chunk);
  _callback.Reset(callback);

  self().stream().resume();
}

mist::h2::generator_callback::result_type
ClientRequestWrap::onRead(std::uint8_t* data, std::size_t length)
{
  if (_dataToWrite && !_lengthToWrite) {
    _dataToWrite = nullptr;
    asyncCall([=]()
    {
      Nan::HandleScope scope;
      Nan::Callback cb(Nan::New(_callback));
      cb();
    });
    return boost::none;
  }

  std::size_t actualLength = std::min(length, _lengthToWrite);
  std::copy(_dataToWrite, _dataToWrite + actualLength, data);
  _dataToWrite += actualLength;
  _lengthToWrite -= actualLength;
  return actualLength;
}

void
ClientRequestWrap::setOnResponse(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto func = v8::Local<v8::Function>::Cast(info[0]);

  self(info.Holder()).setOnResponse(
    makeAsyncCallback<mist::h2::ClientResponse>(func));
}

void
ClientRequestWrap::setOnPush(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto func = v8::Local<v8::Function>::Cast(info[0]);

  self(info.Holder()).setOnPush(
    makeAsyncCallback<mist::h2::ClientRequest>(func));
}

void
ClientRequestWrap::headers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  // TODO:
}

void
ClientRequestWrap::stream(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  info.GetReturnValue().Set(ClientStreamWrap::object(self().stream()));
}

ClientResponseWrap::ClientResponseWrap(mist::h2::ClientResponse _self)
  : NodeWrapSingleton(_self)
{
}

void
ClientResponseWrap::Init(v8::Local<v8::Object> target)
{
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "setOnData",
    Method<&ClientResponseWrap::setOnData>);
  Nan::SetPrototypeMethod(tpl, "stream",
    Method<&ClientResponseWrap::stream>);

  auto func(Nan::GetFunction(tpl).ToLocalChecked());

  constructor().Reset(func);

  Nan::Set(target, Nan::New(ClassName()).ToLocalChecked(), func);
}

void
ClientResponseWrap::setOnData(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  auto func = info[0].As<v8::Function>();
  std::function<void(v8::Local<v8::Function>, const std::uint8_t*,
    std::size_t)> onData
    = [](v8::Local<v8::Function> func, const std::uint8_t* data,
        std::size_t length)
  {
    Nan::HandleScope scope;
    Nan::Callback cb(func);
    v8::Local<v8::Value> args[] = {
      node::Buffer::Copy(isolate, reinterpret_cast<const char*>(data),
        length).ToLocalChecked()
    };
    cb(1, args);
  };
  self().setOnData(
    makeAsyncCallback<const std::uint8_t*, std::size_t>(func, onData));
}

void
ClientResponseWrap::headers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  // TODO:
}

void
ClientResponseWrap::stream(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  info.GetReturnValue().Set(ClientStreamWrap::object(self().stream()));
}

} // namespace Node
} // namespace Mist
