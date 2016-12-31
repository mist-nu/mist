/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "Node/Module.hpp"

#include "CryptoHelper.h"

namespace Mist
{
namespace Node
{

class SignatureWrap : public NodeWrap<SignatureWrap, Mist::CryptoHelper::Signature>
{
public:

  SignatureWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  SignatureWrap();
  SignatureWrap(const Mist::CryptoHelper::Signature& other);

  static const char* ClassName() { return "Signature"; }

  static v8::Local<v8::FunctionTemplate> Init();

  static void fromString(const Nan::FunctionCallbackInfo<v8::Value>& info);
  static void fromBuffer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  
  void toString(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
  struct NodeValueConverter<const Mist::CryptoHelper::Signature>
{
  static v8::Local<v8::Value> conv(const Mist::CryptoHelper::Signature& v)
  {
    return SignatureWrap::make(v);
  }
};

} // namespace detail
} // namespace Node
} // namespace Mist
