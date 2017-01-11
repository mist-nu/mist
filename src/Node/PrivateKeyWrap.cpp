/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "Node/PrivateKeyWrap.hpp"
#include "Node/PublicKeyWrap.hpp"

namespace Mist
{
namespace Node
{

PrivateKeyWrap::PrivateKeyWrap()
  : NodeWrap(Mist::CryptoHelper::PrivateKey())
{
}

PrivateKeyWrap::PrivateKeyWrap(const Mist::CryptoHelper::PrivateKey& other)
  : NodeWrap(Mist::CryptoHelper::PrivateKey(other))
{
}

void
PrivateKeyWrap::Init(v8::Local<v8::Object> target)
{
  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "getPublicKey",
    Method<&PrivateKeyWrap::getPublicKey>);

  auto func(Nan::GetFunction(tpl).ToLocalChecked());

  constructor().Reset(func);

  Nan::Set(target, Nan::New(ClassName()).ToLocalChecked(), func);
}

void
PrivateKeyWrap::getPublicKey(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  info.GetReturnValue().Set
    (PublicKeyWrap::make(self().getPublicKey()));
}

} // namespace Node
} // namespace Mist
