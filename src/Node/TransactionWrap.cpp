#include "Node/TransactionWrap.hpp"
#include "Node/DatabaseWrap.hpp"

namespace Mist
{
namespace Node
{

//
// Transaction
//
TransactionWrap::TransactionWrap(Mist::Transaction* _self)
  : NodeWrapSingleton(_self)
{
}

v8::Local<v8::FunctionTemplate>
TransactionWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "newObject",
    Method<&TransactionWrap::newObject>);
  Nan::SetPrototypeMethod(tpl, "moveObject",
    Method<&TransactionWrap::moveObject>);
  Nan::SetPrototypeMethod(tpl, "updateObject",
    Method<&TransactionWrap::updateObject>);
  Nan::SetPrototypeMethod(tpl, "deleteObject",
    Method<&TransactionWrap::deleteObject>);
  Nan::SetPrototypeMethod(tpl, "commit",
    Method<&TransactionWrap::commit>);
  Nan::SetPrototypeMethod(tpl, "rollback",
    Method<&TransactionWrap::rollback>);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

namespace {

std::map<std::string, Mist::Database::Value>
objectAttributes(v8::Local<v8::Value> attr)
{
  auto attrObj(attr.As<v8::Object>());
  auto propertyNames(Nan::GetPropertyNames(attrObj).ToLocalChecked());
  
  std::map<std::string, Mist::Database::Value> attrs;
  
  for (std::size_t i = 0; i < propertyNames->Length(); ++i) {
    auto attrNameVal(Nan::Get(propertyNames, i).ToLocalChecked());
    std::string attrName(*Nan::Utf8String(attrNameVal));
    auto attrValue(Nan::Get(attrObj, attrNameVal).ToLocalChecked());
    attrs[attrName] = toDatabaseValue(attrValue);
  }

  return attrs;
}
  
} // namespace

void
TransactionWrap::newObject(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  auto parent(ObjectRefWrap::self(info[0]));
  auto attrs(objectAttributes(info[1]));

  auto id(static_cast<double>(self()->newObject(parent, attrs)));
  info.GetReturnValue().Set(conv(id));
}

void
TransactionWrap::moveObject(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  auto id(convBack<double>(info[0]));
  auto newParent(ObjectRefWrap::self(info[1]));
  self()->moveObject(static_cast<unsigned long>(id), newParent);
}

void
TransactionWrap::updateObject(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  auto id(convBack<double>(info[0]));
  auto attrs(objectAttributes(info[1]));
  self()->updateObject(static_cast<unsigned long>(id), attrs);
}

void
TransactionWrap::deleteObject(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  auto id(convBack<double>(info[0]));
  self()->deleteObject(static_cast<unsigned long>(id));
}

void
TransactionWrap::commit(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  self()->commit();
}

void
TransactionWrap::rollback(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  self()->rollback();
}

} // namespace Node
} // namespace Mist
