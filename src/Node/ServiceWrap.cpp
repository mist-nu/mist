#include "Node/PeerWrap.hpp"
#include "Node/ServiceWrap.hpp"

namespace Mist
{
namespace Node
{

v8::Local<v8::FunctionTemplate>
ServiceWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New(ClassName()).ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "setOnPeerConnectionStatus",
			  Method<&ServiceWrap::setOnPeerConnectionStatus>);
  Nan::SetPrototypeMethod(tpl, "setOnPeerRequest",
			  Method<&ServiceWrap::setOnPeerRequest>);
  Nan::SetPrototypeMethod(tpl, "submit",
			  Method<&ServiceWrap::submit>);
  
  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  return tpl;
}
  
void
ServiceWrap::New(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);
  
  if (info.IsConstructCall()) {
    // TODO:
    //std::string name(convBack<std::string>(info[0]));
    //ServiceWrap* obj = new ServiceWrap(connCtx->newService(name));
    // obj->Wrap(info.This());
    //info.GetReturnValue().Set(info.This());
  } else {
    isolate->ThrowException(v8::String::NewFromUtf8(isolate,
						    "This class cannot be constructed in this way"));
  }
}
  /*
  void setOnPeerConnectionStatus(peer_connection_status_callback cb);

  void setOnPeerRequest(peer_request_callback cb);


  void setOnWebSocket(peer_websocket_callback cb);

  void openWebSocket(Peer& peer, std::string path,
    peer_websocket_callback cb);*/

void
ServiceWrap::setOnPeerConnectionStatus(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  auto func = info[0].As<v8::Function>();
  
  self().setOnPeerConnectionStatus(makeAsyncCallback<mist::Peer&, mist::Peer::ConnectionStatus>(func));
}

void
ServiceWrap::setOnPeerRequest(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  auto func = info[0].As<v8::Function>();
  
  self().setOnPeerRequest(
			  makeAsyncCallback<mist::Peer&, mist::h2::ServerRequest,
			  std::string>(func));
}
  
void
ServiceWrap::submit(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    v8::HandleScope scope(isolate);

    mist::Peer& peer = PeerWrap::self(info[0].As<v8::Object>());
    std::string method = convBack<std::string>(info[1]);
    std::string path = convBack<std::string>(info[2]);
    auto func = info[3].As<v8::Function>();

    self().submitRequest(peer, method, path,
      makeAsyncCallback<mist::Peer&, mist::h2::ClientRequest>(func));
}

} // namespace Node
} // namespace Mist
