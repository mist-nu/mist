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

class CentralWrap
  : public NodeWrapSingleton<CentralWrap, std::shared_ptr<Mist::Central>>
{
public:

  CentralWrap(const Nan::FunctionCallbackInfo<v8::Value>& info);
  CentralWrap(std::shared_ptr<Mist::Central> _self);

  static const char *ClassName() { return "Central"; }

  static void Init(v8::Local<v8::Object> target);

private:

  static void newInstance(const Nan::FunctionCallbackInfo<v8::Value>& info);
  
  void init(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void create(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void close(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void startEventLoop(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void startServeTor(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void startServeDirect(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void getPublicKey(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void getPrivateKey(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void sign(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void verify(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void getDatabase(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void createDatabase(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void receiveDatabase(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void listDatabases(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void addPeer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void changePeer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void removePeer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void listPeers(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void getPeer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void getPendingInvites(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void addAddressLookupServer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void removeAddressLookupServer(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void listAddressLookupServers(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void addDatabasePermission(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void removeDatabasePermission(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void listDatabasePermissions(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void addServicePermission(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void removeServicePermission(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void listServicePermissions(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void startSync(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void stopSync(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void listServices(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void registerService(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void registerHttpService(const Nan::FunctionCallbackInfo<v8::Value>& info);

  void openServiceRequest(const Nan::FunctionCallbackInfo<v8::Value>& info);
  void openServiceSocket(const Nan::FunctionCallbackInfo<v8::Value>& info);

};

namespace detail
{

template<>
struct NodeValueConverter<const Mist::PeerStatus>
{
  static inline v8::Local<v8::Value> conv(Mist::PeerStatus v)
  {
    return Nan::New(static_cast<int>(v));
  }
  static inline Mist::PeerStatus convBack(v8::Local<v8::Value> v)
  {
    return static_cast<Mist::PeerStatus>(Nan::To<int>(v).FromJust());
  }
};

}

} // namespace Node
} // namespace Mist
