/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "Node/Module.hpp"

#include "conn/service.hpp"

namespace Mist
{
namespace Node
{

class ServiceWrap : public NodeWrapSingleton<ServiceWrap, mist::Service&>
{
private:

  ServiceWrap(mist::Service& service);

public:

  static const char* ClassName() { return "Service"; }

  static void Init(v8::Local<v8::Object> target);

private:

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void setOnPeerConnectionStatus(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void setOnPeerRequest(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void submit(const Nan::FunctionCallbackInfo<v8::Value>& info);
};

} // namespace Node
} // namespace Mist
