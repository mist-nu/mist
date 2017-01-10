#ifndef __MIST_SRC_CRYPTO_NSS_HPP__
#define __MIST_SRC_CRYPTO_NSS_HPP__

#include <string>
#include <vector>

namespace mist
{
namespace nss
{

c_unique_ptr<SECItem> toSecItem(const std::vector<std::uint8_t>& v);

unsigned int makeInstanceIndex();

unsigned int makeTempNicknameIndex();

std::string createTemporaryNickname();

void importPKCS12(const std::string& nickname, const std::string& data,
  const std::string& password);

void importPKCS12File(const std::string& nickname, const std::string& path,
  const std::string& password);

std::string exportPKCS12(const std::string& nickname, SECOidTag cipher,
  SECOidTag certCipher, const std::string& filePassword);

std::pair<c_unique_ptr<SECKEYPrivateKey>, c_unique_ptr<SECKEYPublicKey>>
  createRsaKeyPair(const std::string& nickname, int size);

void importCert(const std::string& nickname, CERTCertificate* cert);

void deleteKeyAndCert(const std::string& nickname);

/* Initialize NSS with the given database directory */
void initializeNSS(const std::string& dbdir);

/* Makes sure that the internal slot is initialized and returns it */
c_unique_ptr<PK11SlotInfo> internalSlot();

std::vector<std::uint8_t> sign(SECKEYPrivateKey* privKey,
  const std::uint8_t* hashData, std::size_t length);

bool verify(SECKEYPublicKey* pubKey,
  const std::uint8_t* hashData, std::size_t hashLength,
  const std::uint8_t* signData, std::size_t signLength);

} // namespace nss
} // namespace mist

#endif
