/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <cstddef>

#include <boost/system/error_code.hpp>

namespace mist
{

MistConnApi const boost::system::error_category &nghttp2_category() noexcept;

/* Creates a boost::system::error_code with the given error value */
MistConnApi boost::system::error_code make_nghttp2_error(std::uint32_t ev);

}
