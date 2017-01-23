/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <list>

#include <base64.h>

#include <prtypes.h>
#include <prio.h>
#include <pk11func.h>
#include <pk11priv.h>
#include <pk11pub.h>
#include <pkcs12.h>
#include <p12.h>
#include <p12plcy.h>

#include <nss.h>
#include <ssl.h>
#include <sslproto.h>
#include <cert.h>

#include <boost/filesystem.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/optional.hpp>
#include <boost/random/random_device.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include "error/nss.hpp"
#include "error/mist.hpp"

#include "memory/nss.hpp"

#include "crypto/hash.hpp"

namespace mist
{
namespace nss
{

c_unique_ptr<SECItem> toSecItem(const std::vector<std::uint8_t>& v)
{
  c_unique_ptr<SECItem> item = to_unique(SECITEM_AllocItem(nullptr, nullptr,
    boost::numeric_cast<unsigned int>(v.size())));
  item->len = boost::numeric_cast<unsigned int>(v.size());
  std::copy(v.begin(), v.end(), item->data);
  return item;
}

namespace
{

std::string
to_hex(std::uint8_t byte)
{
  static const char* digits = "0123456789abcdef";
  std::array<char, 2> text{ digits[byte >> 4], digits[byte & 0xf] };
  return std::string(text.begin(), text.end());
}

template<typename It>
std::string
to_hex(It begin, It end)
{
  std::string text;
  while (begin != end)
    text += to_hex(static_cast<std::uint8_t>(*(begin++)));
  return text;
}

std::string
to_hex(SECItem* item)
{
  return to_hex(reinterpret_cast<std::uint8_t*>(item->data),
    reinterpret_cast<std::uint8_t*>(item->data + item->len));
}

std::string
to_hex(std::string str)
{
  return to_hex(reinterpret_cast<const std::uint8_t*>(str.data()),
    reinterpret_cast<const std::uint8_t*>(str.data() + str.size()));
}

std::string
generateRandomId(std::size_t numDwords)
{
  std::vector<std::uint32_t> out(numDwords);
  boost::random::random_device rng;
  rng.generate(out.begin(), out.end());
  return std::string(reinterpret_cast<const char *>(out.data()),
    4 * out.size());
}

std::string
generateRandomHexString(std::size_t numBytes)
{
  return to_hex(generateRandomId((numBytes + 3) / 4)).substr(0, 2 * numBytes);
}

unsigned int _nextInstanceIndex = 0;
unsigned int _nextTempNicknameIndex = 0;

} // namespace

unsigned int
makeInstanceIndex()
{
  return _nextInstanceIndex++;
}

unsigned int
makeTempNicknameIndex()
{
  return _nextTempNicknameIndex++;
}

std::string
createTemporaryNickname()
{
  return "tmp_" + std::to_string(makeTempNicknameIndex());
}

namespace
{
void
clearDatabaseDirectory(const std::string& dbdir, c_unique_ptr<PRDir> dir)
{
  std::vector<std::string> filesToDelete;
  while (auto entry = PR_ReadDir(dir.get(), PR_SKIP_BOTH)) {
    std::string filename(entry->name);
    if (filename == "secmod.db"
      || filename == "key3.db"
      || filename == "cert8.db") {
      std::string fullName
        = (boost::filesystem::path(dbdir) / filename).string();
      filesToDelete.push_back(fullName);
    }
  }
  for (auto& fileToDelete : filesToDelete) {
    if (PR_Delete(fileToDelete.c_str()) != PR_SUCCESS)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to delete old database files"));
  }
}

void
initializeDatabaseDirectory(const std::string& dbdir)
{
  auto dir = to_unique(PR_OpenDir(dbdir.c_str()));
  if (dir) {
    clearDatabaseDirectory(dbdir, std::move(dir));
  } else {
    // TODO: Check if the error really is DIR DOES NOT EXIST
    if (PR_MkDir(dbdir.c_str(), 00700) != PR_SUCCESS)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to create database directory"));
  }
}

std::string slotPassword;

} // namespace

/* Initialize NSS with the given database directory */
void
initializeNSS(const std::string& dbdir)
{
  if (NSS_IsInitialized() == PR_TRUE)
    return;

  initializeDatabaseDirectory(dbdir);

  if (NSS_InitReadWrite(dbdir.c_str()) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to initialize NSS"));

  slotPassword = "mist"; //generateRandomHexString(32);

  /* Set the password function to delegate to SSLContext */
  PK11_SetPasswordFunc(
    [](PK11SlotInfo* info, PRBool retry, void* arg) -> char *
  {
    if (!retry) {
      return PL_strdup(slotPassword.c_str());
    } else {
      return nullptr;
    }
  });

  /* This is mandatory, otherwise NSS will crash by trying to access freed
  memory when using PKCS12 functions */
  SEC_PKCS12EnableCipher(PKCS12_RC4_40, 1);
  SEC_PKCS12EnableCipher(PKCS12_RC4_128, 1);
  SEC_PKCS12EnableCipher(PKCS12_RC2_CBC_40, 1);
  SEC_PKCS12EnableCipher(PKCS12_RC2_CBC_128, 1);
  SEC_PKCS12EnableCipher(PKCS12_DES_56, 1);
  SEC_PKCS12EnableCipher(PKCS12_DES_EDE3_168, 1);
  SEC_PKCS12SetPreferredCipher(PKCS12_DES_EDE3_168, 1);

  PORT_SetUCS2_ASCIIConversionFunction(
    [](PRBool toUnicode, unsigned char *inBuf, unsigned int inBufLen,
      unsigned char *outBuf, unsigned int maxOutBufLen,
      unsigned int *outBufLen, PRBool swapBytes) -> PRBool
  {
    if (!PORT_UCS2_UTF8Conversion(toUnicode, inBuf, inBufLen, outBuf,
      maxOutBufLen, outBufLen))
      return PR_FALSE;
    if (swapBytes) {
      std::uint16_t* ucs2Data = reinterpret_cast<std::uint16_t*>(outBuf);
      std::size_t ucs2Len = *outBufLen >> 1;
      for (std::size_t n = 0; n < ucs2Len; ++n) {
        ucs2Data[n] = ((ucs2Data[n] & 0xFF) << 8) | (ucs2Data[n] >> 8);
      }
    }
    return PR_TRUE;
  });
}

namespace
{

/* Get the internal slot where we store all our keys */
c_unique_ptr<PK11SlotInfo>
internalSlot()
{
  auto slot = to_unique(PK11_GetInternalKeySlot());
  if (PK11_NeedUserInit(slot.get())) {
    PK11_InitPin(slot.get(), static_cast<char*>(nullptr),
      slotPassword.c_str());
  } else {
    if (PK11_Authenticate(slot.get(), PR_TRUE, nullptr) != SECSuccess)
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_ASSERTION),
        "Unable to authenticate to slot"));
  }
  return std::move(slot);
}

c_unique_ptr<SECItem>
makeSecItem(const std::uint8_t* data, std::size_t length,
  SECItemType type = siBuffer)
{
  auto item = c_unique_ptr<SECItem>(
    reinterpret_cast<SECItem*>(PORT_ZAlloc(sizeof(SECItem))));
  item->type = type;
  item->len = boost::numeric_cast<unsigned int>(length);
  item->data = static_cast<unsigned char*>(PORT_Alloc(length));
  std::copy(data, data + length, item->data);
  return item;
}

c_unique_ptr<SECItem>
stringToUnicodeItem(const std::string& in)
{
  // Convert the password to Unicode...
  // TODO: Looking at the implementation,
  // it seems that there is already a bit of Unicode conversion going on:
  // https://dxr.mozilla.org/mozilla-central/source/security/nss/lib/pkcs12/p12d.c?q=SEC_PKCS12DecoderStart&redirect_type=direct#1201
  bool toUnicode = true;
  bool swapBytes = false;

#ifdef IS_LITTLE_ENDIAN
  //swapBytes = true;
#endif

  const std::size_t strBufLen = in.size() + 1;
  unsigned char* strBufData
    = reinterpret_cast<unsigned char*>(const_cast<char*>(in.c_str()));

  const std::size_t encBufLen = toUnicode ? 4 * strBufLen : strBufLen;
  std::vector<unsigned char> encBuf(encBufLen);
  unsigned int outLength;

  if (PORT_UCS2_UTF8Conversion(toUnicode, strBufData,
    boost::numeric_cast<unsigned int>(strBufLen), encBuf.data(),
    boost::numeric_cast<unsigned int>(encBufLen), &outLength)
    == PR_FALSE)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to convert password"));

  if (swapBytes) {
    std::uint16_t* ucs2Data = reinterpret_cast<std::uint16_t*>(encBuf.data());
    std::size_t ucs2Len = outLength >> 1;
    for (std::size_t n = 0; n < ucs2Len; ++n) {
      ucs2Data[n] = ((ucs2Data[n] & 0xFF) << 8) | (ucs2Data[n] >> 8);
    }
  }

  auto pwitem = makeSecItem(encBuf.data(), outLength);

  assert(pwitem);

  return std::move(pwitem);
}

c_unique_ptr<SECItem>
stringToAsciiItem(const std::string& in)
{
  auto item = to_unique(SECITEM_AllocItem(nullptr, nullptr,
    boost::numeric_cast<unsigned int>(in.length() + 1)));
  std::copy(in.data(), in.data() + in.length(), item->data);
  item->data[in.length()] = 0;
  return item;
}

c_unique_ptr<SEC_PKCS12DecoderContext>
tryInitDecode(const std::string& data, SECItem* pwitem, PK11SlotInfo* slot,
  void* wincx)
{
  auto p12dcx = to_unique(SEC_PKCS12DecoderStart(pwitem, slot, wincx,
    nullptr, nullptr, nullptr, nullptr, nullptr));

  if (!p12dcx)
    return nullptr;

  if (SEC_PKCS12DecoderUpdate(p12dcx.get(),
    reinterpret_cast<unsigned char*>(const_cast<char*>(data.data())),
    boost::numeric_cast<unsigned long>(data.size())) != SECSuccess)
    return nullptr;

  if (SEC_PKCS12DecoderVerify(p12dcx.get()) != SECSuccess)
    return nullptr;

  return std::move(p12dcx);
}

} // namespace

void
importPKCS12(const std::string& nickname,
  const std::string& data, const std::string& password)
{
  auto slot = internalSlot();

  c_unique_ptr<SECItem> pwitem;
  {
    pwitem = stringToUnicodeItem(password);
    if (!pwitem)
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_ASSERTION),
        "Could not convert password to Unicode"));
  }

  c_unique_ptr<SEC_PKCS12DecoderContext> p12dcx;
  {
    p12dcx = tryInitDecode(data, pwitem.get(), slot.get(), nullptr);
    if (!p12dcx && pwitem->len == 2) {
      pwitem->len = 0;
      p12dcx = tryInitDecode(data, pwitem.get(), slot.get(), nullptr);
    }
    if (!p12dcx)
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_ASSERTION),
        "Unable to decode"));
  }

  /* Rename keys and certificates */
  {
    std::string arg = nickname;
    using argType = decltype(arg);

    auto rv = SEC_PKCS12DecoderRenameCertNicknames(p12dcx.get(),
      [](const CERTCertificate *cert, const SECItem *default_nickname,
        SECItem **new_nickname, void *argPtr)
    {
      auto& arg = *reinterpret_cast<argType*>(argPtr);
      SECItem *nick = nullptr;
      {
        const std::size_t stringBufferSize = arg.size() + 1;
        const char* stringBuffer = arg.c_str();

        nick = SECITEM_AllocItem(nullptr, nullptr,
          boost::numeric_cast<unsigned int>(stringBufferSize));
        assert(nick->len == stringBufferSize);
        nick->type = siAsciiString;
        std::copy(stringBuffer, stringBuffer + stringBufferSize,
          nick->data);
      }
      *new_nickname = nick;
      return SECSuccess;
    }, &arg);

    if (rv != SECSuccess)
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_ASSERTION),
        "Unable to rename key and certificate"));
  }

  /* Validate */
  {
    auto rv = SEC_PKCS12DecoderValidateBags(p12dcx.get(),
      [](SECItem *old_nickname, PRBool *cancel, void *arg)
    {
      *cancel = PR_TRUE;
      return static_cast<SECItem*>(nullptr);
    });

    if (rv != SECSuccess)
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_ASSERTION),
        "PKCS12 decode validate bags failed"));
  }

  /* Import */
  {
    if (SEC_PKCS12DecoderImportBags(p12dcx.get()) != SECSuccess)
      BOOST_THROW_EXCEPTION(boost::system::system_error(
        make_mist_error(MIST_ERR_ASSERTION),
        "PKCS12 decode import bags failed"));
  }
}

void
importPKCS12File(const std::string& nickname,
  const std::string& path, const std::string& password)
{
  c_unique_ptr<PRFileDesc> fd;
  {
    fd = PR_Open(path.c_str(), PR_RDONLY, 0);
    if (!fd)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to open PKCS12 file"));
  }

  std::string data;
  {
    std::array<std::uint8_t, 1024> buf;
    while (1) {
      auto n = PR_Read(fd.get(), buf.data(),
        boost::numeric_cast<PRInt32>(buf.size()));
      if (n == 0)
        break;
      else if (n < 0)
        BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
          "Unable to read PKCS12 file"));
      else
        data.insert(data.end(), buf.data(), buf.data() + n);
    }
  }

  importPKCS12(nickname, data, password);
}

void
importCert(const std::string& nickname, CERTCertificate* cert)
{
  if (PK11_ImportCertForKeyToSlot(internalSlot().get(), cert,
    const_cast<char*>(nickname.c_str()), PR_FALSE, nullptr) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to import certificate"));
}

std::pair<c_unique_ptr<SECKEYPrivateKey>,
  c_unique_ptr<SECKEYPublicKey>>
  makeRsaKeyPair(PK11SlotInfo* slot, int size, int publicExponent,
    PK11AttrFlags attrFlags, CK_FLAGS opFlagsOn,
    CK_FLAGS opFlagsOff, void* wincx)
{
  /* TODO: Seed RNG */
  // UpdateRNG();

  /* RSA parameters */
  PK11RSAGenParams rsaparams;
  {
    rsaparams.keySizeInBits = size;
    rsaparams.pe = publicExponent;
  }

  SECKEYPublicKey* pubKey = nullptr;
  SECKEYPrivateKey* privKey = PK11_GenerateKeyPairWithOpFlags(slot,
    CKM_RSA_PKCS_KEY_PAIR_GEN, &rsaparams, &pubKey, attrFlags,
    opFlagsOn, opFlagsOn | opFlagsOff, wincx);

  if (!privKey || !pubKey)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to generate key pair"));

  return std::make_pair(to_unique(privKey), to_unique(pubKey));
}

std::pair<c_unique_ptr<SECKEYPrivateKey>, c_unique_ptr<SECKEYPublicKey>>
createRsaKeyPair(const std::string& nickname, int size)
{
  auto slot = internalSlot().get();
  int publicExponent = 65537;
  PK11AttrFlags attrFlags
    = PK11_ATTR_TOKEN | PK11_ATTR_SENSITIVE | PK11_ATTR_PRIVATE;
  CK_FLAGS opFlagsOn = 0;
  CK_FLAGS opFlagsOff = 0;

  /* TODO: Seed RNG */
  // UpdateRNG();

  /* RSA parameters */
  PK11RSAGenParams rsaparams;
  {
    rsaparams.keySizeInBits = size;
    rsaparams.pe = publicExponent;
  }

  SECKEYPublicKey* pubKey = nullptr;
  SECKEYPrivateKey* privKey = PK11_GenerateKeyPairWithOpFlags(slot,
    CKM_RSA_PKCS_KEY_PAIR_GEN, &rsaparams, &pubKey, attrFlags,
    opFlagsOn, opFlagsOn | opFlagsOff, nullptr);

  if (!privKey || !pubKey)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to generate key pair"));

  PK11_SetPrivateKeyNickname(privKey, const_cast<char*>(nickname.c_str()));
  PK11_SetPublicKeyNickname(pubKey, const_cast<char*>(nickname.c_str()));

  return std::make_pair(to_unique(privKey), to_unique(pubKey));
}

void
deleteKeyAndCert(const std::string& nickname)
{
  /* Delete the keys and certificate from the database */
  auto cert = to_unique(PK11_FindCertFromNickname(nickname.c_str(), nullptr));
  if (!cert)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to find certificate"));

  if (PK11_DeleteTokenCertAndKey(cert.get(), nullptr) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to delete certificate"));
}

std::string
exportPKCS12(const std::string& nickname, SECOidTag cipher,
  SECOidTag certCipher, const std::string& filePassword)
{
  auto cert = to_unique(PK11_FindCertFromNickname(nickname.c_str(), nullptr));
  auto filePwItem = stringToAsciiItem(filePassword);

  PK11SlotInfo* slot = cert->slot;
  if (!slot)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Certificate does not have a slot"));

  auto p12ecx = to_unique(
    SEC_PKCS12CreateExportContext(nullptr, nullptr, slot, nullptr));
  if (!p12ecx)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Export context creation failed"));

  if (SEC_PKCS12AddPasswordIntegrity(p12ecx.get(), filePwItem.get(),
    SEC_OID_SHA1) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "PKCS12 add password integrity failed"));

  SEC_PKCS12SafeInfo* keySafe = SEC_PKCS12CreateUnencryptedSafe(p12ecx.get());
  SEC_PKCS12SafeInfo* certSafe;
  if (certCipher == SEC_OID_UNKNOWN) {
    certSafe = keySafe;
  } else {
    certSafe = SEC_PKCS12CreatePasswordPrivSafe(p12ecx.get(), filePwItem.get(),
      certCipher);
  }

  if (!certSafe || !keySafe)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Key or certificate safe creation failed"));

  if (SEC_PKCS12AddCertAndKey(p12ecx.get(), certSafe, nullptr, cert.get(),
    CERT_GetDefaultCertDB(), keySafe, nullptr, PR_TRUE,
    filePwItem.get(), cipher) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to add certificate and key to PKCS12 context"));

  std::string out;
  {
    std::ostringstream os;
    auto stringWriter = [](void *arg, const char *buf, unsigned long len)
    {
      std::ostringstream& os
        = *reinterpret_cast<std::ostringstream*>(arg);
      os.write(buf, len);
    };
    if (SEC_PKCS12Encode(p12ecx.get(), stringWriter, &os)
      != SECSuccess)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to encode PKCS12"));
    out = os.str();
  }

  return out;
}

std::vector<std::uint8_t>
sign(SECKEYPrivateKey* privKey, const std::uint8_t* hashData,
  std::size_t length)
{
  /* Input hash item */
  const SECItem hashItem
    = { siBuffer, const_cast<unsigned char*>(hashData),
        boost::numeric_cast<unsigned int>(length) };

  /* Output signature item */
  c_unique_ptr<SECItem> signItem;
  {
    auto signLength = PK11_SignatureLen(privKey);
    if (signLength <= 0)
      BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
        "Unable to get signature length"));
    signItem = to_unique(SECITEM_AllocItem(nullptr, nullptr, signLength));
    assert(signItem->len == signLength);
  }

  /* Sign */
  if (PK11_Sign(privKey, signItem.get(), &hashItem) != SECSuccess)
    BOOST_THROW_EXCEPTION(boost::system::system_error(make_nss_error(),
      "Unable to sign"));

  return std::vector<std::uint8_t>(
    reinterpret_cast<std::uint8_t*>(signItem->data),
    reinterpret_cast<std::uint8_t*>(signItem->data) + signItem->len);
}

bool
verify(SECKEYPublicKey* pubKey,
  const std::uint8_t* hashData, std::size_t hashLength,
  const std::uint8_t* signData, std::size_t signLength)
{
  const SECItem hashItem
    = { siBuffer, const_cast<unsigned char*>(hashData),
        boost::numeric_cast<unsigned int>(hashLength) };
  const SECItem signItem
    = { siBuffer, const_cast<unsigned char*>(signData),
        boost::numeric_cast<unsigned int>(signLength) };
  return PK11_Verify(pubKey, &signItem, &hashItem, nullptr) == SECSuccess;
}

} // namespace nss
} // namespace mist
