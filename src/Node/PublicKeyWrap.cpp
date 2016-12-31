#include "Node/PublicKeyWrap.hpp"
#include "Node/SHA3Wrap.hpp"

namespace Mist
{
namespace Node
{

PublicKeyWrap::PublicKeyWrap()
  : NodeWrap(Mist::CryptoHelper::PublicKey())
{
}

PublicKeyWrap::PublicKeyWrap(const Mist::CryptoHelper::PublicKey& other)
  : NodeWrap(Mist::CryptoHelper::PublicKey(other))
{
}

v8::Local<v8::FunctionTemplate> PublicKeyWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetMethod(tpl, "fromPem", &PublicKeyWrap::fromPem);
  Nan::SetMethod(tpl, "fromDer", &PublicKeyWrap::fromDer);

  Nan::SetPrototypeMethod(tpl, "hash",
      Method<&PublicKeyWrap::hash>);
  Nan::SetPrototypeMethod(tpl, "toString",
      Method<&PublicKeyWrap::toString>);
  Nan::SetPrototypeMethod(tpl, "toDer",
      Method<&PublicKeyWrap::toDer>);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

void
PublicKeyWrap::fromPem(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto arg = convBack<std::string>(info[0]);
  info.GetReturnValue().Set
    (PublicKeyWrap::make(CryptoHelper::PublicKey::fromPem(arg)));
}

void
PublicKeyWrap::fromDer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto arg = convBack<std::vector<std::uint8_t>>(info[0]);
  info.GetReturnValue().Set
    (PublicKeyWrap::make(CryptoHelper::PublicKey::fromDer(arg)));
}

void
PublicKeyWrap::hash(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  info.GetReturnValue().Set
    (SHA3Wrap::make(self().hash()));
}

void
PublicKeyWrap::toString(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  info.GetReturnValue().Set(conv(self().toString()));
}

void
PublicKeyWrap::toDer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  info.GetReturnValue().Set(conv(self().toDer()));
}

} // namespace Node
} // namespace Mist
