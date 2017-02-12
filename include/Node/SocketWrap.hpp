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

class SocketWrap
  : public NodeWrapSingleton<SocketWrap, std::shared_ptr<mist::io::Socket>>
{
public:

  static const char* ClassName() { return "Socket"; }

  SocketWrap(std::shared_ptr<mist::io::Socket> _self);

  static void Init(v8::Local<v8::Object> target);

private:

  void _write(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void setOnData(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
struct NodeValueConverter<const std::shared_ptr<mist::io::Socket>>
{
  static inline v8::Local<v8::Value> conv(std::shared_ptr<mist::io::Socket> v)
  {
    return SocketWrap::object(v);
  }
};

} // namespace detail

} // namespace Node
} // namespace Mist
