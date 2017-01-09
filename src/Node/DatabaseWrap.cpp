#include "Node/DatabaseWrap.hpp"
#include "Node/PrivateKeyWrap.hpp"
#include "Node/PublicKeyWrap.hpp"
#include "Node/SHA3Wrap.hpp"
#include "Node/TransactionWrap.hpp"

namespace Mist
{
namespace Node
{

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

  Nan::SetPrototypeMethod(tpl, "beginTransaction",
    Method<&DatabaseWrap::beginTransaction>);
  Nan::SetPrototypeMethod(tpl, "inviteUser",
    Method<&DatabaseWrap::inviteUser>);
  Nan::SetPrototypeMethod(tpl, "getObject",
    Method<&DatabaseWrap::getObject>);
  Nan::SetPrototypeMethod(tpl, "query",
    Method<&DatabaseWrap::query>);
  Nan::SetPrototypeMethod(tpl, "queryVersion",
    Method<&DatabaseWrap::queryVersion>);
  Nan::SetPrototypeMethod(tpl, "subscribeObject",
    Method<&DatabaseWrap::subscribeObject>);
  Nan::SetPrototypeMethod(tpl, "subscribeQuery",
    Method<&DatabaseWrap::subscribeQuery>);
  Nan::SetPrototypeMethod(tpl, "subscribeQueryVersion",
    Method<&DatabaseWrap::subscribeQueryVersion>);
  Nan::SetPrototypeMethod(tpl, "unsubscribe",
    Method<&DatabaseWrap::unsubscribe>);
  Nan::SetPrototypeMethod(tpl, "getManifest",
    Method<&DatabaseWrap::getManifest>);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

void DatabaseWrap::beginTransaction(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    Database::AccessDomain accessDomain{static_cast<Database::AccessDomain>(convBack<int>(info[0]))};
    Mist::Transaction* ptr{self()->beginTransaction(accessDomain).release()};
    info.GetReturnValue().Set(TransactionWrap::object( ptr ));
}

void DatabaseWrap::inviteUser(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    std::string name{convBack<std::string>(info[0])};
    CryptoHelper::PublicKey publicKey(CryptoHelper::PublicKey::fromPem(convBack<std::string>(info[1])));
    Permission permission(convBack<std::string>(info[2]));
    self()->inviteUser({name, publicKey, publicKey.hash(), permission});
    info.GetReturnValue().SetUndefined();
}

void DatabaseWrap::getObject(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    int accessDomain{convBack<int>(info[0])};
    long long id{convBack<long long>(info[1])};
    bool includeDeleted{convBack<bool>(info[2])};
    info.GetReturnValue().Set(MistObjectWrap::make(self()->getObject(accessDomain,id,includeDeleted)));
}

void DatabaseWrap::query(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    int accessDomain{convBack<int>(info[0])};
    long long id{convBack<long long>(info[1])};
    const std::string select{convBack<std::string>(info[2])};
    const std::string filter{convBack<std::string>(info[3])};
    const std::string sort{convBack<std::string>(info[4])};
    auto attrs(objectAttributes(info[5]));
    int maxVersion{convBack<int>(info[6])};
    bool includeDeleted{convBack<bool>(info[7])};

    info.GetReturnValue().Set(QueryResultWrap::make(self()->query(
            accessDomain,
            id,
            select,
            filter,
            sort,
            attrs,
            maxVersion,
            includeDeleted
            )));
}

void DatabaseWrap::queryVersion(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    int accessDomain{convBack<int>(info[0])};
    long long id{convBack<long long>(info[1])};
    const std::string select{convBack<std::string>(info[2])};
    const std::string filter{convBack<std::string>(info[3])};
    auto attrs(objectAttributes(info[5]));
    bool includeDeleted{convBack<bool>(info[7])};

    info.GetReturnValue().Set(QueryResultWrap::make(self()->queryVersion(
            accessDomain,
            id,
            select,
            filter,
            attrs,
            includeDeleted
            )));
}

void DatabaseWrap::subscribeObject(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    // TODO
}

void DatabaseWrap::subscribeQuery(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    // TODO
}

void DatabaseWrap::subscribeQueryVersion(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    // TODO
}

void DatabaseWrap::unsubscribe(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    // TODO
}

void DatabaseWrap::getManifest(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    info.GetReturnValue().Set(ManifestWrap::make(*(self()->getManifest())));
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

  Nan::SetPrototypeMethod(tpl, "getHash",
    Method<&ManifestWrap::getHash>);
  Nan::SetPrototypeMethod(tpl, "getCreator",
    Method<&ManifestWrap::getCreator>);
  Nan::SetPrototypeMethod(tpl, "sign",
    Method<&ManifestWrap::sign>);
  Nan::SetPrototypeMethod(tpl, "verify",
    Method<&ManifestWrap::verify>);
  Nan::SetPrototypeMethod(tpl, "toString",
    Method<&ManifestWrap::toString>);

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

//
// ObjectRef
//
MistObjectWrap::MistObjectWrap(const Nan::FunctionCallbackInfo<v8::Value>& info)
  : NodeWrap(Mist::Database::Object{
    Database::AccessDomain::Normal,
    0,
    0,
    {Database::AccessDomain::Normal, 0},
    {},
    Database::ObjectStatus::InvalidNew,
    Database::ObjectAction::Delete
})
{
}

MistObjectWrap::MistObjectWrap()
  : NodeWrap(Mist::Database::Object{
    Database::AccessDomain::Normal,
    0,
    0,
    {Database::AccessDomain::Normal, 0},
    {},
    Database::ObjectStatus::InvalidNew,
    Database::ObjectAction::Delete
})
{
}

MistObjectWrap::MistObjectWrap(const Mist::Database::Object& other)
  : NodeWrap(Mist::Database::Object(other))
{
}

v8::Local<v8::FunctionTemplate>
MistObjectWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl
    = constructingTemplate<ObjectRefWrap>(ClassName());

  v8::Local<v8::ObjectTemplate> objTpl = tpl->InstanceTemplate();
  Nan::SetAccessor(objTpl, Nan::New("accessDomain").ToLocalChecked(),
           Getter<&MistObjectWrap::getAccessDomain>);
  Nan::SetAccessor(objTpl, Nan::New("id").ToLocalChecked(),
           Getter<&MistObjectWrap::getId>);
  Nan::SetAccessor(objTpl, Nan::New("version").ToLocalChecked(),
           Getter<&MistObjectWrap::getVersion>);
  Nan::SetAccessor(objTpl, Nan::New("parent").ToLocalChecked(),
           Getter<&MistObjectWrap::getParent>);
  Nan::SetAccessor(objTpl, Nan::New("attributes").ToLocalChecked(),
           Getter<&MistObjectWrap::getAttributes>);
  Nan::SetAccessor(objTpl, Nan::New("status").ToLocalChecked(),
           Getter<&MistObjectWrap::getStatus>);
  Nan::SetAccessor(objTpl, Nan::New("action").ToLocalChecked(),
           Getter<&MistObjectWrap::getAction>);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

void
MistObjectWrap::getAccessDomain(v8::Local<v8::String> name,
                   const Nan::PropertyCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(conv(static_cast<std::uint8_t>(self().accessDomain)));
}

void
MistObjectWrap::getId(v8::Local<v8::String> name,
                   const Nan::PropertyCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(conv(static_cast<double>(self().id)));
}

void
MistObjectWrap::getVersion(v8::Local<v8::String> name,
                   const Nan::PropertyCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(conv(static_cast<unsigned>(self().version)));
}

void
MistObjectWrap::getParent(v8::Local<v8::String> name,
                   const Nan::PropertyCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(ObjectRefWrap::make(self().parent));
}

void
MistObjectWrap::getAttributes(v8::Local<v8::String> name,
                   const Nan::PropertyCallbackInfo<v8::Value>& info)
{
    auto obj(Nan::New<v8::Object>());
    auto attributes(self().attributes);
    for ( const auto& attribute: attributes) {
        obj->Set( conv(attribute.first), fromDatabaseValue(attribute.second));
    }
    info.GetReturnValue().Set(obj);
}

void
MistObjectWrap::getStatus(v8::Local<v8::String> name,
                   const Nan::PropertyCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(conv(static_cast<int>(self().status)));
}

void
MistObjectWrap::getAction(v8::Local<v8::String> name,
                   const Nan::PropertyCallbackInfo<v8::Value>& info)
{
  info.GetReturnValue().Set(conv(static_cast<int>(self().action)));
}


//
// QueryResult
//
QueryResultWrap::QueryResultWrap(const Nan::FunctionCallbackInfo<v8::Value>& info)
  : NodeWrap(Mist::Database::QueryResult{})
{
}

QueryResultWrap::QueryResultWrap()
  : NodeWrap(Mist::Database::QueryResult{})
{
}

QueryResultWrap::QueryResultWrap(const Mist::Database::QueryResult& other)
  : NodeWrap(Mist::Database::QueryResult(other))
{
}

v8::Local<v8::FunctionTemplate>
QueryResultWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl
    = constructingTemplate<QueryResultWrap>(ClassName());

  v8::Local<v8::ObjectTemplate> objTpl = tpl->InstanceTemplate();
  Nan::SetAccessor(objTpl, Nan::New("isFunctionCall").ToLocalChecked(),
           Getter<&QueryResultWrap::isFunctionCall>);
  Nan::SetAccessor(objTpl, Nan::New("functionName").ToLocalChecked(),
           Getter<&QueryResultWrap::getFunctionName>);
  Nan::SetAccessor(objTpl, Nan::New("functionAttribute").ToLocalChecked(),
           Getter<&QueryResultWrap::getFunctionAttribute>);
  Nan::SetAccessor(objTpl, Nan::New("functionValue").ToLocalChecked(),
           Getter<&QueryResultWrap::getFunctionValue>);
  Nan::SetAccessor(objTpl, Nan::New("objects").ToLocalChecked(),
           Getter<&QueryResultWrap::getObjects>);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

void QueryResultWrap::isFunctionCall(v8::Local<v8::String> name,
        const Nan::PropertyCallbackInfo<v8::Value>& info)
{
    bool b(static_cast<bool>(self().isFunctionCall));
    info.GetReturnValue().Set(Nan::New(b));
}

void QueryResultWrap::getFunctionName(v8::Local<v8::String> name,
        const Nan::PropertyCallbackInfo<v8::Value>& info)
{
    info.GetReturnValue().Set(conv(self().functionName));
}

void QueryResultWrap::getFunctionAttribute(v8::Local<v8::String> name,
        const Nan::PropertyCallbackInfo<v8::Value>& info)
{
    info.GetReturnValue().Set(conv(self().functionAttribute));
}

void QueryResultWrap::getFunctionValue(v8::Local<v8::String> name,
        const Nan::PropertyCallbackInfo<v8::Value>& info)
{
    double value(static_cast<double>(self().functionValue));
    info.GetReturnValue().Set(Nan::New(value));
}

void QueryResultWrap::getObjects(v8::Local<v8::String> name,
        const Nan::PropertyCallbackInfo<v8::Value>& info)
{
    v8::HandleScope scope(isolate);
    auto arr(Nan::New<v8::Array>());
    auto objects(self().objects);
    int i{0};
    for ( const auto& object: objects) {
        arr->Set(i++, MistObjectWrap::make(object));
    }
    info.GetReturnValue().Set(arr);
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
