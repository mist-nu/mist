/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "Node/SocketWrap.hpp"

namespace Mist
{
namespace Node
{

SocketWrap::SocketWrap(std::shared_ptr<mist::io::Socket> _self)
  : NodeWrapSingleton(_self) {}

void
SocketWrap::Init(v8::Local<v8::Object> target)
{
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "_write",
    Method<&SocketWrap::_write>);
  Nan::SetPrototypeMethod(tpl, "setOnData",
    Method<&SocketWrap::setOnData>);

  auto func(Nan::GetFunction(tpl).ToLocalChecked());

  constructor().Reset(func);

  Nan::Set(target, Nan::New(ClassName()).ToLocalChecked(), func);
}

void
SocketWrap::_write(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  v8::Local<v8::Object> chunk = info[0].As<v8::Object>();
  std::string encoding = convBack<std::string>(info[1]);
  v8::Local<v8::Function> func = info[2].As<v8::Function>();

  assert(node::Buffer::HasInstance(chunk));

  std::function<void(v8::Local<v8::Function> func, std::size_t,
    boost::system::error_code)> writeCallback
    = [](v8::Local<v8::Function> func, std::size_t written,
        boost::system::error_code ec)
  {
    Nan::HandleScope scope;
    Nan::Callback cb(func);
    cb();
  };
  self()->write(
    reinterpret_cast<const std::uint8_t*>(node::Buffer::Data(chunk)),
    node::Buffer::Length(chunk), makeAsyncCallback<std::size_t,
      boost::system::error_code>(func, writeCallback));
}

void
SocketWrap::setOnData(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;

  auto func = info[0].As<v8::Function>();
  std::function<void(v8::Local<v8::Function>, const std::uint8_t*,
    std::size_t, boost::system::error_code)> onData
    = [](v8::Local<v8::Function> func, const std::uint8_t* data,
        std::size_t length, boost::system::error_code ec)
  {
    Nan::HandleScope scope;
    Nan::Callback cb(func);
    v8::Local<v8::Value> args[] = {
      node::Buffer::Copy(isolate, reinterpret_cast<const char*>(data),
        length).ToLocalChecked()
    };
    cb(1, args);
  };
  self()->read(
    makeAsyncCallback<const std::uint8_t*, std::size_t,
      boost::system::error_code>(func, onData));
}

} // namespace Node
} // namespace Mist
