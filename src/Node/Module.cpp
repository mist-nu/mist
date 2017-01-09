#include "Node/Module.hpp"
#include "Node/PeerWrap.hpp"
#include "Node/CentralWrap.hpp"
#include "Node/ClientStreamWrap.hpp"
#include "Node/DatabaseWrap.hpp"
#include "Node/ServerStreamWrap.hpp"
#include "Node/ServiceWrap.hpp"
#include "Node/SHA3Wrap.hpp"
#include "Node/SHA3HasherWrap.hpp"
#include "Node/PublicKeyWrap.hpp"
#include "Node/PrivateKeyWrap.hpp"

namespace Mist
{
namespace Node
{

v8::Isolate* isolate = nullptr;

v8::Local<v8::Object>
require(v8::Local<v8::Object> module, const std::string& path)
{
  v8::HandleScope scope(isolate);

  v8::Local<v8::Function> require
    = module->Get(conv("require")).As<v8::Function>();

  v8::Local<v8::Value> args[] = { conv(path) };

  return require->Call(module, 1, args).As<v8::Object>();
}

//void
//inherit(v8::Local<v8::Object> module, v8::Local<v8::Object> userClass,
//  v8::Local<v8::Object> baseClass)
//{
//  v8::Local<v8::Object> moduleUtil = Nan::New(moduleUtil_p);
//
//  v8::Local<v8::Function> inherit
//    = moduleUtil->Get(conv("inherit")).As<v8::Function>();
//
//  v8::Local<v8::Value> args[] = { userClass, baseClass };
//  inherit->Call(module, 2, args);
//}

// void
// initializeNSS(const Nan::FunctionCallbackInfo<v8::Value>& info)
// {
//   v8::HandleScope scope(isolate);

//   std::string dbDir(convBack<std::string>(info[0]));
//   v8::Local<v8::Function> peerAuthenticateCb = info[1].As<v8::Function>();

//   sslCtx = mist::make_unique<mist::io::SSLContext>(ioCtx, dbDir);
//   connCtx = mist::make_unique<mist::ConnectContext>(*sslCtx,
//     makeAsyncCallback<mist::Peer&>(peerAuthenticateCb));

//   /* Start the IO event loop in a separate thread */
//   ioCtx.queueJob([]() { ioCtx.exec(); });
// }

// void
// loadPKCS12(const Nan::FunctionCallbackInfo<v8::Value>& info)
// {
//   v8::HandleScope scope(isolate);

//   std::string data(convBack<std::string>(info[0]));
//   std::string password(convBack<std::string>(info[1]));

//   sslCtx->loadPKCS12(data, password);
// }

// void
// loadPKCS12File(const Nan::FunctionCallbackInfo<v8::Value>& info)
// {
//   v8::HandleScope scope(isolate);

//   std::string filename(convBack<std::string>(info[0]));
//   std::string password(convBack<std::string>(info[1]));

//   sslCtx->loadPKCS12File(filename, password);
// }

// void
// serveDirect(const Nan::FunctionCallbackInfo<v8::Value>& info)
// {
//   v8::HandleScope scope(isolate);

//   std::uint16_t incomingPort(convBack<std::uint16_t>(info[0]));

//   connCtx->serveDirect({{incomingPort, incomingPort}});
// }

// void
// startServeTor(const Nan::FunctionCallbackInfo<v8::Value>& info)
// {
//   v8::HandleScope scope(isolate);

//   try {

//     std::uint16_t torIncomingPort(convBack<std::uint16_t>(info[0]));
//     std::uint16_t torOutgoingPort(convBack<std::uint16_t>(info[1]));
//     std::uint16_t controlPort(convBack<std::uint16_t>(info[2]));
//     std::string executableName(convBack<std::string>(info[3]));
//     std::string workingDir(convBack<std::string>(info[4]));

//     std::cerr
//       << torIncomingPort
//       << " " << torOutgoingPort
//       << " " << controlPort
//       << " " << executableName
//       << " " << workingDir << std::endl;
//     connCtx->startServeTor(
//       {{torIncomingPort, torIncomingPort}},
//       {{torOutgoingPort, torOutgoingPort}},
//       {{controlPort, controlPort}},
//       executableName, workingDir);
//   } catch (boost::exception &) {
//     std::cerr
//       << "Unexpected exception, diagnostic information follows:" << std::endl
//       << boost::current_exception_diagnostic_information();
//   }
// }

// void
// connectPeerDirect(const Nan::FunctionCallbackInfo<v8::Value>& info)
// {
//   v8::HandleScope scope(isolate);

//   mist::Peer& peer = Peer::self(info[0].As<v8::Object>());
//   std::string addrStr(convBack<std::string>(info[1]));
//   std::uint16_t port(convBack<std::uint16_t>(info[2]));
//   auto addr = mist::io::Address::fromAny(addrStr, port);

//   connCtx->connectPeerDirect(peer, addr);
// }

// void
// connectPeerTor(const Nan::FunctionCallbackInfo<v8::Value>& info)
// {
//   v8::HandleScope scope(isolate);

//   mist::Peer& peer = Peer::self(info[0].As<v8::Object>());

//   connCtx->connectPeerTor(peer);
// }

// void
// addAuthenticatedPeer(const Nan::FunctionCallbackInfo<v8::Value>& info)
// {
//   v8::HandleScope scope(isolate);

//   std::string derPublicKey(convBack<std::string>(info[0]));

//   std::vector<std::uint8_t> derPublicKeyData(derPublicKey.data(),
//     derPublicKey.data() + derPublicKey.length());

//   mist::Peer& peer = connCtx->addAuthenticatedPeer(derPublicKeyData);

//   v8::Local<v8::Object> nodePeer = Peer::object(peer);

//   info.GetReturnValue().Set(nodePeer);
// }

// void
// onionAddress(const Nan::FunctionCallbackInfo<v8::Value>& info)
// {
//   v8::HandleScope scope(isolate);

//   try {
//     auto func = info[0].As<v8::Function>();
//     connCtx->onionAddress(makeAsyncCallback<const std::string&>(func));
//   } catch (boost::exception &) {
//     std::cerr
//       << "Unexpected exception, diagnostic information follows:" << std::endl
//       << boost::current_exception_diagnostic_information();
//   }
// }

void
Init(v8::Local<v8::Object> target, v8::Local<v8::Object> module)
{
  isolate = target->GetIsolate();
  v8::HandleScope scope(isolate);

  Nan::Set(target, Nan::New(ServiceWrap::ClassName()).ToLocalChecked(),
	   Nan::GetFunction(ServiceWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(PeerWrap::ClassName()).ToLocalChecked(),
	   Nan::GetFunction(PeerWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(ClientStreamWrap::ClassName()).ToLocalChecked(),
	   Nan::GetFunction(ClientStreamWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(ClientRequestWrap::ClassName()).ToLocalChecked(),
	   Nan::GetFunction(ClientRequestWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(ClientResponseWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(ClientResponseWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(ServerStreamWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(ServerStreamWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(ServerRequestWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(ServerRequestWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(ServerResponseWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(ServerResponseWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(CentralWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(CentralWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(SHA3Wrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(SHA3Wrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(SHA3HasherWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(SHA3HasherWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(PublicKeyWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(PublicKeyWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(PrivateKeyWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(PrivateKeyWrap::Init()).ToLocalChecked());

  Nan::Set(target, Nan::New(DatabaseWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(DatabaseWrap::Init()).ToLocalChecked());
  // TODO: Find out how to make these classes properties
  // of DatabaseWrap, and remove them from here.
  Nan::Set(target, Nan::New(ManifestWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(ManifestWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(ObjectRefWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(ObjectRefWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(MistObjectWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(MistObjectWrap::Init()).ToLocalChecked());
  Nan::Set(target, Nan::New(QueryResultWrap::ClassName()).ToLocalChecked(),
    Nan::GetFunction(QueryResultWrap::Init()).ToLocalChecked());

  // Nan::Set(target, Nan::New("initializeNSS").ToLocalChecked(),
  //   Nan::GetFunction(Nan::New<v8::FunctionTemplate>(initializeNSS)).ToLocalChecked());
  // Nan::Set(target, Nan::New("loadPKCS12").ToLocalChecked(),
  //   Nan::GetFunction(Nan::New<v8::FunctionTemplate>(loadPKCS12)).ToLocalChecked());
  // Nan::Set(target, Nan::New("loadPKCS12File").ToLocalChecked(),
  //   Nan::GetFunction(Nan::New<v8::FunctionTemplate>(loadPKCS12File)).ToLocalChecked());
  // Nan::Set(target, Nan::New("serveDirect").ToLocalChecked(),
  //   Nan::GetFunction(Nan::New<v8::FunctionTemplate>(serveDirect)).ToLocalChecked());
  // Nan::Set(target, Nan::New("startServeTor").ToLocalChecked(),
  //   Nan::GetFunction(Nan::New<v8::FunctionTemplate>(startServeTor)).ToLocalChecked());
  // Nan::Set(target, Nan::New("onionAddress").ToLocalChecked(),
  //   Nan::GetFunction(Nan::New<v8::FunctionTemplate>(onionAddress)).ToLocalChecked());

  // Nan::Set(target, Nan::New("addAuthenticatedPeer").ToLocalChecked(),
  //   Nan::GetFunction(Nan::New<v8::FunctionTemplate>(addAuthenticatedPeer)).ToLocalChecked());
  // Nan::Set(target, Nan::New("connectPeerDirect").ToLocalChecked(),
  //   Nan::GetFunction(Nan::New<v8::FunctionTemplate>(connectPeerDirect)).ToLocalChecked());
  // Nan::Set(target, Nan::New("connectPeerTor").ToLocalChecked(),
  //   Nan::GetFunction(Nan::New<v8::FunctionTemplate>(connectPeerTor)).ToLocalChecked());

}

NODE_MODULE(_mist_conn, Init)

} // namespace Node
} // namespace Mist
