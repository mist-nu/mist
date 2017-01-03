/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/optional.hpp>

#include <nghttp2/nghttp2.h>

namespace mist
{
namespace h2
{

/*
 * Helper alias whose ::value is true if T is char-like
 */
template<typename T>
using is_char_type = std::is_same<char,
  typename std::make_signed<
    typename std::remove_cv<T>::type>::type>;

/* 
 * Since boost::lexcial_cast does not work for char types (std::uint8_t etc.),
 * we need to treat this as a special case. TODO: Test
 */
template<typename T, typename std::enable_if<is_char_type<T>::value>::type* = nullptr>
boost::optional<T>
parseDecimal(const std::string &value) {
  try {
    int intermediateValue = boost::lexical_cast<int>(value);
    try {
      return boost::numeric_cast<T>(intermediateValue);
    } catch(boost::numeric::negative_overflow &) {
      return boost::none;
    } catch(boost::numeric::positive_overflow &) {
      return boost::none;
    }
  } catch(boost::bad_lexical_cast &) {
    return boost::none;
  }
}

/*
 * Default to parse directly with boost::lexical_cast.
 */
template<typename T, typename std::enable_if<!is_char_type<T>::value>::type* = nullptr>
boost::optional<T>
parseDecimal(const std::string &value) {
  try {
    return boost::lexical_cast<T>(value);
  } catch(boost::bad_lexical_cast &) {
    return boost::none;
  }
}

std::vector<nghttp2_nv> makeHeaderNv(const header_map &headers);

} // namespace h2
} // namespace mist
