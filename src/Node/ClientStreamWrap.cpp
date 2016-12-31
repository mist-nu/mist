#include "Node/ClientStreamWrap.hpp"

namespace Mist
{
namespace Node
{

ClientStreamWrap::ClientStreamWrap(mist::h2::ClientStream _self)
  : NodeWrapSingleton(_self) {}

v8::Local<v8::FunctionTemplate>
ClientStreamWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "request",
			  Method<&ClientStreamWrap::request>);
  Nan::SetPrototypeMethod(tpl, "response",
			  Method<&ClientStreamWrap::response>);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

void
ClientStreamWrap::request(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  info.GetReturnValue().Set(ClientRequestWrap::object(self().request()));
}

void
ClientStreamWrap::response(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  info.GetReturnValue().Set(ClientResponseWrap::object(self().response()));
}

ClientRequestWrap::ClientRequestWrap(mist::h2::ClientRequest& _self)
  : NodeWrapSingleton(_self),
    _inWrite(false), _lengthToWrite(0), _dataToWrite(nullptr)
{
  using namespace std::placeholders;
  self().setOnRead(std::bind(&ClientRequestWrap::onRead, this, _1, _2, _3));
}

v8::Local<v8::FunctionTemplate>
ClientRequestWrap::Init()
{
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

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}
  
void
ClientRequestWrap::_write(const Nan::FunctionCallbackInfo<v8::Value>& info)
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
  _callback.Reset(callback);

  self().stream().resume();
}

mist::h2::generator_callback::result_type
ClientRequestWrap::onRead(std::uint8_t* data, std::size_t length,
  std::uint32_t* flags)
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
  _dataToWrite += actualLength;
  _lengthToWrite -= actualLength;
  return actualLength;
}

void
ClientRequestWrap::setOnResponse(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto func = v8::Local<v8::Function>::Cast(info[0]);

  self(info.Holder()).setOnResponse(
    makeAsyncCallback<mist::h2::ClientResponse>(func));
}

void
ClientRequestWrap::setOnPush(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto func = v8::Local<v8::Function>::Cast(info[0]);

  self(info.Holder()).setOnPush(
    makeAsyncCallback<mist::h2::ClientRequest>(func));
}

void
ClientRequestWrap::headers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
}

void
ClientRequestWrap::stream(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  info.GetReturnValue().Set(ClientStreamWrap::object(self().stream()));
}

ClientResponseWrap::ClientResponseWrap(mist::h2::ClientResponse _self)
  : NodeWrapSingleton(_self)
{
}

v8::Local<v8::FunctionTemplate>
ClientResponseWrap::Init()
{
    v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

    Nan::SetPrototypeMethod(tpl, "setOnData",
      Method<&ClientResponseWrap::setOnData>);
    Nan::SetPrototypeMethod(tpl, "stream",
      Method<&ClientResponseWrap::stream>);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

    return tpl;
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
ClientResponseWrap::headers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
}

void
ClientResponseWrap::stream(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  info.GetReturnValue().Set(ClientStreamWrap::object(self().stream()));
}

} // namespace Node
} // namespace Mist
