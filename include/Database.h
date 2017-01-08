/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#ifndef SRC_DATABASE_H_
#define SRC_DATABASE_H_

// STL
#include <cstdint>
#include <map>
#include <memory>
#include <streambuf>
#include <string>
#include <vector>

#include <SQLiteCpp/SQLiteCpp.h>

#include "CryptoHelper.h"
#include "Helper.h"
#include "PrivateUserAccount.h"
#include "Query.h"

namespace Mist {

// Forward declaration
class Central;
class RemoteTransaction;
class Transaction;
class Deserializer;
class Serializer;

/**
 * Mist database class. It handles a versioned JSON store, that uses SQLite as
 * a storage engine.
 *
 * Each database has a simple access control system, with a list or users. Each
 * user has read, write or admin permission. With admin permission a user can
 * change user permissions.
 *
 * Each change to the database is stored as a transaction, and signed by the
 * user performing the change. Such transactions are then replicated to other users
 * of the database.
 *
 * Since the database is distributed this are not ACID transactions. Indeed the
 * database may have different state for different users. However, eventually, it
 * will reach the same state, when all transactions have been replicated.
 */
class Database {
public:
    using Connection = Helper::Database::Database;
    using Statement = Helper::Database::Statement;

    class Manifest {
        std::string name;
        Helper::Date created;
        CryptoHelper::PublicKey creator;
        CryptoHelper::SHA3 hash;
        CryptoHelper::Signature signature;

        using Signer = std::function<CryptoHelper::Signature(const CryptoHelper::SHA3&)>;
        using Verifier = std::function<bool(const CryptoHelper::PublicKey& key,
                const CryptoHelper::SHA3& hash,
                const CryptoHelper::Signature& sig)>;
        Signer signer;
        Verifier verifier;

        std::string inner() const;

    public:
        Manifest( Signer signer, Verifier verifier, std::string name,
                Helper::Date created, CryptoHelper::PublicKey creator,
                CryptoHelper::Signature signature = {},
                CryptoHelper::SHA3 hash = {} );
        virtual ~Manifest() = default;
        std::string getName() const { return name; }
        Helper::Date getCreated() const { return created; }
        CryptoHelper::PublicKey getCreator() const { return creator; }
        CryptoHelper::SHA3 getHash() const { return hash; }
        CryptoHelper::Signature getSignature() const { return signature; }
        void sign();
        bool verify() const;
        std::string toString() const;
        static Manifest fromString( const std::string& serialized, Verifier verifier, Signer signer = nullptr );
    };

    enum class AccessDomain
        : std::int8_t {
            Settings = 1,
        Normal = 2,
        User = 3,
        Device = 4
    };
    enum class ObjectStatus
        : std::int8_t {
            Current = 1,
        Deleted = 2,
        DeletedParent = 3, // If a parent in another access domain is deleted,
                                                         // or a parent is deleted in another transaction during a transaction conflict
        InvalidNew = 4,     // Object has not been created with a new action
        InvalidParent = 5,  // Object has never had a valid parent

        Old = 11,
        OldDeleted = 12,
        OldDeletedParent = 13,
        OldInvalidNew = 14,
        OldInvalidParent = 15,

        InvalidMove = 20, // Invalid move, use last revision instead
    };
    enum class ObjectAction
        : std::int8_t {
            New = 1, Update = 2, Move = 3, MoveUpdate = 4, Delete = 5,
    };

    struct Transaction {
        AccessDomain accessDomain;
        unsigned version;
        Helper::Date date;
        CryptoHelper::SHA3 creatorHash;
        CryptoHelper::SHA3 hash;
        CryptoHelper::Signature signature;
    };
    struct Meta {
        AccessDomain accessDomain;
        std::vector<CryptoHelper::SHA3> parents;
        Helper::Date timestamp;
        CryptoHelper::SHA3 user;
        CryptoHelper::SHA3 transactionHash;
        CryptoHelper::Signature signature;
    };
    struct ObjectRef {
        AccessDomain accessDomain;
        unsigned long id;
    };
    struct lessObjectRef {
        bool operator()( const ObjectRef& l, const ObjectRef& r ) const {
            if ( l.accessDomain == r.accessDomain ) {
                return l.id < r.id;
            } else {
                return l.accessDomain < r.accessDomain;
            }
        }
        typedef ObjectRef first_argument_type;
        typedef ObjectRef second_argument_type;
        typedef bool result_type;
    };

    class Value {
    public:
        enum class Type { Typeless, Null, Boolean, Number, String, Json } t;
        bool b;
        double n;
        std::string v;
        Value() : t( Type::Typeless ),b(),n(),v() {}
        Value( std::nullptr_t ) : t( Type::Null ),b(),n(),v() {}
        Value( bool b ) : t( Type::Boolean ),b(b),n(),v() {}
        Value( double n ) : t( Type::Number), b(),n(n),v() {}
        Value( int i ) : t( Type::Number ), b(),n(i),v() {}
        Value( const char* c, bool json = false ) : Value( std::string( c), json ) {}
        Value( const std::string& v, bool json = false ) : t( json? Type::Json: Type::String ),b(),n(),v(v) {}
        bool operator<(const Value& rhs );
        std::string string() const { return v; }
        double number() const { return n; }
        bool boolean() const { return b; }
        Type type() const { return t; }
    };

    // TODO: split Object and attributes to allow for better reading.
    struct Object {
        AccessDomain accessDomain;
        unsigned long id;
        unsigned version;
        ObjectRef parent;
        std::map<std::string, Value> attributes;
        ObjectStatus status;
        ObjectAction action;
    };
    struct ObjectMeta {
        AccessDomain accessDomain;
        unsigned long id;
        unsigned version;
        ObjectRef parent;
        ObjectStatus status;
        ObjectAction action;
    };

    constexpr static unsigned RESERVED = 1023;
    constexpr static unsigned long ALLOCATE_64_BIT = 1024 * 1024 * 1024;
    constexpr static unsigned ROOT_OBJECT_ID = 0;
    constexpr static unsigned USERS_OBJECT_ID = 1;

    Database( Central *central, std::string path );
    virtual ~Database();

    // TODO: Consider moving create and init to ctor as to better conform to RAII
    void create( unsigned localId, std::unique_ptr<Manifest> manifest ); // create new db
    void init( std::unique_ptr<Manifest> manifest = nullptr ); // initialize existing db
    //void load(); // load from disk
    void close();
    void dump( const std::string& filename );

    /**
     * Start a transaction that will perform changes to the database. The transaction will make changes
     * to the normal access domain.
     */
    std::unique_ptr<Mist::Transaction> beginTransaction();

    bool isOK(); // TODO: fix proper error handling instead!

    void inviteUser( const UserAccount& user ); // TODO make some call to central, or the other way around?

    /*
     * Query interface
     */

    struct QueryResult {
        bool isFunctionCall{};
        std::string functionName{};
        std::string functionAttribute{};
        double functionValue{};
        unsigned long id{};
        unsigned long version{};
        std::map<std::string,Value> attributes{};
    };

    void unsubscribe( unsigned subscriberId );

    Object getObject( int accessDomain, long long id, bool includeDeleted = false ) const;
    Object getObject( Connection* connection, int accessDomain, long long id, bool includeDeleted = false ) const;
    unsigned subscribeObject( std::function<void(Object)> cb, int accessDomain,
            long long id, bool includeDeleted = false );

    QueryResult query( int accessDomain, long long id, const std::string& select,
            const std::string& filter, const std::string& sort,
            const std::map<std::string, ArgumentVT>& args,
            int maxVersion, bool includeDeleted = false );
    QueryResult query( Connection* connection, int accessDomain, long long id, const std::string& select,
                const std::string& filter, const std::string& sort,
                const std::map<std::string, ArgumentVT>& args,
                int maxVersion, bool includeDeleted = false );
    QueryResult query( const Query& querier, Connection* connection );
    unsigned subscribeQuery( std::function<void(QueryResult)> cb,
            int accessDomain, long long id, const std::string& select,
            const std::string& filter, const std::string& sort,
            const std::map<std::string, ArgumentVT>& args,
            int maxVersion, bool includeDeleted = false );

    QueryResult queryVersion( int accessDomain, long long id, const std::string& select,
            const std::string& filter, const std::map<std::string, ArgumentVT>& args,
            bool includeDeleted = false );
    QueryResult queryVersion( Connection* connection, int accessDomain, long long id, const std::string& select,
                const std::string& filter, const std::map<std::string, ArgumentVT>& args,
                bool includeDeleted = false );
    QueryResult queryVersion( const Query& quierier, Connection* connection );
    unsigned subscribeQueryVersion( std::function<void(QueryResult)> cb,
            int accessDomain, long long id, const std::string& select,
            const std::string& filter, const std::map<std::string, ArgumentVT>& args,
            bool includeDeleted = false );

    /*
     * Streaming API
     * Using the exchange format
     */
    void writeToDatabase( std::basic_streambuf<char>& sb );
    void writeToDatabase( const char* data, std::size_t length );
    void writeToDatabase( const std::string& data );

    // TODO: serializer have to be reset after throw, change that!
    void readTransaction( std::basic_streambuf<char>& sb,
            const std::string& hash,
            Connection* connection = nullptr ) const;
    // Reading the transaction body is used for calculation the hash for the transaction
    void readTransactionBody( std::basic_streambuf<char>& sb,
            const std::string& hash,
            Connection* connection = nullptr ) const;
    void readTransactionBody( std::basic_streambuf<char>& sb,
            unsigned version,
            Connection* connection = nullptr ) const;
    void readTransactionList( std::basic_streambuf<char>& sb ) const;
    void readTransactionMetadata( std::basic_streambuf<char>& sb,
            const std::string& hash ) const;
    void readTransactionMetadataLastest( std::basic_streambuf<char>& sb ) const;
    void readTransactionMetadataFrom( std::basic_streambuf<char>& sb,
            const std::vector<std::string>& hashes ) const;

    void readUser( std::basic_streambuf<char>& sb,
            const std::string& userHash ) const;
    void readUsers( std::basic_streambuf<char>& sb ) const;
    void readUsersFrom( std::basic_streambuf<char>& sb,
            const std::vector<std::string>& userHashes ) const;

    /*
     * Help methods for the streaming API
     * TODO: Redo all of these to return Database::Meta instead!
     */
    Database::Transaction getTransactionMeta( unsigned version,
            Connection* connection = nullptr ) const;
    Database::Transaction getTransactionMeta( const std::string& hash,
            Connection* connection = nullptr ) const;
    Database::Transaction getTransactionMeta( const CryptoHelper::SHA3& hash,
            Connection* connection = nullptr ) const;

    std::vector<Database::Transaction> getTransactionLatest() const;
    std::vector<Database::Transaction> getTransactionList() const;
    std::vector<Database::Transaction> getTransactionsFrom(
            const std::vector<std::string>& hashIds,
            Connection* connection = nullptr ) const;

    Database::Meta transactionToMeta( const Database::Transaction& tranaction,
            Connection* connection = nullptr ) const;
    static std::vector<Database::Meta> streamToMeta( std::basic_streambuf<char>& sb );

    Manifest *getManifest();

protected:
    // Friend Transaction and RemoteTransaction so they may call
    friend class Mist::Central;
    friend class Mist::RemoteTransaction;
    friend class Mist::Transaction; // Consider redesigning this.
    friend class Mist::Deserializer;
    friend class Mist::Serializer;

    using map_trans_f = std::function<void(const Database::Transaction&)>;
    using map_meta_f = std::function<void(const Database::Meta&)>;
    using map_obj_f = std::function<void(const Database::Object&)>;
    using map_user_f = std::function<void(
            const std::string&,
            const std::string&,
            const std::string&,
            const std::string&)>;

    void mapTransactions( map_trans_f fn,
            Connection* connection = nullptr ) const;
    void mapTransactionLatest( map_trans_f fn,
            Connection* connection = nullptr ) const;
    void mapTransactionsFrom( map_trans_f fn, const std::vector<std::string>& ids,
            Connection* connection = nullptr ) const;
    void mapObject( map_obj_f fn, const Database::Transaction& transaction,
            Connection* connection = nullptr ) const;
    void mapObjectId( map_obj_f fn, const long long id,
            Connection* connection = nullptr ) const;
    void mapParents( map_trans_f fn, const Database::Transaction& transaction,
            Connection* connection = nullptr ) const;

    bool haveAll( const std::vector<std::string>& transactionHashes,
            Connection* connection = nullptr ) const;

    /*
     * Help methods for users
     */
    void mapUser( map_user_f fn, const std::string& userHash ) const;
    void mapUsers( map_user_f fn ) const;
    void mapUsersFrom( map_user_f fn, const std::vector<std::string>& userIds ) const;

    std::shared_ptr<UserAccount> getUser( const std::string& userHash ) const;

    /*
     * Transactions
     */
    std::unique_ptr<Mist::RemoteTransaction> beginRemoteTransaction(
            Database::AccessDomain accessDomain,
            std::vector<Database::Transaction> parents,
            Helper::Date timestamp,
            CryptoHelper::SHA3 userHash,
            CryptoHelper::SHA3 hash,
            CryptoHelper::Signature signature );

    /**
     * Start a transaction that will perform changes to a specified access domain of the database.
     */
    std::unique_ptr<Mist::Transaction> beginTransaction( AccessDomain accessDomain );

    std::unique_ptr<Connection> getIsolatedDbConnection() const;
    CryptoHelper::SHA3 calculateTransactionHash( const Database::Transaction& transaction,
            Connection* connection = nullptr ) const;
    unsigned reorderTransaction( const Database::Transaction& tranasaction,
            Connection* connection = nullptr );
    CryptoHelper::Signature signTransaction( const CryptoHelper::SHA3& hash ) const;

    void objectChanged( const ObjectRef& objectRef );
    void objectsChanged( const std::set<Database::ObjectRef, Database::lessObjectRef>& objects );
    void rollback( Mist::RemoteTransaction *transaction );
    void rollback( Mist::Transaction *transaction );
    void commit( Mist::RemoteTransaction *transaction );
    void commit( Mist::Transaction *transaction );

    static Database::Transaction statementRowToTransaction( Database::Statement& stmt );
    static Database::Object statementRowToObject( Database::Statement& stmt, std::map<std::string, Database::Value> attributes );
    static Database::ObjectMeta statementRowToObjectMeta( Database::Statement& stmt );
    static Database::Value statementRowToValue( Database::Statement& attribute );
    //static UserAccount statementRowToUser( Database::Statement& user );
    static Database::Value queryRowToValue( Database::Statement& query );

    std::vector<CryptoHelper::SHA3> getParents( unsigned version,
            Connection* connection = nullptr ) const;
    std::vector<CryptoHelper::SHA3> getParents( CryptoHelper::SHA3 transactionHash,
            Connection* connection = nullptr ) const;

    const std::string& getUserHash() { return userHash; }
private:

    static bool dbExists( std::string filename );
    //bool enableTransactionFiles{ true };

    bool _isOK; // TODO: fix proper error handling instead.
    std::unique_ptr<Manifest> manifest;
    Central *central;
    std::string path;
    std::string userHash;
    std::unique_ptr<Connection> db;
    std::unique_ptr<Deserializer> deserializer;
    std::unique_ptr<Serializer> serializer;

    static unsigned subId;
    std::map<long long,std::set<unsigned>> objectSubscribers{};
    std::map<unsigned,std::set<long long>> subscriberObjects{};
    std::map<unsigned,std::function<void(Object)>> objectSubscriberCallback{};
    std::map<unsigned,std::pair<
        std::unique_ptr<Query>,
        std::function<void(QueryResult)>>> querySubscriberCallback{};
};

// Database utils


} /* namespace Mist */

#endif /* SRC_DATABASE_H_ */
