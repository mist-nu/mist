/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "Node/SignatureWrap.hpp"
#include "Node/SHA3Wrap.hpp"

namespace Mist
{
namespace Node
{

SignatureWrap::SignatureWrap(const Nan::FunctionCallbackInfo<v8::Value>& info)
  : NodeWrap(Mist::CryptoHelper::Signature())
{
}

SignatureWrap::SignatureWrap()
  : NodeWrap(Mist::CryptoHelper::Signature())
{
}

SignatureWrap::SignatureWrap(const Mist::CryptoHelper::Signature& other)
  : NodeWrap(Mist::CryptoHelper::Signature(other))
{
}

v8::Local<v8::FunctionTemplate> SignatureWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetMethod(tpl, "fromString", &SignatureWrap::fromString);
  Nan::SetMethod(tpl, "fromBuffer", &SignatureWrap::fromBuffer);
  
  Nan::SetPrototypeMethod(tpl, "toString",
			  Method<&SignatureWrap::toString>);
  
  return tpl;
}

void
SignatureWrap::fromString(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto arg = convBack<std::string>(info[0]);
  info.GetReturnValue().Set
    (SignatureWrap::make(CryptoHelper::Signature::fromString(arg)));
}

void
SignatureWrap::fromBuffer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto arg = convBack<std::vector<std::uint8_t>>(info[0]);
  info.GetReturnValue().Set
    (SignatureWrap::make(CryptoHelper::Signature::fromBuffer(arg)));
}

void
SignatureWrap::toString(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  info.GetReturnValue().Set(conv(self().toString()));
}

} // namespace Node
} // namespace Mist
