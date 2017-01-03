/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */
#pragma once

#include "mist_conn_api.hpp"

#include <string>
#include <vector>

namespace mist
{

namespace io
{

class SSLContext;

} // namespace io

namespace crypto
{

enum class ValidityTimeUnit {
  Sec, Min, Hour, Day, Month, Year
};

MistConnApi std::string
createP12TestRsaPrivateKeyAndCert(io::SSLContext& ctx,
  const std::string& password, const std::string& subject, int size,
  ValidityTimeUnit validityUnit, int validityNumber);

MistConnApi std::string
publicKeyDerToPem(const std::vector<std::uint8_t>& publicKey);

MistConnApi std::vector<std::uint8_t>
publicKeyPemToDer(const std::string& publicKey);

MistConnApi std::vector<std::uint8_t> base64Decode(const std::string& src);

MistConnApi std::string base64Encode(const std::vector<std::uint8_t>& src);

MistConnApi std::vector<std::uint8_t> stringToBuffer(const std::string& src);

MistConnApi std::string bufferToString(const std::vector<std::uint8_t>& src);

} // namespace crypto
} // namespace mist
