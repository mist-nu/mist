#include "Node/ServerStreamWrap.hpp"

namespace Mist
{
namespace Node
{

void
ServerStreamWrap::request(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  info.GetReturnValue().Set(ServerRequestWrap::object(self().request()));
}

void
ServerStreamWrap::response(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  info.GetReturnValue().Set(ServerResponseWrap::object(self().response()));
}

void
ServerRequestWrap::setOnData(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  auto func = info[0].As<v8::Function>();
  std::function<void(v8::Local<v8::Function>, const std::uint8_t*,
    std::size_t)> onData
    = [](v8::Local<v8::Function> func, const std::uint8_t* data,
         std::size_t length)
  {
    v8::HandleScope scope(isolate);
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
  v8::HandleScope scope(isolate);
}

void
ServerRequestWrap::stream(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  info.GetReturnValue().Set(ServerStreamWrap::object(self().stream()));
}

void
ServerResponseWrap::_write(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

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
ServerResponseWrap::onRead(std::uint8_t *data, std::size_t length,
  std::uint32_t *flags)
{
  if (_dataToWrite && !_lengthToWrite) {
    _dataToWrite = nullptr;
    asyncCall([=]()
    {
      v8::HandleScope scope(isolate);

      Nan::Callback cb(Nan::New(_callback));
      cb();
    });
    return 0;
  }

  std::size_t actualLength = std::min(length, _lengthToWrite);
  std::copy(_dataToWrite, _dataToWrite + actualLength, data);
  _lengthToWrite -= actualLength;
  return actualLength;
}

void
ServerResponseWrap::headers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
}

void
ServerResponseWrap::stream(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  info.GetReturnValue().Set(ServerStreamWrap::object(self().stream()));
}

} // namespace Node
} // namespace Mist

