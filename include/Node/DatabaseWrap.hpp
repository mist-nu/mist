/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "Node/Module.hpp"
#include "Database.h"

namespace Mist
{
namespace Node
{

//
// Database
//
class DatabaseWrap
  : public NodeWrapSingleton<DatabaseWrap, Mist::Database*>
{
public:

  static const char* ClassName() { return "Database"; }

  DatabaseWrap(Mist::Database* _self);

  static void Init(v8::Local<v8::Object> target);

private:

  void beginTransaction(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void inviteUser(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void getObject(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void query(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void queryVersion(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void subscribeObject(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void subscribeQuery(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void subscribeQueryVersion(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void unsubscribe(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void getManifest(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

//
// Database.Manifest
//
class ManifestWrap
  : public NodeWrap<ManifestWrap, Mist::Database::Manifest>
{
public:

  static const char* ClassName() { return "Manifest"; }

  ManifestWrap(const Mist::Database::Manifest&);

  static void Init(v8::Local<v8::Object> target);

private:

  void getHash(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void getCreator(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void sign(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void verify(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void toString(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

//
// Database.ObjectRef
//
class ObjectRefWrap
  : public NodeWrap<ObjectRefWrap, Mist::Database::ObjectRef>
{
public:

  static const char* ClassName() { return "ObjectRef"; }

  ObjectRefWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  ObjectRefWrap();
  ObjectRefWrap(const Mist::Database::ObjectRef&);

  static void Init(v8::Local<v8::Object> target);

private:

  void getAccessDomain(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void setAccessDomain(v8::Local<v8::String> name,
    v8::Local<v8::Value> value,
    const Nan::PropertyCallbackInfo<void>& info);
  void getId(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void setId(v8::Local<v8::String> name,
    v8::Local<v8::Value> value,
    const Nan::PropertyCallbackInfo<void>& info);

};

//
// Database.Object
//
class MistObjectWrap
  : public NodeWrap<MistObjectWrap, Mist::Database::Object>
{
public:

  static const char* ClassName() { return "Object"; }

  MistObjectWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  MistObjectWrap();
  MistObjectWrap(const Mist::Database::Object& other);

  static void Init(v8::Local<v8::Object> target);

private:

  void getAccessDomain(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void getId(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void getVersion(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void getParent(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void getAttributes(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void getStatus(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void getAction(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);

};

//
// Database.QueryResult
//
class QueryResultWrap
  : public NodeWrap<QueryResultWrap, Mist::Database::QueryResult>
{
public:

  static const char* ClassName() { return "QueryResult"; }

  QueryResultWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  QueryResultWrap();
  QueryResultWrap(const Mist::Database::QueryResult& other);

  static void Init(v8::Local<v8::Object> target);

private:

  void isFunctionCall(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void getFunctionName(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void getFunctionAttribute(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void getFunctionValue(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);
  void getObjects(v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info);

};

Database::Value toDatabaseValue(v8::Local<v8::Value> val);
v8::Local<v8::Value> fromDatabaseValue(Database::Value val);

namespace detail
{

template<>
struct NodeValueConverter<Database* const>
{
  static inline v8::Local<v8::Value> conv(const Database* const v)
  {
    return DatabaseWrap::object(const_cast<Database*>(v));
  }
};

template<>
struct NodeValueConverter<const Database::AccessDomain>
{
  static inline v8::Local<v8::Value> conv(const Database::AccessDomain& v)
  {
    return Nan::New(static_cast<std::uint8_t>(v));
  }
};

template<>
struct NodeValueConverter<const Database::Manifest>
{
  static inline v8::Local<v8::Value> conv(const Database::Manifest& v)
  {
    return ManifestWrap::make(v);
  }
};

}

} // namespace Node
} // namespace Mist
