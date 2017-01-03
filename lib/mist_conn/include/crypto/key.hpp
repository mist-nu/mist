#ifndef __MIST_INCLUDE_CRYPTO_KEYGEN_HPP__
#define __MIST_INCLUDE_CRYPTO_KEYGEN_HPP__

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

std::string
createP12TestRsaPrivateKeyAndCert(io::SSLContext& ctx,
  const std::string& password, const std::string& subject, int size,
  ValidityTimeUnit validityUnit, int validityNumber);

std::string publicKeyDerToPem(const std::vector<std::uint8_t>& publicKey);

std::vector<std::uint8_t> publicKeyPemToDer(const std::string& publicKey);

std::vector<std::uint8_t> base64Decode(const std::string& src);

std::string base64Encode(const std::vector<std::uint8_t>& src);

std::vector<std::uint8_t> stringToBuffer(const std::string& src);

std::string bufferToString(const std::vector<std::uint8_t>& src);

} // namespace crypto
} // namespace mist

#endif
