/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

namespace mist
{

/* unique_facade */

template<typename T>
class unique_facade
{
public:

  T& impl() {}

private:

  std::unique_ptr<T> __impl;

};

template<typename T>
class unique_facade_impl
{
public:

  facade_impl() : __facade(*this) {}

  T& facade() { return __facade; }

private:

  T& __facade;

};

template<typename T>
class ref_facade_impl
{
public:

  facade_impl() : __facade(*this) {}

  T& facade() { return __facade; }

private:

  T __facade;

};

} // namespace mist
