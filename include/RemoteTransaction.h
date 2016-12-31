/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef SRC_REMOTETRANSACTION_H_
#define SRC_REMOTETRANSACTION_H_

// STL
#include <cstdint>
#include <set>
#include <string>

#include "Database.h"

namespace Mist {

class RemoteTransaction {
public:
    RemoteTransaction(
            Database *db,
            Database::AccessDomain accessDomain,
            unsigned version,
            std::vector<Database::Transaction> parents,
            Helper::Date timestamp,
            CryptoHelper::SHA3 userHash,
            CryptoHelper::SHA3 hash,
            CryptoHelper::Signature signature
    );
    virtual ~RemoteTransaction();
    virtual void init();
    virtual void newObject(
            unsigned long id,
            const Database::ObjectRef& parent,
            const std::map<std::string, Database::Value>& attributes
    );
    virtual void moveObject( unsigned long id, Database::ObjectRef newParent );
    virtual void updateObject( unsigned long id, std::map<std::string, Database::Value> attributes );
    virtual void deleteObject( unsigned long id );
    virtual void commit();
    virtual void rollback();

protected:
    virtual void checkNew( Database::ObjectMeta& object );
    virtual void checkNew( Database::ObjectMeta&& object );
    virtual Database::ObjectStatus getParentStatus( const Database::ObjectMeta& object ) const;
    virtual bool isInvalidStatus( const Database::ObjectStatus& status ) const;
    virtual bool isDeletedStatus( const Database::ObjectStatus& status ) const;
    virtual unsigned long nextNumber( unsigned long num ) const;
    virtual unsigned long findNewId( unsigned long id ) const;

private:
    Database *db;
    std::unique_ptr<Database::Connection> connection;
    std::unique_ptr<Helper::Database::Transaction> transaction;
    Database::AccessDomain accessDomain;
    unsigned version;
    std::vector<Database::Transaction> parents;
    Helper::Date timestamp;
    CryptoHelper::SHA3 userHash;
    CryptoHelper::SHA3 hash;
    CryptoHelper::Signature signature;
    bool last;
    bool valid;
    std::map<Database::ObjectRef, unsigned long, Database::lessObjectRef> renumber;
    std::set<Database::ObjectRef, Database::lessObjectRef> affectedObjects;
};

} /* namespace Mist */

#endif /* SRC_REMOTETRANSACTION_H_ */
