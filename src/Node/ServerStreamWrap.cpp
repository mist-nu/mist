/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "Node/ServerStreamWrap.hpp"

namespace Mist
{
namespace Node
{

ServerStreamWrap::ServerStreamWrap(mist::h2::ServerStream _self)
  : NodeWrapSingleton(_self) {}

void
ServerStreamWrap::Init(v8::Local<v8::Object> target)
{
  Nan::HandleScope;

  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "request",
    Method<&ServerStreamWrap::request>);
  Nan::SetPrototypeMethod(tpl, "response",
    Method<&ServerStreamWrap::response>);

  auto func(Nan::GetFunction(tpl).ToLocalChecked());

  constructor().Reset(func);

  Nan::Set(target, Nan::New(ClassName()).ToLocalChecked(), func);
}

void
ServerStreamWrap::request(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  info.GetReturnValue().Set(ServerRequestWrap::object(self().request()));
}

void
ServerStreamWrap::response(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  info.GetReturnValue().Set(ServerResponseWrap::object(self().response()));
}

//
// ServerRequest
//

ServerRequestWrap::ServerRequestWrap(mist::h2::ServerRequest _self)
  : NodeWrapSingleton(_self) {}

void
ServerRequestWrap::Init(v8::Local<v8::Object> target)
{
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "setOnData",
    Method<&ServerRequestWrap::setOnData>);
  Nan::SetPrototypeMethod(tpl, "headers",
    Method<&ServerRequestWrap::headers>);
  Nan::SetPrototypeMethod(tpl, "stream",
    Method<&ServerRequestWrap::stream>);
    auto func(Nan::GetFunction(tpl).ToLocalChecked());

  constructor().Reset(func);

  Nan::Set(target, Nan::New(ClassName()).ToLocalChecked(), func);

}

void
ServerRequestWrap::setOnData(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

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
ServerRequestWrap::headers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  // TODO:
}

void
ServerRequestWrap::stream(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  info.GetReturnValue().Set(ServerStreamWrap::object(self().stream()));
}

//
// ServerResponse
//

ServerResponseWrap::ServerResponseWrap(mist::h2::ServerResponse _self)
  : NodeWrapSingleton(_self),
  _inWrite(false), _lengthToWrite(0), _dataToWrite(nullptr)
{
  using namespace std::placeholders;
  self().setOnRead(std::bind(&ServerResponseWrap::onRead, this, _1, _2));
}

void
ServerResponseWrap::Init(v8::Local<v8::Object> target)
{
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "_write",
    Method<&ServerResponseWrap::_write>);
  Nan::SetPrototypeMethod(tpl, "headers",
    Method<&ServerResponseWrap::headers>);
  Nan::SetPrototypeMethod(tpl, "stream",
    Method<&ServerResponseWrap::stream>);

  auto func(Nan::GetFunction(tpl).ToLocalChecked());

  constructor().Reset(func);

  Nan::Set(target, Nan::New(ClassName()).ToLocalChecked(), func);
}

void
ServerResponseWrap::_write(const Nan::FunctionCallbackInfo<v8::Value>& info)
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
  _callback = callback;

  self().stream().resume();
}

mist::h2::generator_callback::result_type
ServerResponseWrap::onRead(std::uint8_t *data, std::size_t length)
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
  _lengthToWrite -= actualLength;
  return actualLength;
}

void
ServerResponseWrap::headers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  // TODO:
}

void
ServerResponseWrap::stream(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  info.GetReturnValue().Set(ServerStreamWrap::object(self().stream()));
}

} // namespace Node
} // namespace Mist
