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

MistConnApi const boost::system::error_category& nss_category() noexcept;

/* Create a boost::system::error_code with the given error value */
MistConnApi boost::system::error_code make_nss_error(std::int32_t ev);

/* Use PR_GetError() to create a boost::system::error_code */
MistConnApi boost::system::error_code make_nss_error();

}
