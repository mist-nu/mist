#include "Node/PeerWrap.hpp"

namespace Mist
{
namespace Node
{

PeerWrap::PeerWrap(mist::Peer& peer)
  : NodeWrapSingleton(peer)
{
}

v8::Local<v8::FunctionTemplate> PeerWrap::Init()
{
    v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

    Nan::SetPrototypeMethod(tpl, "setAuthenticated",
      Method<&PeerWrap::setAuthenticated>);
    Nan::SetPrototypeMethod(tpl, "derPublicKey",
      Method<&PeerWrap::derPublicKey>);
    Nan::SetPrototypeMethod(tpl, "publicKeyHash",
      Method<&PeerWrap::publicKeyHash>);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

    return tpl;
}

void
PeerWrap::derPublicKey(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    v8::HandleScope scope(isolate);
    info.GetReturnValue().Set(conv(self().derPublicKey()));
}

void
PeerWrap::publicKeyHash(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
    v8::HandleScope scope(isolate);
    info.GetReturnValue().Set(conv(self().publicKeyHash()));
}

void
PeerWrap::setAuthenticated(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  v8::HandleScope scope(isolate);

  auto authenticated = convBack<bool>(info[0]);

  self().setAuthenticated(authenticated);
}

} // namespace Node
} // namespace Mist
