#ifndef __MIST_DB_NODE_ADDON__
#define __MIST_DB_NODE_ADDON__

#include <node.h>
#include <node_object_wrap.h>

#include <string>

namespace mistStub {
  class MistDbs {
  public:
    MistDbs(std::string path) : path(path) {}

  private:
    std::string path;
  };

}

namespace mistNode
{

  class MistDBs : public node::ObjectWrap
  {
  public:
    static void Initialize(v8::Local<v8::Object> exports);

  private:
    explicit MistDBs(std::string path);
    ~MistDBs();

    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Create(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Init(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Close(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void GetDB(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void CreateDB(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ReceiveDB(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void ListDBs(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void GetContent(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void UploadContent(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void AddUser(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void UpdateAddressOfUser(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void RemoveUser(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void BlockUser(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void FindUser(const v8::FunctionCallbackInfo<v8::Value>& args);

    static v8::Persistent<v8::Function> constructor;

    mistStub::MistDbs realMistDbs;
  };

}

#endif
