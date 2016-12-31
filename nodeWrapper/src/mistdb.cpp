#include "mistdb.h"

namespace mistNode
{
  using v8::Context;
  using v8::Function;
  using v8::FunctionCallbackInfo;
  using v8::FunctionTemplate;
  using v8::Isolate;
  using v8::Local;
  using v8::Number;
  using v8::Object;
  using v8::Persistent;
  using v8::String;
  using v8::Value;

  Persistent<Function> MistDBs::constructor;

  MistDBs::MistDBs(std::string path) {
    this->realMistDbs = new mistStub::MistDbs(path);
  }

  MistDBs::~MistDBs() {

  }

  void MistDBs::Initialize(Local<v8::Object> exports) {
    Isolate* isolate = exports->GetIsolate();

    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFormatUtf8(isolate, "MistDBs"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(tpl, "create", Create);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFormatUtf8(isolate, "MistDBs"), tpl->GetFunction());
  }

  void MistDBs::New(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.IsConstructCall()) {
	// Invoked as constructor: `new MistDBs(...)`
	std::string path = args[0]->IsUndefined() ? "" : args[0]);
	MistDBs* obj = new MistDBs(path);
	obj->Wrap(args.This());
	args.GetReturnValue().Set(args.This());
    } else {
	// Invoked as plain function.
	// TODO: implement this handling for this case.
    }
  }

  void MistDBs::Create(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::Init(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::Close(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::GetDB(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::CreateDB(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::ReceiveDB(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::ListDBs(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::GetContent(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::UploadContent(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::AddUser(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::UpdateAddressOfUser(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::RemoveUser(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::BlockUser(const FunctionCallbackInfo<Value>& args) {

  }

  void MistDBs::FindUser(const FunctionCallbackInfo<Value>& args) {

  }

}
