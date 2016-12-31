#pragma once

#include "Node/Module.hpp"

#include "Transaction.h"

namespace Mist
{
namespace Node
{
  
class TransactionWrap
  : public NodeWrapSingleton<TransactionWrap, Mist::Transaction*>
{
public:

  static const char* ClassName() { return "Transaction"; }

  TransactionWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  TransactionWrap(Mist::Transaction* _self);

  static v8::Local<v8::FunctionTemplate> Init();

private:

  void newObject(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void moveObject(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void updateObject(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void deleteObject(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void commit(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void rollback(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
struct NodeValueConverter<Mist::Transaction*>
{
  static v8::Local<v8::Value> conv(Mist::Transaction* v)
  {
    return TransactionWrap::object(v);
  }
};

} // namespace detail

} // namespace Node
} // namespace Mist

