/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef SRC_PRIVATEUSERACCOUNT_H_
#define SRC_PRIVATEUSERACCOUNT_H_

#include <string>

#include "CryptoHelper.h"
#include "Exception.h"
#include "UserAccount.h"

namespace Mist {

class PrivateUserAccountException : public Exception {
public:
    explicit PrivateUserAccountException( std::string err ) : Exception( err ) {}
    virtual ~PrivateUserAccountException() = default;
};

class PrivateUserAccount : public UserAccount {
public:
    PrivateUserAccount( const std::string& name,
            const CryptoHelper::PrivateKey& privateKey );
    PrivateUserAccount() : PrivateUserAccount( "dummyAccount", {} ) {} // TODO: remove this once everythign else is correctly implemented
    virtual ~PrivateUserAccount();

    std::string toString() const;
    static PrivateUserAccount deserialize( const std::string& serialized );

private:
    CryptoHelper::PrivateKey privateKey;
};

} /* namespace Mist */

#endif /* SRC_PRIVATEUSERACCOUNT_H_ */
