#include "Node/CentralWrap.hpp"
#include "Node/SHA3Wrap.hpp"
#include "Node/PrivateKeyWrap.hpp"
#include "Node/PublicKeyWrap.hpp"
#include "Node/SignatureWrap.hpp"

namespace Mist
{
namespace Node
{

CentralWrap::CentralWrap(const Nan::FunctionCallbackInfo<v8::Value>& info)
  : NodeWrapSingleton(nullptr)
{
  v8::HandleScope scope(isolate);
  std::string path(convBack<std::string>(info[0]));
  self() = std::make_shared<Mist::Central>(path);
}

CentralWrap::CentralWrap(std::shared_ptr<Mist::Central> _self)
  : NodeWrapSingleton(_self)
{
}

v8::Local<v8::FunctionTemplate>
CentralWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl
    = constructingTemplate<CentralWrap>(ClassName());
  
  Nan::SetMethod(tpl, "newInstance", &CentralWrap::newInstance);

  Nan::SetPrototypeMethod(tpl, "init",
			  Method<&CentralWrap::init>);
  Nan::SetPrototypeMethod(tpl, "create",
			  Method<&CentralWrap::create>);
  Nan::SetPrototypeMethod(tpl, "close",
      Method<&CentralWrap::close>);

  Nan::SetPrototypeMethod(tpl, "startEventLoop",
      Method<&CentralWrap::startEventLoop>);

  Nan::SetPrototypeMethod(tpl, "startServeTor",
      Method<&CentralWrap::startServeTor>);
  Nan::SetPrototypeMethod(tpl, "startServeDirect",
      Method<&CentralWrap::startServeDirect>);

  Nan::SetPrototypeMethod(tpl, "getPublicKey",
			  Method<&CentralWrap::getPublicKey>);
  Nan::SetPrototypeMethod(tpl, "getPrivateKey",
			  Method<&CentralWrap::getPrivateKey>);

  Nan::SetPrototypeMethod(tpl, "getDatabase",
			  Method<&CentralWrap::getDatabase>);
  Nan::SetPrototypeMethod(tpl, "createDatabase",
			  Method<&CentralWrap::createDatabase>);
  Nan::SetPrototypeMethod(tpl, "receiveDatabase",
			  Method<&CentralWrap::receiveDatabase>);
  Nan::SetPrototypeMethod(tpl, "listDatabases",
			  Method<&CentralWrap::listDatabases>);

  Nan::SetPrototypeMethod(tpl, "addPeer",
			  Method<&CentralWrap::addPeer>);
  Nan::SetPrototypeMethod(tpl, "changePeer",
			  Method<&CentralWrap::changePeer>);
  Nan::SetPrototypeMethod(tpl, "removePeer",
			  Method<&CentralWrap::removePeer>);
  Nan::SetPrototypeMethod(tpl, "listPeers",
			  Method<&CentralWrap::listPeers>);
  Nan::SetPrototypeMethod(tpl, "getPeer",
			  Method<&CentralWrap::getPeer>);

  Nan::SetPrototypeMethod(tpl, "addAddressLookupServer",
			  Method<&CentralWrap::addAddressLookupServer>);
  Nan::SetPrototypeMethod(tpl, "removeAddressLookupServer",
			  Method<&CentralWrap::removeAddressLookupServer>);
  Nan::SetPrototypeMethod(tpl, "listAddressLookupServers",
			  Method<&CentralWrap::listAddressLookupServers>);
  
  Nan::SetPrototypeMethod(tpl, "addDatabasePermission",
			  Method<&CentralWrap::addDatabasePermission>);
  Nan::SetPrototypeMethod(tpl, "removeDatabasePermission",
			  Method<&CentralWrap::removeDatabasePermission>);
  Nan::SetPrototypeMethod(tpl, "listDatabasePermissions",
			  Method<&CentralWrap::listDatabasePermissions>);

  Nan::SetPrototypeMethod(tpl, "addServicePermission",
			  Method<&CentralWrap::addServicePermission>);
  Nan::SetPrototypeMethod(tpl, "removeServicePermission",
			  Method<&CentralWrap::removeServicePermission>);
  Nan::SetPrototypeMethod(tpl, "listServicePermissions",
			  Method<&CentralWrap::listServicePermissions>);

  Nan::SetPrototypeMethod(tpl, "startSync",
			  Method<&CentralWrap::startSync>);
  Nan::SetPrototypeMethod(tpl, "stopSync",
			  Method<&CentralWrap::stopSync>);

  Nan::SetPrototypeMethod(tpl, "listServices",
			  Method<&CentralWrap::listServices>);
  Nan::SetPrototypeMethod(tpl, "registerService",
			  Method<&CentralWrap::registerService>);
  Nan::SetPrototypeMethod(tpl, "registerHttpService",
			  Method<&CentralWrap::registerHttpService>);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

void
CentralWrap::newInstance(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  std::string path(convBack<std::string>(info[0]));
  auto central(std::make_shared<Mist::Central>(path));
  info.GetReturnValue().Set(CentralWrap::object(central));
}

void
CentralWrap::init(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  boost::optional<std::string> privKey;
  if (info.Length() > 0)
    privKey = convBack<std::string>(info[0]);
  self()->init(privKey);
}

void
CentralWrap::create(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  boost::optional<std::string> privKey;
  if (info.Length() > 0)
    privKey = convBack<std::string>(info[0]);
  self()->create(privKey);
}

void
CentralWrap::close(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    v8::HandleScope scope(isolate);
    self()->close();
}

void
CentralWrap::startEventLoop(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    v8::HandleScope scope(isolate);
    self()->startEventLoop();
}

void
CentralWrap::startServeTor(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    v8::HandleScope scope(isolate);
    std::string torPath(convBack<std::string>(info[0]));
    std::uint16_t torIncomingPortLow(convBack<std::uint16_t>(info[1]));
    std::uint16_t torIncomingPortHigh(convBack<std::uint16_t>(info[2]));
    std::uint16_t torOutgoingPortLow(convBack<std::uint16_t>(info[3]));
    std::uint16_t torOutgoingPortHigh(convBack<std::uint16_t>(info[4]));
    std::uint16_t torControlPortLow(convBack<std::uint16_t>(info[5]));
    std::uint16_t torControlPortHigh(convBack<std::uint16_t>(info[6]));

    self()->startServeTor(torPath,
        mist::io::port_range_list{ { torIncomingPortLow, torIncomingPortHigh } },
        mist::io::port_range_list{ { torOutgoingPortLow, torOutgoingPortHigh} },
        mist::io::port_range_list{ { torControlPortLow, torControlPortHigh} });
}

void
CentralWrap::startServeDirect(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    v8::HandleScope scope(isolate);
    std::uint16_t portLow(convBack<std::uint16_t>(info[0]));
    std::uint16_t portHigh(convBack<std::uint16_t>(info[1]));
    self()->startServeDirect(
        mist::io::port_range_list{ {portLow, portHigh} });
}

void
CentralWrap::getPublicKey(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  info.GetReturnValue().Set(conv(self()->getPublicKey()));
}

void
CentralWrap::getPrivateKey(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  info.GetReturnValue().Set(conv(self()->getPrivateKey()));
}

void
CentralWrap::sign(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto hash(SHA3Wrap::self(info[0]));
  info.GetReturnValue().Set(conv(self()->sign(hash)));
}

void
CentralWrap::verify(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto pubKey(PublicKeyWrap::self(info[0]));
  auto hash(SHA3Wrap::self(info[1]));
  auto sig(SignatureWrap::self(info[2]));
  info.GetReturnValue().Set(conv(self()->verify(pubKey, hash, sig)));
}

void
CentralWrap::getDatabase(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto dbHash(SHA3Wrap::self(info[0]));
  info.GetReturnValue().Set(conv(self()->getDatabase(dbHash)));
}

void
CentralWrap::createDatabase(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  std::string name(convBack<std::string>(info[0]));
  info.GetReturnValue().Set(conv(self()->createDatabase(name)));
}

void
CentralWrap::receiveDatabase(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  // TODO:
}

void
CentralWrap::listDatabases(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  // TODO:
}

void
CentralWrap::addPeer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto pubKey(PublicKeyWrap::self(info[0]));
  std::string name(convBack<std::string>(info[1]));
  Mist::PeerStatus status(static_cast<Mist::PeerStatus>(convBack<int>(info[2])));
  bool anonymous(convBack<bool>(info[3]));
  self()->addPeer(pubKey, name, status, anonymous);
}

void
CentralWrap::changePeer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    v8::HandleScope scope(isolate);
    auto pubKeyHash(SHA3Wrap::self(info[0]));
    std::string name(convBack<std::string>(info[1]));
    Mist::PeerStatus status(static_cast<Mist::PeerStatus>(convBack<int>(info[2])));
    bool anonymous(convBack<bool>(info[3]));
    self()->changePeer(pubKeyHash, name, status, anonymous);
}

void
CentralWrap::removePeer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    v8::HandleScope scope(isolate);
    auto pubKeyHash(SHA3Wrap::self(info[0]));
    self()->removePeer(pubKeyHash);
}

void
CentralWrap::listPeers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  // TODO:
}

void
CentralWrap::getPeer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  // TODO:
}

void
CentralWrap::addAddressLookupServer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto address(convBack<std::string>(info[0]));
  auto port(convBack<std::uint16_t>(info[1]));
  self()->addAddressLookupServer(address, port);
}

void
CentralWrap::removeAddressLookupServer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto address(convBack<std::string>(info[0]));
  auto port(convBack<std::uint16_t>(info[1]));
  self()->removeAddressLookupServer(address, port);
}

void
CentralWrap::listAddressLookupServers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  // TODO:
}

void
CentralWrap::addDatabasePermission(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto keyHash(SHA3Wrap::self(info[0]));
  auto dbHash(SHA3Wrap::self(info[1]));
  self()->addDatabasePermission(keyHash, dbHash);
}

void
CentralWrap::removeDatabasePermission(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto keyHash(SHA3Wrap::self(info[0]));
  auto dbHash(SHA3Wrap::self(info[1]));
  self()->removeDatabasePermission(keyHash, dbHash);
}

void
CentralWrap::listDatabasePermissions(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  // TODO:
}

void
CentralWrap::addServicePermission(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto keyHash(SHA3Wrap::self(info[0]));
  auto service(convBack<std::string>(info[1]));
  auto path(convBack<std::string>(info[2]));
  self()->addServicePermission(keyHash, service, path);
}

void
CentralWrap::removeServicePermission(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto keyHash(SHA3Wrap::self(info[0]));
  auto service(convBack<std::string>(info[1]));
  auto path(convBack<std::string>(info[2]));
  self()->removeServicePermission(keyHash, service, path);
}

void
CentralWrap::listServicePermissions(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  // TODO:
}

void
CentralWrap::startSync(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  auto newDbFunc(info[0].As<v8::Function>());
  auto anonymous(convBack<bool>(info[1]));
  self()->startSync(
      makeAsyncCallback<Mist::Database::Manifest>(newDbFunc),
      anonymous);
}

void
CentralWrap::stopSync(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  // TODO:
}

void
CentralWrap::listServices(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  // TODO:
}

void
CentralWrap::registerService(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  // TODO:
}

void
CentralWrap::registerHttpService(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  // TODO:
}

} // namespace Node
} // namespace Mist
