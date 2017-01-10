/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <limits>
#include <random>

#include "CryptoHelper.h"
#include "Central.h"

#include <crypto/hash.hpp>
#include <crypto/key.hpp>
#include <crypto/random.hpp>

unsigned long long Mist::CryptoHelper::cryptoRandom() {
    //return mist::crypto::getRandomUInt53(); // TODO: fix this and  use it instead

    static std::random_device rd;
    static std::mt19937 gen( rd() );
    static std::uniform_int_distribution<unsigned> dis( 0, std::numeric_limits<unsigned>::max() );
    return dis( gen );
}

//
// SHA3
//
Mist::CryptoHelper::SHA3::SHA3() {
    hash.reset();
}

Mist::CryptoHelper::SHA3::SHA3( const SHA3& rhs ) {
    *this = rhs;
}

Mist::CryptoHelper::SHA3::SHA3( const std::string& str ) {
    if ( str.empty() ) {
        hash.reset();
    } else {
        hash.reset( mist::crypto::stringToBuffer( str ) );
    }
}

Mist::CryptoHelper::SHA3::SHA3( const std::vector<std::uint8_t>& buf )
    : hash(buf) {
}

Mist::CryptoHelper::SHA3
Mist::CryptoHelper::SHA3::fromBuffer( const std::vector<std::uint8_t>& buf ) {
    return SHA3(buf);
}

Mist::CryptoHelper::SHA3
Mist::CryptoHelper::SHA3::fromString( const std::string& str ) {
  return str.empty() ? SHA3() : SHA3( mist::crypto::base64Decode(str) );
}

Mist::CryptoHelper::SHA3&
Mist::CryptoHelper::SHA3::operator=(const Mist::CryptoHelper::SHA3& rhs) {
    if ( rhs.hash && !rhs.hash->empty() ) {
        hash.reset( *rhs.hash );
    } else {
        hash.reset();
    }
    return *this;
}

bool Mist::CryptoHelper::SHA3::operator<(const SHA3& rhs) const {
    if ( !hash && !rhs.hash ) {
        return false;
    } else if ( !hash ) {
        return true;
    } else if ( !rhs.hash ) {
        return false;
    } else {
        return *hash < *rhs.hash;
    }
}

bool Mist::CryptoHelper::SHA3::operator==(const SHA3& rhs) const {
    if ( !hash && !rhs.hash ) {
        return true;
    } else if ( !hash || !rhs.hash ) {
        return false;
    } else {
        return *hash == *rhs.hash;
    }
}

std::string
Mist::CryptoHelper::SHA3::toString() const {
    if ( hash && !hash->empty() ) {
        return mist::crypto::base64Encode(*hash);
    } else {
        return std::string{};
    }
}

const std::uint8_t*
Mist::CryptoHelper::SHA3::data() const {
    if ( hash ) {
        return hash->data();
    } else {
        return nullptr;
    }
}

std::size_t
Mist::CryptoHelper::SHA3::size() const {
    if ( hash ) {
        return hash->size();
    } else {
        return 0;
    }
}

//
// SHA3Hasher
//
Mist::CryptoHelper::SHA3Hasher::SHA3Hasher() {
    reset();
}

void
Mist::CryptoHelper::SHA3Hasher::reset() {
    hasher.reset();
    hasher = std::move( mist::crypto::hash_sha3_256() );
}

void
Mist::CryptoHelper::SHA3Hasher::update(const std::uint8_t* begin,
    const std::uint8_t* end) {
    if ( hasher ) {
        hasher->update(begin, end);
    }
}

void
Mist::CryptoHelper::SHA3Hasher::update(const std::string& data) {
    if ( data.empty() ) {
        return;
    } else {
    hasher->update(reinterpret_cast<const std::uint8_t*>(data.data()),
        reinterpret_cast<const std::uint8_t*>(data.data() + data.size()));
    }
}

Mist::CryptoHelper::SHA3
Mist::CryptoHelper::SHA3Hasher::finalize() {
    if ( hasher ) {
        auto data(hasher->finalize());
        hasher.reset();
        return Mist::CryptoHelper::SHA3(
                std::string(data.data(), data.data() + data.size()));
    } else {
        return SHA3();
    }
}

//
// PublicKey
//

Mist::CryptoHelper::PublicKey::PublicKey() {
    publicKey = {};
}

Mist::CryptoHelper::PublicKey::PublicKey( const std::vector<std::uint8_t>& derKey ) :
        publicKey(derKey) {
}

Mist::CryptoHelper::PublicKey
Mist::CryptoHelper::PublicKey::fromPem( const std::string& pemKey ) {
    return pemKey.empty() ? PublicKey() : PublicKey(mist::crypto::publicKeyPemToDer(pemKey));
}

Mist::CryptoHelper::PublicKey
Mist::CryptoHelper::PublicKey::fromDer( const std::vector<std::uint8_t>& derKey ) {
    return PublicKey(derKey);
}

std::string
Mist::CryptoHelper::PublicKey::toString() const {
    return publicKey.empty() ? std::string() : mist::crypto::publicKeyDerToPem(publicKey);
}

std::vector<std::uint8_t>
Mist::CryptoHelper::PublicKey::toDer() const {
    return publicKey;
}

Mist::CryptoHelper::PublicKeyHash
Mist::CryptoHelper::PublicKey::hash() const {
    Mist::CryptoHelper::SHA3Hasher hasher;
    hasher.update(publicKey.data(), publicKey.data() + publicKey.size());
    return hasher.finalize();
}

std::string
Mist::CryptoHelper::PublicKey::fingerprint() const
{
    auto sha3Hash(hash());
    std::vector<std::uint8_t> buffer(sha3Hash.data(),
        sha3Hash.data() + sha3Hash.size());
    return mist::crypto::base64Encode(buffer);
}

Mist::CryptoHelper::PublicKey&
Mist::CryptoHelper::PublicKey::operator=(const Mist::CryptoHelper::PublicKey& rhs) {
    this->publicKey = rhs.publicKey;
    return *this;
}

bool
Mist::CryptoHelper::PublicKey::operator==(const Mist::CryptoHelper::PublicKey& rhs) const {
    if ( this->publicKey.empty() || rhs.publicKey.empty() ) {
        return false;
    }
    return this->publicKey == rhs.publicKey;
}

//
// Private Key
//
Mist::CryptoHelper::PrivateKey::PrivateKey() :
        mCentral( boost::none ) {
}

Mist::CryptoHelper::PrivateKey::PrivateKey( const PrivateKey& rhs ) :
        mCentral( rhs.mCentral ) {
}

Mist::CryptoHelper::PrivateKey::PrivateKey( const Mist::Central& central ) :
        mCentral( central ) {
}

Mist::CryptoHelper::PrivateKey&
Mist::CryptoHelper::PrivateKey::operator=( const PrivateKey& rhs ) {
    mCentral = rhs.mCentral;
    return *this;
}

const Mist::CryptoHelper::PublicKey&
Mist::CryptoHelper::PrivateKey::getPublicKey() const {
    assert(mCentral);
    return mCentral->getPublicKey();
}

//
// Signature
//
Mist::CryptoHelper::Signature::Signature() {
    signature.reset();
}
 
Mist::CryptoHelper::Signature::Signature( const Signature& other ) {
    if ( other.signature ) {
        this->signature.reset( *other.signature );
    } else {
        this->signature.reset();
    }
}

Mist::CryptoHelper::Signature::Signature( const std::string& str ) {
    if ( str.empty() ) {
        signature.reset();
    } else {
        signature.reset( mist::crypto::base64Decode(str) );
    }
}

Mist::CryptoHelper::Signature::Signature( const std::vector<std::uint8_t>& buf ) :
    signature( buf ) {
}

Mist::CryptoHelper::Signature&
Mist::CryptoHelper::Signature::operator=( const Signature& rhs ) {
    if ( rhs.signature ) {
        signature.reset( *rhs.signature );
    } else {
        signature.reset();
    }
    return *this;
}

Mist::CryptoHelper::Signature
Mist::CryptoHelper::Signature::fromString( const std::string& str ) {
    return Signature(str);
}

Mist::CryptoHelper::Signature
Mist::CryptoHelper::Signature::fromBuffer( const std::vector<std::uint8_t>& buf ) {
    return Signature(buf);
}

std::string Mist::CryptoHelper::Signature::toString() const {
    if ( signature && !signature->empty() ) {
        return mist::crypto::base64Encode(*signature);
    } else {
        return std::string{};
    }
}
 
const std::uint8_t*
Mist::CryptoHelper::Signature::data() const {
    if ( signature ) {
        return signature->data();
    }
    return nullptr;
}

std::size_t
Mist::CryptoHelper::Signature::size() const {
    if ( signature ) {
        return signature->size();
    }
    return 0;
}

