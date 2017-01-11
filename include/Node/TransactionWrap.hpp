/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
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

  static void Init(v8::Local<v8::Object> target);

private:

  void newObject(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void moveObject(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void updateObject(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void deleteObject(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void commit(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void rollback(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void getObject(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void query(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void queryVersion(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
struct NodeValueConverter<Mist::Transaction*>
{
  static inline v8::Local<v8::Value> conv(const Mist::Transaction* const v)
  {
    return TransactionWrap::object(const_cast<Mist::Transaction*>(v));
  }
};

} // namespace detail

} // namespace Node
} // namespace Mist

