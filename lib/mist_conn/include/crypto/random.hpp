/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <cstddef>

namespace mist
{
namespace crypto
{

MistConnApi std::uint64_t getRandomUInt53();
MistConnApi double getRandomDouble53();

} // namespace crypto
} // namespace mist
