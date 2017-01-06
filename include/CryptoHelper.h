/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef SRC_CRYPTOHELPER_H_
#define SRC_CRYPTOHELPER_H_

#include <cstddef>
#include <memory>
#include <string>

#include <boost/optional.hpp>

#include <crypto/hash.hpp>

namespace Mist {

class Central;
 
namespace CryptoHelper {

// TODO: Fix everything in this namespace

unsigned long long cryptoRandom();

class SHA3 {
public:
    SHA3();
    SHA3( const SHA3& rhs );
    explicit SHA3( const std::vector<std::uint8_t>& buf );
    explicit SHA3( const std::string& str );
    
    static SHA3 fromBuffer( const std::vector<std::uint8_t>& buf );
    static SHA3 fromString( const std::string& str );

    SHA3& operator=( const SHA3& rhs );
    bool operator<( const SHA3& rhs ) const;
    bool operator==( const SHA3& rhs ) const;

    std::string toString() const;
    const std::uint8_t* data() const;
    std::size_t size() const;
private:
    boost::optional<std::vector<std::uint8_t>> hash;
};

using PublicKeyHash = SHA3;

class SHA3Hasher {
public:
    SHA3Hasher();
    void reset();
    void update(const std::uint8_t* begin, const std::uint8_t* end);
    void update(const std::string& data);
    SHA3 finalize();
private:
    std::unique_ptr<mist::crypto::Hasher> hasher;
};

class Signature {
public:
    Signature();
    Signature( const Signature& );
    explicit Signature( const std::string& str );
    explicit Signature( const std::vector<std::uint8_t>& buf );
    Signature& operator=( const Signature& );

    static Signature fromString( const std::string& str );
    static Signature fromBuffer( const std::vector<std::uint8_t>& buf );
    
    std::string toString() const;
    const std::uint8_t* data() const;
    std::size_t size() const;
    
private:
    boost::optional<std::vector<std::uint8_t>> signature;
};

class PublicKey {
public:
    PublicKey();
    explicit PublicKey( const std::string& pemKey );
    explicit PublicKey( const std::vector<std::uint8_t>& derKey );
    static PublicKey fromPem( const std::string& pemKey );
    static PublicKey fromDer( const std::vector<std::uint8_t>& derKey );
    PublicKey& operator=( const PublicKey& rhs );
    bool operator==( const PublicKey& rhs ) const;

    //bool verify( const SHA3& hash, const Signature& sig ) const;
    
    PublicKeyHash hash() const;
    std::string fingerprint() const;
    std::string toString() const;
    std::vector<std::uint8_t> toDer() const;
    
private:
    std::vector<std::uint8_t> publicKey;
};

class PrivateKey {
public:
    PrivateKey();
    PrivateKey( const PrivateKey& );
    PrivateKey& operator=( const PrivateKey& );

    // TODO: Make this only visible to Central
    PrivateKey( const Mist::Central& central );
    
    const PublicKey& getPublicKey() const;

    //Signature sign( const SHA3& hash ) const;
    
private:
    // TODO: Why does this not work..? (does not make priv ctor visible.)
    //friend class Central;

    boost::optional<const Mist::Central&> mCentral;

};

} /* namespace CryptoHelper */
} /* namespace Mist */

#endif /* SRC_CRYPTOHELPER_H_ */
