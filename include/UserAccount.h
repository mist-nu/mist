/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef SRC_USERACCOUNT_H_
#define SRC_USERACCOUNT_H_

#include <string>

#include "CryptoHelper.h"
#include "Exception.h"

namespace Mist {

class UserAccountException : public Exception {
public:
    explicit UserAccountException( std::string err = "Unknown serialized format." ) : Exception( err ) {}
    virtual ~UserAccountException() = default;
};

class Permission {
public:
    enum class P {
        read,
        write,
        admin
    };

    Permission( const P& p ) : permission( p ) {}
    Permission( const std::string& p );
    virtual ~Permission() = default;

    bool operator==( Permission::P p ) const { return p == permission; }

    std::string toString() const;

    static Permission deserialize( const std::string& serialized );

protected:
    P permission;
};

class UserAccount {
public:
    UserAccount( const std::string& name,
            const CryptoHelper::PublicKey& publicKey,
            const CryptoHelper::PublicKeyHash& id,
            const Permission& permission );
    UserAccount( std::string&& name,
            CryptoHelper::PublicKey&& publicKey,
            CryptoHelper::PublicKeyHash&& hash,
            Permission&& permission );
    virtual ~UserAccount();

    const CryptoHelper::PublicKey& getPublicKey() const;
    const CryptoHelper::PublicKeyHash& getPublicKeyHash() const;
    const std::string& getName() const;
    const Permission& getPermission() const;

    std::string toString() const;
    static UserAccount deserialize( const std::string& serialized );

protected:
    CryptoHelper::PublicKeyHash id;
    CryptoHelper::PublicKey publicKey;
    std::string name;
    Permission permission;
};

} /* namespace Mist */

#endif /* SRC_USERACCOUNT_H_ */
