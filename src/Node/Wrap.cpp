/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <nan.h>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/system/error_code.hpp>

#include "error/mist.hpp"

#include "Node/Wrap.hpp"

namespace Mist
{
namespace Node
{

namespace
{

class Sentinel : public Nan::ObjectWrap
{
public:

  static v8::Local<v8::Object> make()
  {
    if (_ctor.IsEmpty()) {
      v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(
        [](const Nan::FunctionCallbackInfo<v8::Value>& info) -> void
      {
	Sentinel* wrapper = new Sentinel(++_prevId);
	wrapper->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
      });
      tpl->SetClassName(Nan::New("Sentinel").ToLocalChecked());
      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      _ctor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    }
    
    v8::Local<v8::Function> ctor = Nan::New(_ctor);
    return ctor->NewInstance(0, nullptr);
  }

  int getId() const { return _id; }

private:
  
  Sentinel(int id) : _id(id) {}

  int _id;
  static int _prevId;
  static Nan::Persistent<v8::Function> _ctor;

};

int Sentinel::_prevId = 0;
  
Nan::Persistent<v8::Function> Sentinel::_ctor;

Nan::Persistent<v8::Object> _noCtor(Sentinel::make());

} // namespace

v8::Local<v8::Value> noCtorSentinel()
{
  v8::Local<v8::Object> r(Nan::New(_noCtor));
  return r.As<v8::Value>();
}

bool isNoCtorSentinel(v8::Local<v8::Value> obj)
{
  return obj.As<v8::Object>() == _noCtor;
}

bool convertToNodeException(const boost::system::system_error& e)
{
  Nan::HandleScope scope;
  if (e.code().category() == mist::mist_category()) {
    auto error = Nan::Error(e.code().message().c_str());
    Nan::Set(error.As<v8::Object>(), Nan::New("code").ToLocalChecked(),
      Nan::New(e.code().value()));
    Nan::ThrowError(error);
    return true;
  } else {
    return false;
  }
}

} // namespace Node
} // namespace Mist
