#pragma once

#include "Node/Module.hpp"

#include "CryptoHelper.h"

namespace Mist
{
namespace Node
{

class PrivateKeyWrap : public NodeWrap<PrivateKeyWrap, Mist::CryptoHelper::PrivateKey>
{
public:

  PrivateKeyWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  PrivateKeyWrap();
  PrivateKeyWrap(const Mist::CryptoHelper::PrivateKey& other);

  static const char* ClassName() { return "PrivateKey"; }

  static v8::Local<v8::FunctionTemplate> Init();

  void getPublicKey(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
  struct NodeValueConverter<const Mist::CryptoHelper::PrivateKey>
{
  static v8::Local<v8::Value> conv(const Mist::CryptoHelper::PrivateKey& v)
  {
    return PrivateKeyWrap::make(v);
  }
};

} // namespace detail
} // namespace Node
} // namespace Mist
