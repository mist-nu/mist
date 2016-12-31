#pragma once

#include "Node/Module.hpp"

#include "CryptoHelper.h"

namespace Mist
{
namespace Node
{

class PublicKeyWrap : public NodeWrap<PublicKeyWrap, Mist::CryptoHelper::PublicKey>
{
public:

  PublicKeyWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  PublicKeyWrap();
  PublicKeyWrap(const Mist::CryptoHelper::PublicKey& other);

  static const char* ClassName() { return "PublicKey"; }

  static v8::Local<v8::FunctionTemplate> Init();

  static void fromPem(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void fromDer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  
  void hash(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void toString(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void toDer(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
  struct NodeValueConverter<const Mist::CryptoHelper::PublicKey>
{
  static v8::Local<v8::Value> conv(const Mist::CryptoHelper::PublicKey& v)
  {
    return PublicKeyWrap::make(v);
  }
};

} // namespace detail
} // namespace Node
} // namespace Mist
