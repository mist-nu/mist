/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "Node/CentralWrap.hpp"
#include "Node/DatabaseWrap.hpp"
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
  Nan::HandleScope scope;
  std::string path(convBack<std::string>(info[0]));
  bool isLoggerInitialized(convBack<bool>(info[1]));
  self() = std::make_shared<Mist::Central>(path, isLoggerInitialized);
}

CentralWrap::CentralWrap(std::shared_ptr<Mist::Central> _self)
  : NodeWrapSingleton(_self)
{
}

void
CentralWrap::Init(v8::Local<v8::Object> target)
{
  Nan::HandleScope scope;

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
  Nan::SetPrototypeMethod(tpl, "getPendingInvites",
    Method<&CentralWrap::getPendingInvites>);

  Nan::SetPrototypeMethod(tpl, "addAddressLookupServer",
    Method<&CentralWrap::addAddressLookupServer>);
  Nan::SetPrototypeMethod(tpl, "removeAddressLookupServer",
    Method<&CentralWrap::removeAddressLookupServer>);
  Nan::SetPrototypeMethod(tpl, "listAddressLookupServers",
    Method<&CentralWrap::listAddressLookupServers>);

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

  auto func(Nan::GetFunction(tpl).ToLocalChecked());

  constructor().Reset(func);

  Nan::Set(target, Nan::New(ClassName()).ToLocalChecked(), func);
}

void
CentralWrap::newInstance(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  std::string path(convBack<std::string>(info[0]));
  auto central(std::make_shared<Mist::Central>(path));
  info.GetReturnValue().Set(CentralWrap::object(central));
}

void
CentralWrap::init(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  boost::optional<std::string> privKey;
  if (info.Length() > 0)
    privKey = convBack<std::string>(info[0]);
  self()->init(privKey);
}

void
CentralWrap::create(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  boost::optional<std::string> privKey;
  if (info.Length() > 0)
    privKey = convBack<std::string>(info[0]);
  self()->create(privKey);
}

void
CentralWrap::close(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  self()->close();
}

void
CentralWrap::startEventLoop(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  self()->startEventLoop();
}

void
CentralWrap::startServeTor(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  std::string torPath(convBack<std::string>(info[0]));
  std::uint16_t torIncomingPortLow(convBack<std::uint16_t>(info[1]));
  std::uint16_t torIncomingPortHigh(convBack<std::uint16_t>(info[2]));
  std::uint16_t torOutgoingPortLow(convBack<std::uint16_t>(info[3]));
  std::uint16_t torOutgoingPortHigh(convBack<std::uint16_t>(info[4]));
  std::uint16_t torControlPortLow(convBack<std::uint16_t>(info[5]));
  std::uint16_t torControlPortHigh(convBack<std::uint16_t>(info[6]));
  auto startFunc(info[7].As<v8::Function>());
  auto exitFunc(info[8].As<v8::Function>());

  auto startCb(makeAsyncCallback<>(startFunc));
  std::function<void(v8::Local<v8::Function>, boost::system::error_code ec)>
    adapter = [](v8::Local<v8::Function> fn,
      boost::system::error_code ec) {
    // TODO: Make a converter for the error code
    Nan::Callback cb(fn);
    cb();
  };
  auto exitCb(makeAsyncCallback<boost::system::error_code>(exitFunc, adapter));

  self()->startServeTor(torPath,
    mist::io::port_range_list{ { torIncomingPortLow, torIncomingPortHigh } },
    mist::io::port_range_list{ { torOutgoingPortLow, torOutgoingPortHigh} },
    mist::io::port_range_list{ { torControlPortLow, torControlPortHigh} },
    startCb, exitCb);
}

void
CentralWrap::startServeDirect(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  std::uint16_t portLow(convBack<std::uint16_t>(info[0]));
  std::uint16_t portHigh(convBack<std::uint16_t>(info[1]));
  self()->startServeDirect(
      mist::io::port_range_list{ {portLow, portHigh} });
}

void
CentralWrap::getPublicKey(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  info.GetReturnValue().Set(conv(self()->getPublicKey()));
}

void
CentralWrap::getPrivateKey(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  info.GetReturnValue().Set(conv(self()->getPrivateKey()));
}

void
CentralWrap::sign(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto hash(SHA3Wrap::self(info[0]));
  info.GetReturnValue().Set(conv(self()->sign(hash)));
}

void
CentralWrap::verify(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto pubKey(PublicKeyWrap::self(info[0]));
  auto hash(SHA3Wrap::self(info[1]));
  auto sig(SignatureWrap::self(info[2]));
  info.GetReturnValue().Set(conv(self()->verify(pubKey, hash, sig)));
}

void
CentralWrap::getDatabase(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto dbHash(SHA3Wrap::self(info[0]));
  info.GetReturnValue().Set(conv(self()->getDatabase(dbHash)));
}

void
CentralWrap::createDatabase(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  std::string name(convBack<std::string>(info[0]));
  info.GetReturnValue().Set(conv(self()->createDatabase(name)));
}

void
CentralWrap::receiveDatabase(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto manifest(ManifestWrap::self(info[0]));
  info.GetReturnValue().Set(conv(self()->receiveDatabase(manifest)));
}

void
CentralWrap::listDatabases(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto databases(self()->listDatabases());
  v8::Local<v8::Array> arr = Nan::New<v8::Array>(databases.size());
  for (std::size_t i = 0; i < databases.size(); ++i) {
    Nan::Set(arr, i, ManifestWrap::make(databases[i]));
  }
  info.GetReturnValue().Set(arr);
}

void
CentralWrap::addPeer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto pubKey(PublicKeyWrap::self(info[0]));
  std::string name(convBack<std::string>(info[1]));
  Mist::PeerStatus status(static_cast<Mist::PeerStatus>(convBack<int>(info[2])));
  bool anonymous(convBack<bool>(info[3]));
  self()->addPeer(pubKey, name, status, anonymous);
}

void
CentralWrap::changePeer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto pubKeyHash(SHA3Wrap::self(info[0]));
  std::string name(convBack<std::string>(info[1]));
  Mist::PeerStatus status(
    static_cast<Mist::PeerStatus>(convBack<int>(info[2])));
  bool anonymous(convBack<bool>(info[3]));
  self()->changePeer(pubKeyHash, name, status, anonymous);
}

void
CentralWrap::removePeer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto pubKeyHash(SHA3Wrap::self(info[0]));
  self()->removePeer(pubKeyHash);
}

namespace
{
v8::Local<v8::Object> peerToObject(const Mist::Peer& peer)
{
  Nan::EscapableHandleScope scope;
  auto obj(Nan::New<v8::Object>());
  Nan::Set(obj, Nan::New("id").ToLocalChecked(), SHA3Wrap::make(peer.id));
  Nan::Set(obj, Nan::New("anonymous").ToLocalChecked(), Nan::New(peer.anonymous));
  Nan::Set(obj, Nan::New("key").ToLocalChecked(), PublicKeyWrap::make(peer.key));
  Nan::Set(obj, Nan::New("name").ToLocalChecked(), Nan::New(peer.name).ToLocalChecked());
  Nan::Set(obj, Nan::New("status").ToLocalChecked(), conv(peer.status));
  return scope.Escape(obj);
}
} // namespace

void
CentralWrap::listPeers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto arr(Nan::New<v8::Array>());
  auto peers(self()->listPeers());
  for (std::size_t i = 0; i < peers.size(); ++i) {
    arr->Set(i, peerToObject(peers[i]));
  }
  info.GetReturnValue().Set(arr);
}

void
CentralWrap::getPeer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    Nan::HandleScope scope;
    auto keyHash(SHA3Wrap::self(info[0]));
    info.GetReturnValue().Set(peerToObject(self()->getPeer(keyHash)));
}

void
CentralWrap::getPendingInvites(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    Nan::HandleScope scope;
    auto arr(Nan::New<v8::Array>());
    auto pendingInvites(self()->getPendingInvites());
    for (std::size_t i = 0; i < pendingInvites.size(); ++i) {
        arr->Set(i, ManifestWrap::make(pendingInvites[i]));
    }
    info.GetReturnValue().Set(arr);
}

void
CentralWrap::addAddressLookupServer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto address(convBack<std::string>(info[0]));
  auto port(convBack<std::uint16_t>(info[1]));
  self()->addAddressLookupServer(address, port);
}

void
CentralWrap::removeAddressLookupServer(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto address(convBack<std::string>(info[0]));
  auto port(convBack<std::uint16_t>(info[1]));
  self()->removeAddressLookupServer(address, port);
}

void
CentralWrap::listAddressLookupServers(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto arr(Nan::New<v8::Array>());
  auto addressLookupServers(self()->listAddressLookupServers());
  for (std::size_t i = 0; i < addressLookupServers.size(); ++i) {
    auto obj(Nan::New<v8::Object>());
    Nan::Set(obj, Nan::New("address").ToLocalChecked(),
      conv(addressLookupServers[i].first));
    Nan::Set(obj, Nan::New("port").ToLocalChecked(),
      conv(addressLookupServers[i].second));
    Nan::Set(arr, i, obj);
  }
  info.GetReturnValue().Set(arr);
}

void
CentralWrap::listDatabasePermissions(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto keyHash(SHA3Wrap::self(info[0]));
  auto arr(Nan::New<v8::Array>());
  auto permissions(self()->listDatabasePermissions(keyHash));
  for (std::size_t i = 0; i < permissions.size(); ++i) {
      Nan::Set(arr, i, SHA3Wrap::make(permissions[i]));
  }
  info.GetReturnValue().Set(arr);
}

void
CentralWrap::addServicePermission(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto keyHash(SHA3Wrap::self(info[0]));
  auto service(convBack<std::string>(info[1]));
  auto path(convBack<std::string>(info[2]));
  self()->addServicePermission(keyHash, service, path);
}

void
CentralWrap::removeServicePermission(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto keyHash(SHA3Wrap::self(info[0]));
  auto service(convBack<std::string>(info[1]));
  auto path(convBack<std::string>(info[2]));
  self()->removeServicePermission(keyHash, service, path);
}

void
CentralWrap::listServicePermissions(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto keyHash(SHA3Wrap::self(info[0]));
  auto permissions(self()->listServicePermissions(keyHash));
  auto map(Nan::New<v8::Object>());
  for (auto& item : permissions) {
    auto& services = item.second;
    auto arr(Nan::New<v8::Array>());
    for (std::size_t i = 0; i < services.size(); ++i) {
      Nan::Set(arr, i, conv(services[i]));
    }
    Nan::Set(map, conv(item.first), arr);
  }
  info.GetReturnValue().Set(map);
}

void
CentralWrap::startSync(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  auto newDbFunc(info[0].As<v8::Function>());
  auto anonymous(convBack<bool>(info[1]));
  self()->startSync(
    makeAsyncCallback<Mist::Database::Manifest>(newDbFunc),
    anonymous);
}

void
CentralWrap::stopSync(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  self()->stopSync();
}

void
CentralWrap::listServices(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  // TODO:
}

void
CentralWrap::registerService(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  // TODO:
}

void
CentralWrap::registerHttpService(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  // TODO:
}

} // namespace Node
} // namespace Mist
