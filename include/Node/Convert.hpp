/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include <cstddef>
#include <string>
#include <type_traits>

#include <nan.h>

namespace Mist
{
namespace Node
{

extern v8::Isolate* isolate;

namespace detail
{

template<typename T, typename Enable = void>
struct NodeValueConverter
{
};

template<typename T, typename Enable = void>
struct NodeValueDecayHelper;

template<typename T>
struct NodeValueDecayHelper<T,
  typename std::enable_if<std::is_reference<T>::value>::type>
{
  using type = typename std::add_pointer<
    typename std::add_const<typename std::decay<T>::type>::type>::type;

  static type decay(T value) { return &value; }
};

template<typename T>
struct NodeValueDecayHelper<T,
  typename std::enable_if<!std::is_reference<T>::value>::type>
{
  using type = typename std::add_const<typename std::decay<T>::type>::type;
  
  static type decay(T value) { return value; }
};

template<typename T>
using node_decay_t = typename NodeValueDecayHelper<T>::type;

template<typename T>
node_decay_t<T> nodeDecay(T value)
{
  return NodeValueDecayHelper<T>::decay(value);
}

//
// std::string* => String
//
template<>
struct NodeValueConverter<const std::string*>
{
  static v8::Local<v8::Value> conv(const std::string* v)
  {
    return Nan::New(*v).ToLocalChecked();
  }
};

//
// std::string <=> String
//
template<>
struct NodeValueConverter<const std::string>
{
  static v8::Local<v8::Value> conv(const std::string& v)
  {
    return Nan::New(v).ToLocalChecked();
  }
  static std::string convBack(v8::Local<v8::Value> v)
  {
    v8::Local<v8::String> str(Nan::To<v8::String>(v).ToLocalChecked());
    return std::string(*v8::String::Utf8Value(str));
  }
};

//
// char* const => String
//
template<>
struct NodeValueConverter<const char* const>
{
  static v8::Local<v8::Value> conv(const char* const v)
  {
    return Nan::New(v).ToLocalChecked();
  }
};

//
// bool <=> Boolean
//
template<>
struct NodeValueConverter<bool>
{
  static v8::Local<v8::Value> conv(bool v)
  {
    return Nan::New(v);
  }
  static bool convBack(v8::Local<v8::Value> v)
  {
    return v->BooleanValue();
  }
};

//
// std::vector<std::uint8_t> <=> node::Buffer
//
template<>
struct NodeValueConverter<const std::vector<std::uint8_t>>
{
  static v8::Local<v8::Value> conv(const std::vector<std::uint8_t> v)
  {
    const char* data
      = reinterpret_cast<const char*>(v.data());
    std::size_t length = v.size();
    return node::Buffer::Copy(isolate, data, length)
      .ToLocalChecked();
  }
  static std::vector<std::uint8_t> convBack(v8::Local<v8::Value> v)
  {
    v8::Local<v8::Object> buffer = v.As<v8::Object>();
    assert(node::Buffer::HasInstance(buffer));
    const std::uint8_t* data
      = reinterpret_cast<std::uint8_t*>(node::Buffer::Data(buffer));
    std::size_t length = node::Buffer::Length(buffer);
    return std::vector<std::uint8_t>(data, data + length);
  }
};

//
// int... <=> Integer
//
template<typename T>
struct NodeValueConverter<T,
  typename std::enable_if<std::is_integral<T>::value>::type>
{
  static v8::Local<v8::Value> conv(T v)
  {
    return Nan::New(v);
  }
  static T convBack(v8::Local<v8::Value> v)
  {
    return v->IntegerValue();
  }
};

//
// double... <=> Number
//
template<typename T>
struct NodeValueConverter<T,
  typename std::enable_if<std::is_floating_point<T>::value>::type>
{
  static v8::Local<v8::Value> conv(T v)
  {
    return Nan::New(v);
  }
  static T convBack(v8::Local<v8::Value> v)
  {
    return v->NumberValue();
  }
};

} // namespace detail

template<typename T>
inline v8::Local<v8::Value> conv(T v)
{
  return detail::NodeValueConverter<detail::node_decay_t<T>>::conv(
    detail::nodeDecay<T>(v));
}

template<typename T>
inline T convBack(v8::Local<v8::Value> v)
{
  return detail::NodeValueConverter<detail::node_decay_t<T>>::convBack(v);
}

} // namespace Node
} // namespace Mist

