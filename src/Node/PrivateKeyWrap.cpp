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

v8::Local<v8::FunctionTemplate> PrivateKeyWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "getPublicKey",
			  Method<&PrivateKeyWrap::getPublicKey>);
  
  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
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
