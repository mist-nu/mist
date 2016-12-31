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

  ServiceWrap(mist::Service& service)
    : NodeWrapSingleton(service)
  {
  }

public:

  static const char* ClassName() { return "Service"; }

  static v8::Local<v8::FunctionTemplate> Init();

private:

  static void New(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void setOnPeerConnectionStatus(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void setOnPeerRequest(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void submit(const Nan::FunctionCallbackInfo<v8::Value>& info);
};

} // namespace Node
} // namespace Mist
