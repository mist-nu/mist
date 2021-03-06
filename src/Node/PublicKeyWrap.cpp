/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

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

void
PublicKeyWrap::Init(v8::Local<v8::Object> target)
{
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetMethod(tpl, "fromPem", &PublicKeyWrap::fromPem);
  Nan::SetMethod(tpl, "fromDer", &PublicKeyWrap::fromDer);

  Nan::SetPrototypeMethod(tpl, "hash",
      Method<&PublicKeyWrap::hash>);
  Nan::SetPrototypeMethod(tpl, "toString",
      Method<&PublicKeyWrap::toString>);
  Nan::SetPrototypeMethod(tpl, "toDer",
      Method<&PublicKeyWrap::toDer>);
  Nan::SetPrototypeMethod(tpl, "fingerprint",
      Method<&PublicKeyWrap::fingerprint>);

  auto func(Nan::GetFunction(tpl).ToLocalChecked());

  constructor().Reset(func);

  Nan::Set(target, Nan::New(ClassName()).ToLocalChecked(), func);
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

void
PublicKeyWrap::fingerprint(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  info.GetReturnValue().Set(conv(self().fingerprint()));
}

} // namespace Node
} // namespace Mist
