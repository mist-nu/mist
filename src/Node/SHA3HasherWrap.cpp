/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "Node/SHA3HasherWrap.hpp"
#include "Node/SHA3Wrap.hpp"

namespace Mist
{
namespace Node
{

SHA3HasherWrap::SHA3HasherWrap()
  : NodeWrap(Mist::CryptoHelper::SHA3Hasher())
{
}

v8::Local<v8::FunctionTemplate> SHA3HasherWrap::Init()
{
  v8::Local<v8::FunctionTemplate> tpl = defaultTemplate(ClassName());

  Nan::SetPrototypeMethod(tpl, "reset",
			  Method<&SHA3HasherWrap::reset>);
  Nan::SetPrototypeMethod(tpl, "update",
			  Method<&SHA3HasherWrap::update>);
  Nan::SetPrototypeMethod(tpl, "finalize",
			  Method<&SHA3HasherWrap::finalize>);
  
  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  return tpl;
}

void
SHA3HasherWrap::reset(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  self().reset();
}

void
SHA3HasherWrap::update(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  if (info[0]->IsString()) {
    auto buf(convBack<std::string>(info[0]));
    const std::uint8_t* data
      = reinterpret_cast<const std::uint8_t*>(buf.data());
    std::size_t length = buf.length();
    self().update(data, data + length);
  } else {
    v8::Local<v8::Object> buffer = info[0].As<v8::Object>();
    const std::uint8_t* data
      = reinterpret_cast<std::uint8_t*>(node::Buffer::Data(buffer));
    std::size_t length = node::Buffer::Length(buffer);
    self().update(data, data + length);
  }
}

void
SHA3HasherWrap::finalize(const Nan::FunctionCallbackInfo<v8::Value>& info)
{
  Nan::HandleScope scope;
  info.GetReturnValue().Set(conv(self().finalize()));
}

} // namespace Node
} // namespace Mist
