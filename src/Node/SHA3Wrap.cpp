#include "Node/SHA3Wrap.hpp"

namespace Mist
{
namespace Node
{

SHA3Wrap::SHA3Wrap()
  : NodeWrap(Mist::CryptoHelper::SHA3())
{
}

SHA3Wrap::SHA3Wrap(const Mist::CryptoHelper::SHA3& other)
  : NodeWrap(Mist::CryptoHelper::SHA3(other))
{
}

v8::Local<v8::FunctionTemplate> SHA3Wrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetMethod(tpl, "fromBuffer", &SHA3Wrap::fromBuffer);
  Nan::SetMethod(tpl, "fromString", &SHA3Wrap::fromString);
  
  Nan::SetPrototypeMethod(tpl, "toString",
			  Method<&SHA3Wrap::toString>);
  
  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

void
SHA3Wrap::toString(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  info.GetReturnValue().Set(conv(self().toString()));
}

void
SHA3Wrap::fromBuffer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  //info.GetReturnValue().Set(conv(self().toString()));
}

void
SHA3Wrap::fromString(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  std::string arg = convBack<std::string>(info[0]);
  info.GetReturnValue().Set
    (SHA3Wrap::make(CryptoHelper::SHA3::fromString(arg)));
}

} // namespace Node
} // namespace Mist
