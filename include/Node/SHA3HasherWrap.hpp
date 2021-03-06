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

class SHA3HasherWrap : public NodeWrap<SHA3HasherWrap, Mist::CryptoHelper::SHA3Hasher>
{
public:

  SHA3HasherWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  SHA3HasherWrap();

  static const char* ClassName() { return "SHA3Hasher"; }

  static void Init(v8::Local<v8::Object> target);

  void reset(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void update(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void finalize(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

} // namespace Node
} // namespace Mist
