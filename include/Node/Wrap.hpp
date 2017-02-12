/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include <nan.h>
#include <array>

#include <iostream>

#include <boost/system/system_error.hpp>

#include "Node/Async.hpp"

namespace Mist
{
namespace Node
{

v8::Local<v8::Value> noCtorSentinel();
bool isNoCtorSentinel(v8::Local<v8::Value>);

extern v8::Isolate* isolate;

template<typename K, typename V>
class PersistentMap
{
public:
  using local_type = v8::Local<V>;
  using persistent_type = CopyablePersistent<V>;

private:
  static_assert(!std::is_reference<K>::value, "Key may not be reference");
  std::map<K, persistent_type> _m;

public:
  bool hasKey(K key)
  {
    //return false;
    return _m.find(key) != _m.end();
  }
  local_type operator[](K key)
  {
    //return local_type();
    return Nan::New<V>(_m[key]);
  }
  void insert(K key, local_type obj)
  {
    _m.insert(std::make_pair(key, persistent_type(obj)));
  }
};

namespace detail
{

template<typename T, typename Enable = void>
struct PointerTraits
{
  using type = T;
  static type getPointer(T ptr)
  {
    return ptr;
  }
};

// template<typename T>
// struct PointerTraits<T,
//   typename std::enable_if<!std::is_reference<T>::value
//                        && !std::is_pointer<T>::value>::type>
// {
//   using type = T;
//   static type getPointer(T ptr)
//   {
//     return ptr;
//   }
// };

template<typename T>
struct PointerTraits<T,
  typename std::enable_if<std::is_reference<T>::value>::type>
{
  using type = typename std::add_pointer<
    typename std::remove_reference<T>::type>::type;
  static type getPointer(T ptr)
  {
    return &ptr;
  }
};

template<typename T>
struct PointerTraits<T,
  typename std::enable_if<std::is_pointer<T>::value>::type>
{
  using type = T;
  static type getPointer(T ptr)
  {
    return ptr;
  }
};

template<typename T>
struct PointerTraits<std::shared_ptr<T>>
{
  using type = typename std::add_pointer<T>::type;
  static type getPointer(std::shared_ptr<T>& ptr)
  {
    return ptr.get();
  }
};

template<typename T>
struct PointerTraits<std::unique_ptr<T>>
{
  using type = typename std::add_pointer<T>::type;
  static type getPointer(std::unique_ptr<T>& ptr)
  {
    return ptr.get();
  }
};

} // namespace detail

static v8::Local<v8::FunctionTemplate>
defaultTemplate(const char* className)
{
  Nan::EscapableHandleScope scope;
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(
    [](const Nan::FunctionCallbackInfo<v8::Value>& info) -> void
  {
    if (!info.IsConstructCall()) {
      // Calling constructor without new
      isolate->ThrowException(v8::String::NewFromUtf8(isolate,
        "This class cannot be constructed in this way; use new"));
    } else if (info.Length() != 1 || !isNoCtorSentinel(info[0])) {
      // Trying to construct
      isolate->ThrowException(v8::String::NewFromUtf8(isolate,
        "This class cannot be instantiated directly"));
    }
  });
  tpl->SetClassName(Nan::New(className).ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  return scope.Escape(tpl);
}

template<typename T>
static v8::Local<v8::FunctionTemplate>
constructingTemplate(const char* className)
{
  Nan::EscapableHandleScope scope;
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(
    [](const Nan::FunctionCallbackInfo<v8::Value>& info) -> void
  {
    if (!info.IsConstructCall()) {
      // Calling constructor without new
      isolate->ThrowException(v8::String::NewFromUtf8(isolate,
        "This class cannot be constructed in this way; use new"));
    } else if (info.Length() != 1 || !isNoCtorSentinel(info[0])) {
      // Construct
      T* wrapper = new T(info);
      wrapper->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    }
  });
  tpl->SetClassName(Nan::New(className).ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  return scope.Escape(tpl);
}

bool convertToNodeException(const boost::system::system_error& e);

template<typename T, typename Ptr>
class NodeWrap : public Nan::ObjectWrap
{
protected:

  using pointer_type = typename detail::PointerTraits<Ptr>::type;
  using element_type = Ptr;

  friend v8::Local<v8::FunctionTemplate>
  constructingTemplate<T>(const char* className);

private:

  static Nan::Persistent<v8::Function> _ctor;
  Ptr _self;

protected:

  NodeWrap(Ptr s) : _self(std::forward<Ptr>(s)) {}

  inline static T* wrapper(v8::Local<v8::Object> obj) {
    return Nan::ObjectWrap::Unwrap<T>(obj);
  }

  using wrapped_method_type = void(T::*)
    (const Nan::FunctionCallbackInfo<v8::Value>& info);

  template<wrapped_method_type m>
  static void Method(
    const Nan::FunctionCallbackInfo<v8::Value>& info)
  {
    try {
      T* obj = ObjectWrap::Unwrap<T>(info.This());
      (obj->*m)(info);
    } catch (const boost::system::system_error& e) {
      if (!convertToNodeException(e))
        throw;
    } catch (const boost::exception& e) {
      std::cerr << "Exception in wrapped method: "
        << boost::diagnostic_information(e);
    } catch (const std::exception& e) {
      std::cerr << "Exception in wrapped method: "
        << boost::diagnostic_information(e);
    } catch (...) {
      std::cerr << "Exception in wrapped method" << std::endl;
    }
  }

  using wrapped_getter_type = void(T::*)
    (v8::Local<v8::String> name,
     const Nan::PropertyCallbackInfo<v8::Value>& info);

  template<wrapped_getter_type m>
  static void Getter(
    v8::Local<v8::String> name,
    const Nan::PropertyCallbackInfo<v8::Value>& info)
  {
    try {
      T* obj = ObjectWrap::Unwrap<T>(info.This());
      (obj->*m)(name, info);
    } catch (const boost::system::system_error& e) {
      if (!convertToNodeException(e))
        throw;
    } catch (boost::exception& e) {
      std::cerr << "Exception in wrapped getter: "
        << boost::diagnostic_information(e);
    } catch (std::exception& e) {
      std::cerr << "Exception in wrapped getter: "
        << boost::diagnostic_information(e);
    } catch (...) {
      std::cerr << "Exception in wrapped getter" << std::endl;
    }
  }

  using wrapped_setter_type = void(T::*)
    (v8::Local<v8::String> name,
     v8::Local<v8::Value> value,
     const Nan::PropertyCallbackInfo<void>& info);

  template<wrapped_setter_type m>
  static void Setter(
    v8::Local<v8::String> name,
    v8::Local<v8::Value> value,
    const Nan::PropertyCallbackInfo<void>& info)
  {
    try {
      T* obj = ObjectWrap::Unwrap<T>(info.This());
      (obj->*m)(name, value, info);
    } catch (const boost::system::system_error& e) {
      if (!convertToNodeException(e))
        throw;
    } catch (boost::exception& e) {
      std::cerr << "Exception in wrapped setter: "
        << boost::diagnostic_information(e);
    } catch (std::exception& e) {
      std::cerr << "Exception in wrapped setter: "
        << boost::diagnostic_information(e);
    } catch (...) {
      std::cerr << "Exception in wrapped setter" << std::endl;
    }
  }

  static Nan::Persistent<v8::Function> &constructor() { return _ctor; }

public:

  template<typename... Args>
  static v8::Local<v8::Object> make(Args&&... args) {
    Nan::EscapableHandleScope scope;
    v8::Local<v8::Function> ctor = Nan::New(constructor());
    std::array<v8::Local<v8::Value>, 1> ctorArgs{noCtorSentinel()};
    v8::Local<v8::Object> obj
      = Nan::NewInstance(ctor, ctorArgs.size(),
          ctorArgs.data()).ToLocalChecked();
    T* wrapper = new T(std::forward<Args>(args)...);
    wrapper->Wrap(obj);
    return scope.Escape(obj);
  }

  element_type& self() { return _self; }

  inline static element_type& self(v8::Local<v8::Object> obj) {
    return wrapper(obj)->self();
  }

  inline static element_type& self(v8::Local<v8::Value> val) {
    return wrapper(val.As<v8::Object>())->self();
  }

};

template<typename T, typename Ptr>
Nan::Persistent<v8::Function> NodeWrap<T, Ptr>::_ctor;

template<typename T, typename Ptr>
class NodeWrapSingleton : public NodeWrap<T, Ptr>
{
private:

  using typename NodeWrap<T, Ptr>::pointer_type;
  //static_assert(std::is_pointer<pointer_type>::value, "Oops");
  //static_assert(!std::is_reference<pointer_type>::value, "Oops");
  static PersistentMap<pointer_type, v8::Object> _objMap;

public:

  // TODO: Make use of already existing function handle
  inline v8::Local<v8::Object> object()
  {
    return _objMap[this->self()];
  }

  static v8::Local<v8::Object> object(Ptr key)
  {
    Nan::EscapableHandleScope scope;
    auto ptrKey = detail::PointerTraits<Ptr>::getPointer(key);
    if (!_objMap.hasKey(ptrKey)) {
      v8::Local<v8::Function> ctor = Nan::New(NodeWrap<T, Ptr>::constructor());
      std::array<v8::Local<v8::Value>, 1> ctorArgs{noCtorSentinel()};
      v8::Local<v8::Object> obj
        = ctor->NewInstance(ctorArgs.size(), ctorArgs.data());
      T* wrapper = new T(key);
      wrapper->Wrap(obj);
      _objMap.insert(ptrKey, obj);
    }
    return scope.Escape(_objMap[ptrKey]);
  }

protected:

  NodeWrapSingleton(Ptr s) : NodeWrap<T, Ptr>(std::forward<Ptr>(s)) {}

  void setObject(v8::Local<v8::Object> obj)
  {
    _objMap.insert(this->self(), obj);
  }

};

template<typename T, typename Ptr>
PersistentMap<typename NodeWrapSingleton<T, Ptr>::pointer_type, v8::Object>
NodeWrapSingleton<T, Ptr>::_objMap;

} // namespace Node
} // namespace Mist
