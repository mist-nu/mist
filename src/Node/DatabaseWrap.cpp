#include "Node/DatabaseWrap.hpp"
#include "Node/PrivateKeyWrap.hpp"

namespace Mist
{
namespace Node
{

//
// Database
//
DatabaseWrap::DatabaseWrap(Mist::Database* _self)
  : NodeWrapSingleton(_self)
{
}

v8::Local<v8::FunctionTemplate>
DatabaseWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  // TODO: Remove from Module.cpp and add here:
  //v8::Local<v8::ObjectTemplate> objTpl = tpl->InstanceTemplate();
  //Nan::Set(objTpl,
  //	   conv(ManifestWrap::ClassName()),
  // 	   Nan::GetFunction(ManifestWrap::Init()).ToLocalChecked());
  // Nan::Set(objTpl,
  // 	   conv(ObjectRefWrap::ClassName()),
  // 	   Nan::GetFunction(ObjectRefWrap::Init()).ToLocalChecked());

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

//
// Manifest
//
ManifestWrap::ManifestWrap(const Mist::Database::Manifest& other)
  : NodeWrap(Mist::Database::Manifest(other))
{
}

v8::Local<v8::FunctionTemplate>
ManifestWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  //Nan::SetPrototypeMethod(tpl, "_write",
  //  Method<&ServerResponse::_write>);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

void ManifestWrap::getHash(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(conv(self().getHash()));
}

void ManifestWrap::getCreator(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(conv(self().getCreator()));
}

void ManifestWrap::sign(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
// TODO Fix error
  //auto privateKey(PrivateKeyWrap::self(info[0]));
  //self().sign(&privateKey);
}

void ManifestWrap::verify(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(conv(self().verify()));
}

void ManifestWrap::toString(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(conv(self().toString()));
}

//
// ObjectRef
//
ObjectRefWrap::ObjectRefWrap(const Nan::FunctionCallbackInfo<v8::Value>& info)
  : NodeWrap(Mist::Database::ObjectRef{Database::AccessDomain::Normal, 0})
{
}

ObjectRefWrap::ObjectRefWrap()
  : NodeWrap(Mist::Database::ObjectRef{Database::AccessDomain::Normal, 0})
{
}

ObjectRefWrap::ObjectRefWrap(const Mist::Database::ObjectRef& other)
  : NodeWrap(Mist::Database::ObjectRef(other))
{
}

v8::Local<v8::FunctionTemplate>
ObjectRefWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl
    = constructingTemplate<ObjectRefWrap>(ClassName());

  v8::Local<v8::ObjectTemplate> objTpl = tpl->InstanceTemplate();
  Nan::SetAccessor(objTpl, Nan::New("accessDomain").ToLocalChecked(),
		   Getter<&ObjectRefWrap::getAccessDomain>,
  		   Setter<&ObjectRefWrap::setAccessDomain>);
  Nan::SetAccessor(objTpl, Nan::New("id").ToLocalChecked(),
  		   Getter<&ObjectRefWrap::getId>,
  		   Setter<&ObjectRefWrap::setId>);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

void
ObjectRefWrap::getAccessDomain(v8::Local<v8::String> name,
			       const Nan::PropertyCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(conv(static_cast<std::uint8_t>(self().accessDomain)));
}

void
ObjectRefWrap::setAccessDomain(v8::Local<v8::String> name,
			       v8::Local<v8::Value> value,
			       const Nan::PropertyCallbackInfo<void>& info)
{
  self().accessDomain = static_cast<Database::AccessDomain>
    (convBack<std::int8_t>(value));
}

void
ObjectRefWrap::getId(v8::Local<v8::String> name,
		     const Nan::PropertyCallbackInfo<v8::Value>& info)
{
  double id(static_cast<double>(self().id));
  info.GetReturnValue().Set(Nan::New(id));
}

void
ObjectRefWrap::setId(v8::Local<v8::String> name,
		     v8::Local<v8::Value> value,
		     const Nan::PropertyCallbackInfo<void>& info)
{
  double id(convBack<double>(value));
  self().id = static_cast<unsigned long>(id);
}

Database::Value toDatabaseValue(v8::Local<v8::Value> val)
{
  using val_t = Database::Value;
  if (val->IsBoolean()) {
    return val_t(Nan::To<bool>(val).FromJust());
  } else if (val->IsString() || val->IsSymbol()) {
    return val_t(*Nan::Utf8String(val));
  } else if (val->IsInt32()) {
    return val_t(static_cast<double>(Nan::To<std::int32_t>(val).FromJust()));
  } else if (val->IsUint32()) {
    return val_t(static_cast<double>(Nan::To<std::uint32_t>(val).FromJust()));
  } else if (val->IsNumber()) {
    return val_t(Nan::To<double>(val).FromJust());
  } else {
    return val_t();
  }
}

v8::Local<v8::Value> fromDatabaseValue(Database::Value val)
{
  if (val.t == Database::Value::Type::Boolean) {
    return Nan::New(val.b);
  } else if (val.t == Database::Value::Type::Number) {
    return Nan::New(val.n);
  } else if (val.t == Database::Value::Type::String) {
    return Nan::New(val.v).ToLocalChecked();
  } else if (val.t == Database::Value::Type::Json) {
    // TODO: Convert from Json
    return v8::Local<v8::Value>();
  } else {
    return v8::Local<v8::Value>();
  }
}

} // namespace Node
} // namespace Mist
