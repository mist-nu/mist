/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "Node/Module.hpp"

namespace Mist
{
namespace Node
{

class SHA3Wrap : public NodeWrap<SHA3Wrap, Mist::CryptoHelper::SHA3>
{
public:

  SHA3Wrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  SHA3Wrap();
  SHA3Wrap(const Mist::CryptoHelper::SHA3& other);

  static const char* ClassName() { return "SHA3"; }

  static void Init(v8::Local<v8::Object> target);

  static void fromBuffer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void fromString(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void toString(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
  struct NodeValueConverter<const Mist::CryptoHelper::SHA3>
{
  static inline v8::Local<v8::Value> conv(const Mist::CryptoHelper::SHA3& v)
  {
    return SHA3Wrap::make(v);
  }
};

} // namespace detail
} // namespace Node
} // namespace Mist
