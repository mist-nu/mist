/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef SRC_TRANSACTION_H_
#define SRC_TRANSACTION_H_

#include <set>
#include <string>

#include "Database.h"

namespace Mist {

/**
 * Class for performing changes to a Mist database. Create by calling beginTransaction in a Database object.
 */
class Transaction {
public:
    Transaction( Database *db, Database::AccessDomain accessDomain, unsigned version );
    virtual ~Transaction();
private:
    void addAccessDomainDependency( Database::AccessDomain accessDomain, unsigned version );
    unsigned long allocateObjectId();
public:
    /**
     * Create a new object. The parent object must be in the same, or a lower, access domain.
     */
    virtual unsigned long newObject( const Database::ObjectRef &parent,
            const std::map<std::string,
            Database::Value> &attributes );
    /**
     * Move an existing object. The object must belong to this access domain. However it can be moved
     * so the parent object is in another, lower, access domain.
     */
    virtual void moveObject( unsigned long id, const Database::ObjectRef &newParent ); // TODO: Is it enough with just id? the ts version uses an objref
    /**
     * Change the attributes of an existing object.
     */
    virtual void updateObject( unsigned long id, const std::map<std::string, Database::Value> &attributes );
    /**
     * Delete an existing object. The object must not have any children. If it has, the children must be
     * deleted before the object can be deleted.
     */
    virtual void deleteObject( unsigned long id );
    /**
     * Commit the transaction. It is stored and replicated to peers. May trigger subscribed queries to be
     * reevaluated.
     */
    virtual void commit();
    /**
     * Roll back the transaction. Nothing is stored.
     */
    virtual void rollback();

    /**
     * Get object from the current transaction. With this method you can access objects before they have
     * been committed to the database.
     */
    virtual Database::Object getObject( int accessDomain, long long id, bool includeDeleted = false ) const;

    /**
     * Query the current transaction.
     */
    virtual Database::QueryResult query( int accessDomain, long long id, const std::string& select,
                const std::string& filter, const std::string& sort,
                const std::map<std::string, ArgumentVT>& args,
                int maxVersion, bool includeDeleted = false );

    /**
     * Query version against the current transaction.
     */
    virtual Database::QueryResult queryVersion( int accessDomain, long long id, const std::string& select,
                const std::string& filter, const std::map<std::string, ArgumentVT>& args,
                bool includeDeleted = false );

private:
    Database *db;
    std::unique_ptr<Database::Connection> connection;
    std::unique_ptr<Helper::Database::Transaction> transaction;
    Database::AccessDomain accessDomain;
    unsigned version;

    // List of objects affected by this transaction.
    std::set<Database::ObjectRef, Database::lessObjectRef> affectedObjects;

    /*
     TODO: When is 'parentAccessDomains' used?
     The given skeleton implements parentAccessDomains as a set,
     and thus can not be saved as a key-value pair in 'addAccessDomainDependency'.
     Should this be dropped?
     //*/
    std::set<Database::AccessDomain> parentAccessDomains;
    bool valid;
};

} /* namespace Mist */

#endif /* SRC_TRANSACTION_H_ */
