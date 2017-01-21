/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <fstream>
#include <string>
//#include <unistd.h>
#include <iostream>

#include "Central.h"
#include "Database.h"
#include "ExchangeFormat.h"
#include "Helper.h"
#include "Transaction.h"
#include "RemoteTransaction.h"

namespace Mist {

// Initialize static member
unsigned Database::Database::subId = 0u;

namespace {

ArgumentVT convertValueToArg( const Database::Value& val ) {
    using T = Database::Value::Type;
    switch( val.type() ) {
    case T::Typeless:
        return ArgumentVT();
    case T::Null:
        return ArgumentVT( nullptr );
    case T::Boolean:
        return ArgumentVT( val.boolean() );
    case T::Number:
        return ArgumentVT( val.number() );
    case T::String:
        return ArgumentVT( val.string() );
    case T::Json:
        return ArgumentVT( val.string(), true );
    default:
        throw std::logic_error( "Unhandled conversion of Database::Value to Argument" );
    }
}

std::map<std::string,ArgumentVT> valueMapToArgumentMap( const std::map<std::string, Database::Value>& args ) {
    std::map<std::string,ArgumentVT> res{};
    for ( const auto& p : args ) {
        res.emplace( p.first, convertValueToArg( p.second ) );
    }
    return res;
}

} /* anonymous namespace */

Database::Database( Central *central, std::string path ) :
        _isOK( true ),
        manifest( nullptr ),
        central( central ),
        path( path ),
        userHash(),
        db( nullptr ),
        deserializer( new Deserializer( this ) ),
        serializer( new Serializer( this ) ) {

}

Database::~Database() {
}

bool Database::isOK() {
    return _isOK;
}

void Database::create( unsigned localId, std::unique_ptr<Manifest> manifest ) {
    LOG ( DBUG ) << "Creating database";
    // TODO: Do something with manifest?
    if ( db ) {
        _isOK = false;
        throw std::runtime_error( "Database already set." );
    }

    if ( manifest ) {
        if( manifest->verify() ) {
            userHash = manifest->getCreator().hash().toString();
            this->manifest = std::move( manifest );
        } else {
            throw std::runtime_error( "Manifest verification failed" );
        }
    }


    //Connection( path, Helper::Database::OPEN_READONLY );
    bool alreadyExists = true;
    try {
        Connection( path, Helper::Database::OPEN_READONLY );
    } catch ( Helper::Database::Exception &e ) {
        // TODO: verify that it's the correct exception
        alreadyExists = false;
    }

    if ( alreadyExists ) {
        // TODO: what todo?
        _isOK = false;
        throw std::runtime_error( "Database already exists." );
    }

    //try {
        db.reset( new Connection( path, Helper::Database::OPEN_CREATE | Helper::Database::OPEN_READWRITE ) );

        // Make sure the db is in WAL-mode so that multiple readers can read at the same time as one writer is writing.
        int rc = db->exec( "PRAGMA journal_mode=WAL;" );
        if ( rc == Helper::Database::OK ) {
            // TODO: check if db is WAL here, can avoid unnecessary dtor ctor calls.
            db.reset( new Connection( path, Helper::Database::OPEN_READWRITE ) );
        } else {
            // TODO: figure out why it failed.
            _isOK = false;
        }

        Helper::Database::Transaction transaction( *db.get() );
        //db->exec( "CREATE TABLE AccessDomain (hash TEXT, localId INTEGER, parent INTEGER, creator INTEGER, name TEXT)" );
        db->exec( "CREATE TABLE Object (accessDomain INTEGER, id INTEGER, version INTEGER, status INTEGER, parent INTEGER, parentAccessDomain INTEGER, transactionAction INTEGER)" );
        db->exec( "CREATE TABLE Attribute (accessDomain INTEGER, id INTEGER, version INTEGER, name TEXT, type INTEGER, value)" );
        db->exec( "CREATE TABLE 'Transaction' (accessDomain INTEGER, version INTEGER, timestamp DATETIME, userHash TEXT, hash TEXT, signature TEXT)" );
        db->exec( "CREATE TABLE TransactionParent (version INTEGER, parentAccessDomain INTEGER, parentVersion INTEGER)" );
        db->exec( "CREATE TABLE Renumber (accessDomain INTEGER, version INTEGER, oldId INTEGER, newId INTEGER)" );
        transaction.commit();
    /*
    } catch ( Helper::Database::Exception &e ) {
        // TODO: handle errors
        _isOK = false;
    }
     //*/
}

void Database::init( std::unique_ptr<Manifest> manifest ) {
    LOG ( DBUG ) << "Initializing database";
    try {
        if ( manifest ) {
            if( manifest->verify() ) {
                userHash = manifest->getCreator().hash().toString();
                this->manifest = std::move( manifest );
            } else {
                throw std::runtime_error( "Manifest verification failed" );
            }
        }
        if ( !db ) { // TODO: what to do if this.create() have been called?
            db.reset( new Connection( path, Helper::Database::OPEN_READWRITE ) );
        }
    } catch ( Helper::Database::Exception &e ) {
        // TODO: handle errors.
        _isOK = false;
        throw e;
    }
}

/*
void Database::load() {
    LOG( DBUG ) << "Loading transactions from disk.";
    if ( db == nullptr ) {
        LOG( WARNING ) << "Db not initialized";
        return;
    }
    enableTransactionFiles = false;
}
//*/

void Database::close() {
    LOG ( DBUG ) << "Closing database";
    db.reset();
}

void Database::dump( const std::string& filename ) {
    LOG( INFO ) << "Dumping database to exchange format.";
    if( manifest ) {
        std::fstream fs( filename + ".manifest.json", std::fstream::out | std::fstream::trunc | std::fstream::binary );
        fs << manifest->toString();
        fs.close();
    }
    std::fstream fs ( filename + ".json", std::fstream::out | std::fstream::trunc | std::fstream::binary );
    mapTransactions( [this, &fs](const Database::Transaction& trans) -> void {
        this->readTransaction( *fs.rdbuf(), trans.hash.toString() );
        fs.put( '\n' );
    } );
    fs.close();
}

std::unique_ptr<Mist::Transaction> Database::beginTransaction() {
    return beginTransaction( AccessDomain::Normal );
}

void Database::writeToDatabase( std::basic_streambuf<char>& sb ) {
    deserializer->write( sb );
}

void Database::writeToDatabase( const char* data, std::size_t length ) {
    deserializer->write( data, length );
}

void Database::writeToDatabase( const std::string& data ) {
    deserializer->write( data );
}

void Database::readTransaction( std::basic_streambuf<char>& sb,
        const std::string& hash,
        Connection* connection ) const {
    serializer->readTransaction( sb, hash, connection );
}

void Database::readTransactionBody( std::basic_streambuf<char>& sb,
        const std::string& hash,
        Connection* connection ) const {
    serializer->readTransactionBody( sb, hash, connection );
}

void Database::readTransactionBody( std::basic_streambuf<char>& sb,
        unsigned version,
        Connection* connection ) const {
    serializer->readTransactionBody( sb, version, connection );
}

void Database::readTransactionList( std::basic_streambuf<char>& sb ) const {
    serializer->readTransactionList( sb );
}

void Database::readTransactionMetadata( std::basic_streambuf<char>& sb, const std::string& hash ) const {
    serializer->readTransactionMetadata( sb, hash );
}

void Database::readTransactionMetadataLastest( std::basic_streambuf<char>& sb ) const {
    serializer->readTransactionMetadataLatest( sb );
}

void Database::readTransactionMetadataFrom( std::basic_streambuf<char>& sb, const std::vector<std::string>& hashes ) const {
    serializer->readTransactionMetadataFrom( sb, hashes );
}

void Database::readUser( std::basic_streambuf<char>& sb, const std::string& hash ) const {
    serializer->readUser( sb, hash );
}

void Database::readUsers( std::basic_streambuf<char>& sb ) const {
    serializer->readUsers( sb );
}

void Database::readUsersFrom( std::basic_streambuf<char>& sb, const std::vector<std::string>& userHashes ) const {
    serializer->readUsersFrom( sb, userHashes );
}

Database::Transaction Database::getTransactionMeta( unsigned version,
        Connection* connection ) const {
    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }
    Database::Statement transactionMeta( *conn,
            "SELECT accessDomain, version, timestamp, userHash, hash, signature "
            " FROM 'Transaction' "
            " WHERE version=? " );
    transactionMeta << version;
    if ( !transactionMeta.executeStep() ) {
        LOG( WARNING ) << "Transaction does not exist.";
        throw std::runtime_error( "Transaction does not exist." );
    }
    return statementRowToTransaction( transactionMeta );
}

Database::Transaction Database::getTransactionMeta( const std::string& hash,
        Connection* connection ) const {
    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }
    Database::Statement query( *conn,
            "SELECT accessDomain, version, timestamp, userHash, hash, signature "
            " FROM 'Transaction' "
            " WHERE hash=? " );
    query << hash;
    if ( !query.executeStep() ) {
        LOG( WARNING ) << "Transaction does not exist.";
        throw std::runtime_error( "Transaction does not exist." );
    }
    return statementRowToTransaction( query );
}

Database::Transaction Database::getTransactionMeta( const CryptoHelper::SHA3& hash,
        Connection* connection ) const {
    return getTransactionMeta( hash.toString(), connection );
}

// TODO: Redo, should return vector<Meta> and should include all child-less transactions
std::vector<Database::Transaction> Database::getTransactionLatest() const {
    std::vector<Database::Transaction> latest{};

    auto cb = [&latest] ( const Database::Transaction& trans ) -> void {
        latest.push_back( trans );
    };

    mapTransactionLatest( cb );

    return latest;
}

std::vector<Database::Transaction> Database::getTransactionList() const {
    Database::Statement qAll( *db.get(),
            "SELECT accessDomain, version, timestamp, userHash, hash, signature "
            "FROM 'Transaction' "
            "ORDER BY version ASC ");
    std::vector<Database::Transaction> all{};
    while( qAll.executeStep() ) {
        all.push_back( statementRowToTransaction( qAll ) );
    }
    return all;
}

// TODO: redo, should return vector<Meta> with all transactions from the oldest transaction of the hashes
std::vector<Database::Transaction> Database::getTransactionsFrom( const std::vector<std::string>& hashIds,
        Connection* connection ) const {
    if ( 0 == hashIds.size() ) {
        return std::vector<Database::Transaction>{};
    }

    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }

    if ( !haveAll( hashIds, conn ) ) {
        LOG( DBUG ) << "Not found";
        throw Exception( Error::ErrorCode::NotFound );
    }

    std::string getOldest {
        "SELECT version, timestamp "
        " FROM 'Transaction' "
        " WHERE hash IN ( "
    };

    for ( const std::string id: hashIds ) {
        getOldest += "'" + id +"',";
    }
    getOldest.pop_back(); // Remove last comma
    getOldest += ") "
            "ORDER BY datetime( timestamp ) DESC ";

    Database::Statement oldest( *conn, getOldest );

    if ( !oldest.executeStep() ) {
        LOG( WARNING ) << "Have transaction but could not fetch them.";
        throw Mist::Exception( "Have transaction but could not fetch them.", Error::ErrorCode::UnexpectedDatabaseError );
    }

    Database::Statement transactionsFromVersion( *conn,
            "SELECT accessDomain, version, timestamp, userHash, hash, signature "
            "FROM 'Transaction' "
            "WHERE version >= ? "
            "ORDER BY version ASC ");
    transactionsFromVersion << oldest.getColumn( "version" ).getUInt();

    std::vector<Database::Transaction> transactions{};
    while ( transactionsFromVersion.executeStep() ) {
        transactions.push_back( statementRowToTransaction( transactionsFromVersion ) );
    }

    return transactions;
}

Database::Meta Database::transactionToMeta( const Database::Transaction& trans,
        Connection* connection ) const {
    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }
    std::vector<CryptoHelper::SHA3> parents{};
    auto cb = [&parents] ( const Database::Transaction& parent ) -> void {
        parents.push_back( parent.hash );
    };

    mapParents( cb, trans, conn );

    return Database::Meta{ trans.accessDomain,
        parents,
        trans.date,
        trans.creatorHash,
        trans.hash,
        trans.signature
    };
}

std::vector<Database::Meta> Database::streamToMeta( std::basic_streambuf<char>& sb ) {
    return Deserializer::exchangeFormatToMeta( sb );
}

void Database::mapTransactions( map_trans_f fn,
        Connection* connection ) const {
    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }
    Database::Statement all( *conn,
            "SELECT accessDomain, version, timestamp, userHash, hash, signature "
            "FROM 'Transaction' "
            "ORDER BY version ASC ");
    while( all.executeStep() ) {
        fn( statementRowToTransaction( all ) );
    }
}

void Database::mapTransactionLatest( map_trans_f fn,
        Connection* connection ) const {
    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }
    Database::Statement openTransactions( *conn,
            "SELECT accessDomain, t.version AS version, timestamp, userHash, hash, signature "
            "FROM 'Transaction' AS t "
            "LEFT OUTER JOIN TransactionParent tp "
                "ON t.version=tp.parentVersion " // AND t.accessDomain=tp.parentAccessDomain
            "WHERE tp.version IS NULL "
            "ORDER BY t.version ASC " );

    while( openTransactions.executeStep() ) {
        fn( statementRowToTransaction( openTransactions ) );
    }
}

void Database::mapParents( map_trans_f fn, const Database::Transaction& transaction,
        Connection* connection ) const {
    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }
    Database::Statement parent( *conn,
            "SELECT accessDomain, t.version AS version, timestamp, userHash, hash, signature "
            "FROM TransactionParent AS tp, 'Transaction' AS t "
            "WHERE tp.version=? AND t.accessDomain=tp.parentAccessDomain AND t.version=tp.parentVersion "
            "ORDER BY t.version ASC ");
    parent << transaction.version;
    while ( parent.executeStep() ) {
        fn( statementRowToTransaction( parent ) );
    }
}

void Database::mapTransactionsFrom( map_trans_f fn, const std::vector<std::string>& ids,
        Connection* connection ) const {
    if ( 0 == ids.size() ) {
        return;
    }

    if ( !haveAll( ids ) ) {
        LOG( DBUG ) << "Not found";
        throw Exception( Error::ErrorCode::NotFound );
    }

    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }

    std::string getOldest {
        "SELECT version, timestamp "
        "FROM 'Transaction' "
        "WHERE hash IN ( "
    };

    for ( const std::string id: ids ) {
        getOldest += "'" + id +"',";
    }
    getOldest.pop_back(); // Remove last comma
    getOldest += ") "
            "ORDER BY datetime( timestamp ) DESC ";

    Database::Statement oldest( *conn, getOldest );

    if ( !oldest.executeStep() ) {
        LOG( WARNING ) << "Have transaction but could not fetch them.";
        throw Mist::Exception( "Have transaction but could not fetch them.", Error::ErrorCode::UnexpectedDatabaseError );
    }

    Database::Statement transactionsFromVersion( *conn,
            "SELECT accessDomain, version, timestamp, userHash, hash, signature "
            "FROM 'Transaction' "
            "WHERE version >= ? "
            "ORDER BY version ASC ");
    transactionsFromVersion << oldest.getColumn( "version" ).getUInt();

    while ( transactionsFromVersion.executeStep() ) {
        fn( statementRowToTransaction( transactionsFromVersion ) );
    }
}

void Database::mapObject( map_obj_f fn, const Database::Transaction& transaction,
        Connection* connection ) const {
    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }
    Database::Statement object( *conn,
            "SELECT accessDomain, id, version, status, parent, parentAccessDomain, transactionAction "
            "FROM Object "
            "WHERE version=? "
            "ORDER BY id ASC " );
    object << transaction.version;
    Database::Statement attribute( *conn,
            //"SELECT accessDomain, id, version, name, value, json "
            "SELECT accessDomain, id, version, name, type, value "
            "FROM Attribute "
            "WHERE version=?"
            "ORDER BY id ASC " );
    while( object.executeStep() ) {
        std::map<std::string,Value> attributes{};
        attribute << object.getColumn( "version" ).getInt64();
        while ( attribute.executeStep() ) {
            attributes.emplace(
                    attribute.getColumn( "name" ).getString(),
                    statementRowToValue( attribute )
            );
        }
        fn( statementRowToObject( object, attributes ) );
        attribute.clearBindings();
        attribute.reset();
    }
}

void Database::mapObjectId( map_obj_f fn, const long long id,
        Connection* connection ) const {
    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }
    Database::Statement object( *conn,
            "SELECT accessDomain, id, version, status, parent, parentAccessDomain, transactionAction "
            "FROM Object "
            "WHERE id=? "
            "ORDER BY version ASC ");
    object << id;
    Database::Statement attribute( *conn,
            "SELECT accessDomain, id, version, name, type, value "
            "FROM Attribute "
            "WHERE version=?"
            "ORDER BY id ASC " );
    while( object.executeStep() ) {
        std::map<std::string,Value> attributes{};
        attribute << object.getColumn( "version" ).getInt64();
        while ( attribute.executeStep() ) {
            attributes.emplace(
                    attribute.getColumn( "name" ).getString(),
                    statementRowToValue( attribute )
            );
        }
        fn( statementRowToObject( object, attributes ) );
        attribute.clearBindings();
        attribute.reset();
    }
}

bool Database::haveAll( const std::vector<std::string>& transactionHashes ,
            Connection* connection ) const {
    if ( transactionHashes.empty() ) {
        LOG( DBUG ) << "Not found";
        throw Exception( "Can not find transactions for an empty array of hashes.", Mist::Error::ErrorCode::NotFound );
    }
    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }

    std::string haveAllQuery {
        "SELECT hash "
        "FROM 'Transaction' "
        "WHERE hash IN ( "
    };

    for ( const std::string id: transactionHashes ) {
        haveAllQuery += "'" + id +"',";
    }
    haveAllQuery.pop_back(); // Remove last comma
    haveAllQuery += ") ";

    unsigned count{};
    Database::Statement countAll( *conn, haveAllQuery );
    while( countAll.executeStep() ) {
        ++count;
    }

    return transactionHashes.size() == count;
}

std::shared_ptr<UserAccount> Database::getUser( const std::string& userHash ) const {
    Database::Statement userId( *db.get(),
            "SELECT id "
            "FROM Attribute "
            "WHERE name='id' AND value=? AND version IN ( "
                "SELECT DISTINCT version "
                "FROM Object "
                "WHERE parent=? "
            ") "
            "ORDER BY version DESC " );
    userId << userHash << USERS_OBJECT_ID;
    if( !userId.executeStep() ) {
        // Not found
        return nullptr;
    }

    // TODO: check object status to make sure that the user still is valid
    Database::Statement user( *db.get(),
            "SELECT accessDomain, id, version, name, value "
            "FROM Attribute "
            "WHERE id=? "
            "ORDER BY version DESC ");
    user << userId.getColumn( "id" ).getString();

    std::string id{}, name{}, permission{}, publicKeyPem{};
    for( int i{0}; i < 4; ++i ) {
        if ( !user.executeStep() ) {
            LOG( WARNING ) << "Unexpected Error, could not read user data";
            throw Exception( Error::ErrorCode::UnexpectedDatabaseError );
        }
        std::string what{ user.getColumn( "name" ).getString() };
        if ( "id" == what ) {
            id = user.getColumn( "value" ).getString();
        } else if ( "name" == what ) {
            name = user.getColumn( "value" ).getString();
        } else if ( "permission" == what ) {
            permission = user.getColumn( "value" ).getString();
        } else if ( "publicKey" == what ) {
            publicKeyPem = user.getColumn( "value" ).getString();
        }
    }
    if ( id.empty() || name.empty() || permission.empty() || publicKeyPem.empty() ) {
        // TODO: throw or return nullptr?
        LOG( WARNING ) << "Can not get user from database, invalid user data in database";
        throw std::runtime_error( "Can not get user from database, invalid user data in database" );
    }

    CryptoHelper::PublicKey publicKey{ CryptoHelper::PublicKey::fromPem( publicKeyPem ) };
    return std::make_shared<UserAccount>( name, publicKey, publicKey.hash(), Permission( permission ) );
}

// TODO: check that the account is still valid?
void Database::mapUser( map_user_f fn, const std::string& userHash ) const {
    // TODO: do everything with a single query with some join magic to replace sql "pivot"
    Database::Statement userId( *db.get(),
            "SELECT version "
            "FROM Attribute "
            "WHERE name='id' AND value=? AND version IN ( "
                "SELECT DISTINCT version "
                "FROM Object "
                "WHERE parent=? "
            ") "
            "ORDER BY version DESC " );
    userId << userHash << USERS_OBJECT_ID;
    if( !userId.executeStep() ) {
        // Not found
        return;
    }

    Database::Statement user( *db.get(),
            "SELECT accessDomain, id, version, name, value "
            "FROM Attribute "
            "WHERE version=? ");
    user << userId.getColumn( "version" ).getString();

    std::string id{}, name{}, permission{}, publicKey{};
    for( int i{0}; i < 4; ++i ) {
        if ( !user.executeStep() ) {
            throw std::runtime_error("Error in mapUser"); // TODO
        }
        std::string what{ user.getColumn( "name" ).getString() };
        if ( "id" == what ) {
            id = user.getColumn( "value" ).getString();
        } else if ( "name" == what ) {
            name = user.getColumn( "value" ).getString();
        } else if ( "permission" == what ) {
            permission = user.getColumn( "value" ).getString();
        } else if ( "publicKey" == what ) {
            publicKey = user.getColumn( "value" ).getString();
        }
    }
    if ( id.empty() || name.empty() || permission.empty() || publicKey.empty() ) {
        throw std::runtime_error("Error in mapUser"); // TODO
    }

    fn( id, name, permission, publicKey );
}

void Database::mapUsers( map_user_f fn ) const {
    // TODO: do everything with a single query with some join magic to replace sql "pivot"
    Database::Statement userId( *db.get(),
            "SELECT version "
            "FROM Attribute "
            "WHERE name='id' AND version IN ( "
                "SELECT DISTINCT version "
                "FROM Object "
                "WHERE parent=? "
            ") "
            "ORDER BY value ASC " );
    userId << USERS_OBJECT_ID;

    Database::Statement user( *db.get(),
            "SELECT accessDomain, id, version, name, value "
            "FROM Attribute "
            "WHERE version=? ");

    while( userId.executeStep() ) {
        user << userId.getColumn( "version" ).getString();

        std::string id{}, name{}, permission{}, publicKey{};
        for( int i{0}; i < 4; ++i ) {
            if ( !user.executeStep() ) {
                throw std::runtime_error("Error in mapUsers"); // TODO
            }
            std::string what{ user.getColumn( "name" ).getString() };
            if ( "id" == what ) {
                id = user.getColumn( "value" ).getString();
            } else if ( "name" == what ) {
                name = user.getColumn( "value" ).getString();
            } else if ( "permission" == what ) {
                permission = user.getColumn( "value" ).getString();
            } else if ( "publicKey" == what ) {
                publicKey = user.getColumn( "value" ).getString();
            }
        }
        if ( id.empty() || name.empty() || permission.empty() || publicKey.empty() ) {
            throw std::runtime_error("Error in mapUsers"); // TODO
        }

        fn( id, name, permission, publicKey );
        user.clearBindings();
        user.reset();
    }
}

void Database::mapUsersFrom( map_user_f fn, const std::vector<std::string>& userIds ) const {
    // TODO: do everything with a single query with some join magic to replace sql "pivot"
    std::string queryUserIds{
        "SELECT version "
        "FROM Attribute "
        "WHERE name='id' AND version IN ( "
            "SELECT DISTINCT version "
            "FROM Object "
            "WHERE parent=? "
        ") AND value IN ( "
    };
    for ( const std::string& id : userIds ) {
        queryUserIds += "\"" + id + "\",";
    }
    queryUserIds.pop_back();
    queryUserIds += " )"
            "ORDER BY value ASC ";

    Database::Statement userId( *db.get(), queryUserIds );
    userId << USERS_OBJECT_ID;

    Database::Statement user( *db.get(),
            "SELECT accessDomain, id, version, name, value "
            "FROM Attribute "
            "WHERE version=? ");

    while( userId.executeStep() ) {
        user << userId.getColumn( "version" ).getString();

        std::string id{}, name{}, permission{}, publicKey{};
        for( int i{0}; i < 4; ++i ) {
            if ( !user.executeStep() ) {
                throw std::runtime_error("Error in mapUsersFrom"); // TODO
            }
            std::string what{ user.getColumn( "name" ).getString() };
            if ( "id" == what ) {
                id = user.getColumn( "value" ).getString();
            } else if ( "name" == what ) {
                name = user.getColumn( "value" ).getString();
            } else if ( "permission" == what ) {
                permission = user.getColumn( "value" ).getString();
            } else if ( "publicKey" == what ) {
                publicKey = user.getColumn( "value" ).getString();
            }
        }
        if ( id.empty() || name.empty() || permission.empty() || publicKey.empty() ) {
            throw std::runtime_error("Error in mapUsersFrom"); // TODO
        }

        fn( id, name, permission, publicKey );
        user.clearBindings();
        user.reset();
    }
}

void Database::inviteUser( const UserAccount& user ) {
    std::map<std::string, Database::Value> attribute{};

    Database::Value id, name, permission, publicKey;
    id = user.getPublicKeyHash().toString();
    name = user.getName();
    permission = user.getPermission().toString();
    publicKey = user.getPublicKey().toString();

    attribute.emplace( "id", id );
    attribute.emplace( "name", name );
    attribute.emplace( "permission", permission );
    attribute.emplace( "publicKey", publicKey );

    std::unique_ptr<Mist::Transaction> transaction{ beginTransaction( AccessDomain::Settings ) };
    ObjectRef USER{ AccessDomain::Settings, USERS_OBJECT_ID };
    unsigned long objId{ transaction->newObject( USER, attribute ) };
    transaction->commit();
}

Database::Object Database::getObject( int accessDomain, long long id, bool includeDeleted ) const {
    return getObject( db.get(), accessDomain, id, includeDeleted );
}

Database::Object Database::getObject( Connection* connection, int accessDomain, long long id, bool includeDeleted ) const {
    Database::Statement object( *connection,
            "SELECT accessDomain, id, MAX( version ) as version, status, parent, parentAccessDomain, transactionAction "
            "FROM Object "
            "WHERE accessDomain=? AND id=? "
            "ORDER BY version DESC ");
    object << accessDomain << id;

    Database::Statement attribute( *connection,
            "SELECT accessDomain, id, version, name, type, value "
            "FROM Attribute "
            "WHERE accessDomain=? AND id=? AND version=? "
            "ORDER BY version DESC ");

    if( object.executeStep() ) {
        ObjectStatus status{ static_cast<ObjectStatus>( object.getColumn( "status" ).getUInt() ) };
        if ( ObjectStatus::Current != status || !includeDeleted ) {
            LOG( DBUG ) << "Object not found";
            throw Exception( Error::ErrorCode::NotFound );
        }

        std::map<std::string,Value> attributes{};
        attribute << accessDomain << id << object.getColumn( "version" ).getUInt();
        while ( attribute.executeStep() ) {
            attributes.emplace(
                    attribute.getColumn( "name" ).getString(),
                    statementRowToValue( attribute )
            );
        }
        return statementRowToObject( object, attributes );
    } else {
        LOG( DBUG ) << "Object not found";
        throw Exception( Error::ErrorCode::NotFound );
    }
}
/*
Database::Value Database::queryRowToValue( SQLite::Statement& query ) {
    Value::Type type{ static_cast<Database::Value::Type>( query.getColumn( "type" ).getInt() ) };
    switch( type ) {
    case Value::Type::Typeless:
        return {};
    case Value::Type::Null:
        return { nullptr };
    case Value::Type::Boolean:
        return static_cast<bool>( query.getColumn( "value" ).getUInt() );
    case Value::Type::Number:
        return query.getColumn( "value" ).getDouble();
    case Value::Type::String:
        return query.getColumn( "value" ).getString();
    case Value::Type::Json:
        return { query.getColumn( "value" ).getString(), true };
    default:
        LOG( WARNING ) << "Unhandled case, this needs to be fixed in the source code!";
        throw std::logic_error( "Unhandled case" );
    }
}
//*/
unsigned Database::subscribeObject( std::function<void(Object)> cb, int accessDomain,
        long long id, bool includeDeleted ) {
    ++subId;
    if ( !includeDeleted ) {
        LOG( DBUG ) << "Checking if object exists";
        getObject( accessDomain, id, includeDeleted ); // Test if the object exists, otherwise throw
    }
    try {
        objectSubscribers.at( id ).insert( subId );
    } catch ( const std::out_of_range& ) {
        // No subscirbers for this object yet
        objectSubscribers[ id ] = {};
        objectSubscribers.at( id ).insert( subId );
    }
    try {
        subscriberObjects.at( subId ).insert( id );
    } catch ( const std::out_of_range& ) {
        // No objects for this subscriber yet.
        subscriberObjects[ subId ] = {};
        subscriberObjects.at( subId ).insert( id );
    }
    objectSubscriberCallback[ subId ] = cb;

    return subId;
}

void Database::unsubscribe( unsigned subId ) {
    LOG( DBUG ) << "Unsubscribing";
    try {
        for( const long long& objId: subscriberObjects.at( subId ) ) {
            objectSubscribers.at( objId ).erase( subId );
            if ( objectSubscribers.at( objId ).empty() ) {
                objectSubscribers.erase( objId );
            }
        }
        subscriberObjects.erase( subId );
    } catch ( const std::out_of_range& ) {
        LOG( DBUG ) << "Tried to remove non-subscriber";
    }
    objectSubscriberCallback.erase( subId );
    querySubscriberCallback.erase( subId );
}

Database::QueryResult Database::query( int accessDomain, long long id, const std::string& select,
        const std::string& filter, const std::string& sort,
        const std::map<std::string, Value>& args,
        int maxVersion, bool includeDeleted ) {
    return query( db.get(), accessDomain, id, select, filter, sort, args, maxVersion, includeDeleted );
}

Database::QueryResult Database::query( Connection* connection, int accessDomain, long long id, const std::string& select,
        const std::string& filter, const std::string& sort,
        const std::map<std::string, Value>& args,
        int maxVersion, bool includeDeleted ) {
    Mist::Query querier{};
    querier.parseQuery( accessDomain, id, select, filter, sort, valueMapToArgumentMap( args ), maxVersion, includeDeleted );
    return query( querier, connection );
}


Database::QueryResult Database::query( const Query& querier, Connection* connection ) {
    Database::Statement dbQuery( *connection, querier.getSqlQuery() );
    QueryResult result{};
    if ( querier.isFunctionCall() ) {
        try {
            if ( !dbQuery.executeStep() ) {
                LOG( DBUG ) << "Query failed";
                // TODO: what should happen when the query fails?
                throw Exception( Error::ErrorCode::NotFound );
            }
        } catch ( const SQLite::Exception& e ) {
            LOG( WARNING ) << "Unexpected database error";
            throw Exception( e.what(), Error::ErrorCode::UnexpectedDatabaseError );
        }
        result.isFunctionCall = true;
        result.functionName = querier.getFunctionName();
        result.functionAttribute = querier.getFunctionAttribute();
        result.functionValue = dbQuery.getColumn( "value" ).getDouble();
    } else {
        if( dbQuery.executeStep() ) {
            // TODO: fix queries so we get all info needed?
            result.objects.push_back({
                static_cast<AccessDomain>( dbQuery.getColumn( "_accessDomain" ).getInt() ),
                static_cast<unsigned long>( dbQuery.getColumn( "_id" ).getUInt() ),
                static_cast<unsigned>( dbQuery.getColumn( "_version" ) ),
                { AccessDomain::Normal, 0 }, // TODO
                { { dbQuery.getColumn( "name" ).getString(), statementRowToValue( dbQuery ) } },
                ObjectStatus::Current, // TODO
                ObjectAction::New // TODO
            });
        } else {
	  //            LOG( DBUG ) << "No results";
	  //            throw Exception( Error::ErrorCode::NotFound );
	  return result;
        }
        while ( dbQuery.executeStep() ) {
            Object& object{ *(result.objects.end() - 1) };
            unsigned long id{ dbQuery.getColumn( "_id" ).getUInt() };
            unsigned version{ dbQuery.getColumn( "_version" ) };
            std::string name{ dbQuery.getColumn( "name" ).getString() };
            Value value{ statementRowToValue( dbQuery ) };
            if ( id == object.id && version == object.version ) {
                object.attributes.emplace( name, value );
            } else {
                result.objects.push_back({
                    static_cast<AccessDomain>( dbQuery.getColumn( "_accessDomain" ).getInt() ),
                    id,
                    version,
                    { AccessDomain::Normal, 0 }, // TODO
                    { { name, value } },
                    ObjectStatus::Current, // TODO
                    ObjectAction::New // TODO
                });
            }
        }
    }
    return result;
}

unsigned Database::subscribeQuery( std::function<void(QueryResult)> cb,
            int accessDomain, long long id, const std::string& select,
            const std::string& filter, const std::string& sort,
            const std::map<std::string, Value>& args,
            int maxVersion, bool includeDeleted ) {

    ++subId;
    if ( !includeDeleted ) {
        // TODO: verify this behaviour
        LOG( DBUG ) << "Checking if object exists";
        getObject( accessDomain, id, includeDeleted ); // Test if the object exists, otherwise throw
    }
    try {
        objectSubscribers.at( id ).insert( subId );
    } catch ( const std::out_of_range& ) {
        // No subscirbers for this object yet
        objectSubscribers[ id ] = {};
        objectSubscribers.at( id ).insert( subId );
    }
    try {
        subscriberObjects.at( subId ).insert( id );
    } catch ( const std::out_of_range& ) {
        // No objects for this subscriber yet.
        subscriberObjects[ subId ] = {};
        subscriberObjects.at( subId ).insert( id );
    }
    querySubscriberCallback[ subId ] =  {};
    querySubscriberCallback.at( subId ).first.reset( new Query() );
    querySubscriberCallback.at( subId ).second = cb;
    querySubscriberCallback.at( subId ).first->parseQuery( accessDomain, id, select, filter, sort, valueMapToArgumentMap( args ), maxVersion, includeDeleted );

    return subId;
}


Database::QueryResult Database::queryVersion( int accessDomain, long long id, const std::string& select,
        const std::string& filter, const std::map<std::string, Value>& args, bool includeDeleted ) {
    return queryVersion( db.get(), accessDomain, id, select, filter, args, includeDeleted );
}

Database::QueryResult Database::queryVersion( Connection* connection, int accessDomain, long long id, const std::string& select,
        const std::string& filter, const std::map<std::string, Value>& args, bool includeDeleted ) {
    Query querier{};
    querier.parseVersionQuery( accessDomain, id, select, filter, valueMapToArgumentMap( args ), includeDeleted );
    return queryVersion( querier, connection );
}

Database::QueryResult Database::queryVersion( const Query& querier, Connection* connection ) {
    Database::Statement dbQuery( *connection, querier.getSqlQuery() );
    QueryResult result{};
    if ( querier.isFunctionCall() ) {
        try {
            if ( !dbQuery.executeStep() ) {
                LOG( DBUG ) << "Query failed";
                // TODO: what should happen when the query fails?
                throw Exception( Error::ErrorCode::NotFound );
            }
        } catch ( const SQLite::Exception& e ) {
            LOG( WARNING ) << "Unexpected database error";
            throw Exception( e.what(), Error::ErrorCode::UnexpectedDatabaseError );
        }
        result.isFunctionCall = true;
        result.functionName = querier.getFunctionName();
        result.functionAttribute = querier.getFunctionAttribute();
        result.functionValue = dbQuery.getColumn( "value" ).getDouble();
    } else {
        if( dbQuery.executeStep() ) {
            // TODO: fix queries so we get all info needed?
            result.objects.push_back({
                AccessDomain::Normal, // TODO //static_cast<AccessDomain>( dbQuery.getColumn( "accessDomain" ).getInt() ),
                static_cast<unsigned long>( dbQuery.getColumn( "_id" ).getUInt() ),
                static_cast<unsigned>( dbQuery.getColumn( "_version" ) ),
                { AccessDomain::Normal, 0 }, // TODO
                { { dbQuery.getColumn( "name" ).getString(), statementRowToValue( dbQuery ) } },
                ObjectStatus::Current, // TODO
                ObjectAction::New // TODO
            });
        } else {
            LOG( DBUG ) << "No results";
            throw Exception( Error::ErrorCode::NotFound );
        }
        while ( dbQuery.executeStep() ) {
            Object& object{ *(result.objects.end() - 1) };
            unsigned long id{ dbQuery.getColumn( "_id" ).getUInt() };
            unsigned version{ dbQuery.getColumn( "_version" ) };
            std::string name{ dbQuery.getColumn( "name" ).getString() };
            Value value{ statementRowToValue( dbQuery ) };
            if ( id == object.id && version == object.version ) {
                object.attributes.emplace( name, value );
            } else {
                result.objects.push_back({
                    AccessDomain::Normal, // TODO //static_cast<AccessDomain>( dbQuery.getColumn( "accessDomain" ).getInt() ),
                    id,
                    version,
                    { AccessDomain::Normal, 0 }, // TODO
                    { { name, value } },
                    ObjectStatus::Current, // TODO
                    ObjectAction::New // TODO
                });
            }
        }
    }
    return result;
}

unsigned Database::subscribeQueryVersion( std::function<void(QueryResult)> cb,
        int accessDomain, long long id, const std::string& select,
        const std::string& filter, const std::map<std::string, Value>& args,
        bool includeDeleted ) {

    ++subId;
    if ( !includeDeleted ) {
        // TODO: verify this behaviour
        LOG( DBUG ) << "Checking if object exists";
        getObject( accessDomain, id, includeDeleted ); // Test if the object exists, otherwise throw
    }
    try {
        objectSubscribers.at( id ).insert( subId );
    } catch ( const std::out_of_range& ) {
        // No subscirbers for this object yet
        objectSubscribers[ id ] = {};
        objectSubscribers.at( id ).insert( subId );
    }
    try {
        subscriberObjects.at( subId ).insert( id );
    } catch ( const std::out_of_range& ) {
        // No objects for this subscriber yet.
        subscriberObjects[ subId ] = {};
        subscriberObjects.at( subId ).insert( id );
    }
    querySubscriberCallback[ subId ] =  {};
    querySubscriberCallback.at( subId ).first.reset( new Query() );
    querySubscriberCallback.at( subId ).second = cb;
    querySubscriberCallback.at( subId ).first->parseVersionQuery( accessDomain, id, select, filter, valueMapToArgumentMap( args ), includeDeleted );

    return subId;
}

std::unique_ptr<Mist::RemoteTransaction> Database::beginRemoteTransaction(
        Database::AccessDomain accessDomain,
        std::vector<Database::Transaction> parents,
        Helper::Date timestamp,
        CryptoHelper::SHA3 userHash,
        CryptoHelper::SHA3 hash,
        CryptoHelper::Signature signature ) {
    LOG ( DBUG ) << "Begin remote transaction";
    if(!_isOK) {
        LOG ( WARNING ) << "Invalid database state: Can not begin remote transaction";
        throw std::runtime_error( "Can not begin remote transaction, database error state." );
    }

    if( !manifest || !( manifest->getCreator().hash() == userHash ) ) {
        std::shared_ptr<UserAccount> userAccount{ nullptr };
        try {
            userAccount = getUser( userHash.toString() );
        } catch ( const std::runtime_error& e ) {
            LOG( WARNING ) << "Could not get user: " << e.what();
        }

        if ( !userAccount || userAccount->getPermission() == Permission::P::read ) {
            LOG ( WARNING ) << "Invalid user or user permission";
            throw Mist::Exception( Mist::Error::ErrorCode::AccessDenied );
        }
    }

    Database::Statement query(*db.get(), "SELECT IFNULL(MAX(version),0)+1 AS newVersion "
            "FROM 'Transaction'");
    if ( !query.executeStep() ) {
        _isOK = false;
        LOG ( WARNING ) << "Invalid database state: Can not begin remote transaction";
        throw std::runtime_error( "Invalid database state: Can not begin remote transaction" );
    }
    const unsigned version{ query.getColumn( "newVersion" ).getUInt() };
    return std::unique_ptr<Mist::RemoteTransaction>(
        new Mist::RemoteTransaction (
            this,
            accessDomain,
            version,
            parents,
            timestamp,
            userHash,
            hash,
            signature
        )
    );
}

std::unique_ptr<Mist::Transaction> Database::beginTransaction( AccessDomain accessDomain ) {
    unsigned newVersion;

    LOG ( DBUG ) << "Begin transaction";
    // TODO: Is this completely wrong?
    if(!_isOK) {
        LOG ( WARNING ) << "Invalid database state: Can not begin transaction";
        throw std::runtime_error( "Invalid database state: Can not begin transaction" );
    }
    //Helper::Database::Transaction newVersion( *db );
    Database::Statement getVersion(*db.get(), "SELECT MAX(version) AS version, timestamp, STRFTIME('%Y-%m-%d %H:%M:%f','now') AS now "
            "FROM 'Transaction'");
    if ( !getVersion.executeStep() ) {
        _isOK = false;
        LOG ( WARNING ) << "Invalid database state: Can not begin transaction";
        throw std::runtime_error( "Invalid database state: Can not begin transaction" );
    }
    if ( getVersion.getColumn( "version" ).isNull()) {
        newVersion = 1;
    } else if (getVersion.getColumn( "timestamp" ).getString() == getVersion.getColumn( "now" ).getString()) {
      newVersion = getVersion.getColumn( "version" ).getUInt() + 1;
//      usleep( 1 );
    } else if (getVersion.getColumn( "timestamp" ).getString() > getVersion.getColumn( "now" ).getString()) {
        LOG ( WARNING ) << "Last transaction is from the future. Cannot begin a new transaction";
        throw std::runtime_error( "Last transaction is from the future. Cannot begin a new transaction" );
    } else {
      newVersion = getVersion.getColumn( "version" ).getUInt() + 1;
    }

    //Transaction* transaction{ new Transaction( this, accessDomain, query.getColumn("newVersion").getUInt() ) };
    //newVersion.commit();
    return std::unique_ptr<Mist::Transaction>(
        new Mist::Transaction(
            this,
            accessDomain,
	    newVersion
        )
    );
}

std::unique_ptr<Database::Connection> Database::getIsolatedDbConnection() const {
    try {
        return std::unique_ptr<Connection>( new Connection( path, Helper::Database::OPEN_READWRITE ) );
    } catch ( SQLite::Exception& e ) {
        LOG ( WARNING ) << "Database error: " << e.what();
        throw e;
    } catch (...) {
        LOG ( WARNING ) << "Database error.";
        throw std::runtime_error("Database error");
    }
}

CryptoHelper::SHA3 Database::calculateTransactionHash(
        const Database::Transaction& transaction,
        Connection* connection ) const {
    CryptoHelper::SHA3Hasher hasher{};
    StreamToString<> streamer{
        std::bind(
                (void(CryptoHelper::SHA3Hasher::*)(const std::string&))&CryptoHelper::SHA3Hasher::update,
                std::ref( hasher ),
                std::placeholders::_1 )
    };
    try {
        readTransactionBody( std::ref( streamer ), transaction.version, connection );
    } catch (...) {
        LOG( WARNING ) << "Transaction hashing failed: Could not read transaction body.";
        throw std::runtime_error("Transaction hashing failed");
    }
    if ( std::char_traits<char>::eof() == streamer.pubsync() ) {
        LOG( WARNING ) << "Can not sync streamer." ;
        throw std::runtime_error( "Can not sync streamer." ); // TODO
    }
    try {
        LOG( DBUG ) << "Hasher finalize";
        return hasher.finalize();
    } catch (...) {
        LOG( WARNING ) << "Transaction hashing failed: hasher finalize failed.";
        throw std::runtime_error("Transaction hasing failed");
    }
}

// TODO: Move the method to the db as a trigger after insert
unsigned Database::reorderTransaction( const Database::Transaction& tranasaction,
        Connection* connection ) {
    LOG ( DBUG ) <<
            "Reordering: version: " << tranasaction.version <<
            " date: " << tranasaction.date.toString() <<
            " hash: " << tranasaction.hash.toString();
    Connection* conn{ db.get() };
    if ( nullptr != connection ) {
        conn = connection;
    }

    std::unique_ptr<Helper::Database::SavePoint> savePoint;
    try {
        savePoint.reset( new Helper::Database::SavePoint( conn, "reorder" ) );
    } catch (...) {
        LOG( WARNING ) << "SavePoint constructor failed.";
        throw std::runtime_error("SavePoint constructor failed");
    }

    Statement newerTransaction( *conn,
            "SELECT version, timestamp "
            "FROM 'Transaction' "
            "WHERE timestamp >= ? "
            "ORDER BY version DESC ");

    Statement sameTimeTransaction( *conn,
            "SELECT version, hash "
            "FROM 'Transaction' "
            "WHERE timestamp >= ? "
            "ORDER BY version ASC ");

    std::vector<std::unique_ptr<Statement>> versionUp;
    versionUp.emplace_back( new Statement( *conn, "UPDATE Attribute SET version=version+1 WHERE version=? " ) );
    versionUp.emplace_back( new Statement( *conn, "UPDATE Object SET version=version+1 WHERE version=? " ) );
    versionUp.emplace_back( new Statement( *conn, "UPDATE 'Transaction' SET version=version+1 WHERE version=? " ) );
    versionUp.emplace_back( new Statement( *conn, "UPDATE TransactionParent SET parentVersion=parentVersion+1 WHERE parentVersion=? " ) );
    versionUp.emplace_back( new Statement( *conn, "UPDATE TransactionParent SET version=version+1 WHERE version=? " ) );
    // TODO: Up version in "Renumber" table
    // TODO: Up version in "AccessDomain" table
    std::vector<std::unique_ptr<Statement>> versionDown;
    versionDown.emplace_back( new Statement( *conn, "UPDATE Attribute SET version=version-1 WHERE version=? " ) );
    versionDown.emplace_back( new Statement( *conn, "UPDATE Object SET version=version-1 WHERE version=? " ) );
    versionDown.emplace_back( new Statement( *conn, "UPDATE 'Transaction' SET version=version-1 WHERE version=? " ) );
    versionDown.emplace_back( new Statement( *conn, "UPDATE TransactionParent SET parentVersion=parentVersion-1 WHERE parentVersion=? " ) );
    versionDown.emplace_back( new Statement( *conn, "UPDATE TransactionParent SET version=version-1 WHERE version=? " ) );
    // TODO: Down version in "Renumber" table
    // TODO: Down version in "AccessDomain" table
    std::vector<std::unique_ptr<Statement>> versionSet;
    versionSet.emplace_back( new Statement( *conn, "UPDATE Attribute SET version=? WHERE version=? " ) );
    versionSet.emplace_back( new Statement( *conn, "UPDATE Object SET version=? WHERE version=? " ) );
    versionSet.emplace_back( new Statement( *conn, "UPDATE 'Transaction' SET version=? WHERE version=? " ) );
    versionSet.emplace_back( new Statement( *conn, "UPDATE TransactionParent SET parentVersion=? WHERE parentVersion=? " ) );
    versionSet.emplace_back( new Statement( *conn, "UPDATE TransactionParent SET version=? WHERE version=? " ) );
    // TODO: Set version in "Renumber" table
    // TODO: Set version in "AccessDomain" table

    newerTransaction << tranasaction.date.toString();
    unsigned version{ 0 };

    // Add one to version for all newer transactions, including the current one
    // to create a gap that we can move this transaction to.
    // Starting with the highest version
    while( newerTransaction.executeStep() ) {
        version = newerTransaction.getColumn( "version" ).getUInt();
        for( std::unique_ptr<Statement>& s : versionUp ) {
            *s << version;
            s->exec();
            s->clearBindings();
            s->reset();
        }
    }

    // If the version is the same it was in the correct datetime place,
    // or if there are no newer transactions
    if ( tranasaction.version == version || 0 == version ) {
        savePoint.reset();
        return version;
    }

    // Sort by id if the datetime is the same
    sameTimeTransaction << tranasaction.date.toString();
    while( sameTimeTransaction.executeStep() ) {
        if ( tranasaction.hash < CryptoHelper::SHA3(
                sameTimeTransaction.getColumn( "hash" ).getString() ) ) {
            // Break the loop if the current transaction has the smaller hash
            version = sameTimeTransaction.getColumn( "version" ).getUInt() - 1;
            LOG ( DBUG ) << "Reordering: sametime version: " << version;
            break;
        }
        version = sameTimeTransaction.getColumn( "version" ).getUInt();
        LOG ( DBUG ) << "Reordering: sametime version: " << version;
        for( std::unique_ptr<Statement>& s : versionDown ) {
            *s << version;
            s->exec();
            s->clearBindings();
            s->reset();
        }
    }
    // If the version is the same it was in the correct datetime place,
    // therefore nothing needs to be done
    if ( version == tranasaction.version ) {
        savePoint.reset();
        return version;
    }

    // Set version for the current transaction to the newly freed gap
    for( std::unique_ptr<Statement>& s : versionSet ) {
        *s << version << tranasaction.version + 1; // Add one to reflect the update in the database
        s->exec();
        s->clearBindings();
        s->reset();
    }

    savePoint->save(); // Same as commit for a regular transaction begin
    savePoint.reset();
    return version;
}

CryptoHelper::Signature Database::signTransaction( const CryptoHelper::SHA3& hash ) const {
    return nullptr == central ? CryptoHelper::Signature() : central->sign( hash );
}

void Database::objectChanged( const Database::ObjectRef& objectRef ) {
    if ( AccessDomain::Settings == objectRef.accessDomain && USERS_OBJECT_ID == objectRef.id ) {
        // Handle user changes
        Database::Statement affectedUsers( *db.get(),
                "SELECT id, max(version) AS version, status "
                "FROM Object WHERE accessDomain=? AND parent=? "
                "GROUP BY id " );

        Database::Statement user( *db.get(),
                "SELECT accessDomain, id, version, name, value "
                "FROM Attribute "
                "WHERE accessDomain=? AND id=? AND version=? ");

        affectedUsers << static_cast<int>( AccessDomain::Settings )
            << USERS_OBJECT_ID;
        while( affectedUsers.executeStep() ) {
            user << static_cast<int>( AccessDomain::Settings )
                << affectedUsers.getColumn( "id" ).getInt64()
                << affectedUsers.getColumn( "version" ).getUInt();
            std::string id{}, name{}, permission{}, publicKeyPem{};
            //for( int i{0}; i < 4; ++i ) {
            while( user.executeStep() ) {
                std::string what{ user.getColumn( "name" ).getString() };
                if ( "id" == what ) {
                    id = user.getColumn( "value" ).getString();
                } else if ( "name" == what ) {
                    name = user.getColumn( "value" ).getString();
                } else if ( "permission" == what ) {
                    permission = user.getColumn( "value" ).getString();
                } else if ( "publicKey" == what ) {
                    publicKeyPem = user.getColumn( "value" ).getString();
                }
            }
            if ( id.empty() || name.empty() || permission.empty() || publicKeyPem.empty() ) {
                LOG( WARNING ) << "Can not get user from database";
                // Continue with the next user
                user.clearBindings();
                user.reset();
                continue;
            }

            // TODO: add/remove user from central.
            if ( ObjectStatus::Current != static_cast<ObjectStatus>( affectedUsers.getColumn( "status" ).getUInt() ) ) {
		central->removeDatabasePermission( CryptoHelper::PublicKeyHash::fromString( id ), getManifest()->getHash() );
		//                central->removePeer( CryptoHelper::PublicKeyHash::fromString( id ) );
            } else {
                central->addPeer( CryptoHelper::PublicKey::fromPem( publicKeyPem ), name, PeerStatus::IndirectAnonymous, true );
		central->addDatabasePermission( CryptoHelper::PublicKeyHash::fromString( id ), getManifest()->getHash() );
            }
            user.clearBindings();
            user.reset();
        }
    }
    try {
        const std::set<unsigned>& subs { objectSubscribers.at( objectRef.id ) };
        Object obj{ getObject( static_cast<long long>( objectRef.accessDomain ),
                static_cast<long long>( objectRef.id ), true ) };
        for ( const unsigned sub : subs ) {
            objectSubscriberCallback.at( sub )( obj );
        }
    } catch ( const std::out_of_range& ) {
        // OK, no subscribers found
    }
}

void Database::objectsChanged( const std::set<Database::ObjectRef, Database::lessObjectRef>& objects ) {
    std::set<unsigned> subscribers{};
    for( const ObjectRef& objectRef: objects ) {
        objectChanged( objectRef );
        try {
            const std::set<unsigned>& subs { objectSubscribers.at( objectRef.id ) };
            Object obj{ getObject( static_cast<long long>( objectRef.accessDomain ),
                    static_cast<long long>( objectRef.id ), true ) };
            for ( const unsigned sub : subs ) {
                if ( objectSubscriberCallback.count( sub ) ) {
                    objectSubscriberCallback.at( sub )( obj );
                }
                if (  querySubscriberCallback.count( sub ) ) {
                    QueryResult newQueryResults{ query( *querySubscriberCallback.at( sub ).first.get(), db.get() ) };
                    querySubscriberCallback.at( sub ).second( newQueryResults );
                }
            }
        } catch ( const std::out_of_range& ) {
            // OK, no subscribers found
        }
    }
}

void Database::rollback( Mist::RemoteTransaction* transaction ) {
    // TODO
    LOG ( DBUG ) << "TODO: Remote transaction rollback";
}

void Database::rollback( Mist::Transaction* transaction ) {
    // TODO
    LOG ( DBUG ) << "TODO: Transaction rollback";
}

void Database::commit( Mist::RemoteTransaction* transaction ) {
    // TODO
    LOG ( DBUG ) << "TODO: Remote transaction commit";
}

void Database::commit( Mist::Transaction* transaction ) {
    // TODO
    LOG ( DBUG ) << "TODO: Transaction commit";
}

std::vector<CryptoHelper::SHA3> Database::getParents( unsigned version,
        Connection* connection ) const {
    Database::Statement parent( *db.get(),
            "SELECT hash"
            "FROM TransactionParent AS tp, 'Transaction' AS t "
            "WHERE tp.version=? AND t.accessDomain=tp.parentAccessDomain AND t.version=tp.parentVersion "
            "ORDER BY hash ");
    parent << version;

    std::vector<CryptoHelper::SHA3> parents;
    while ( parent.executeStep() ) {
        parents.push_back( CryptoHelper::SHA3::fromString( parent.getColumn( "hash" ).getString() ) );
    }
    return parents;
}

std::vector<CryptoHelper::SHA3> Database::getParents( CryptoHelper::SHA3 transactionHash,
        Connection* connection ) const {
    Database::Statement parent( *db.get(),
            "SELECT hash "
            "FROM TransactionParent AS tp, 'Transaction' AS t "
            "WHERE t.hash=? AND tp.version=t.version AND tp.accessDomain=t.parentAccessDomain "
            "ORDER BY hash ");
    parent << transactionHash.toString();

    std::vector<CryptoHelper::SHA3> parents;
    while ( parent.executeStep() ) {
        parents.push_back( CryptoHelper::SHA3::fromString( parent.getColumn( "hash" ).getString() ) );
    }
    return parents;
}

/*
Database::Meta Database::transactionToMeta( const Database::Transaction& transaction,
        Connection* connection ) const {
    try {
        // TODO
    } catch ( const SQLite::Exception& e ) {
        LOG( WARNING ) << "Database error while trying to fetch transaction parents: " << e.what();
        throw e;
    }
}
//*/

Database::Transaction Database::statementRowToTransaction( Database::Statement& stmt ) {
    // "SELECT accessDomain, version, timestamp, userHash, hash, signature "
    try {
        Database::AccessDomain ad{ static_cast<Database::AccessDomain>( stmt.getColumn( "accessDomain" ).getUInt() ) };
        unsigned version{ stmt.getColumn( "version" ).getUInt() };
        Helper::Date timestamp{ stmt.getColumn( "timestamp" ).getString() };

        CryptoHelper::SHA3 creator{};
        CryptoHelper::SHA3 hash{};
        CryptoHelper::Signature signature{};

        std::string userHash{ stmt.getColumn( "userHash" ).getString() };
        std::string hashStr{ stmt.getColumn( "hash" ).getString() };
        std::string sig{ stmt.getColumn( "signature" ).getString() };

        if ( !userHash.empty() ) {
            creator = CryptoHelper::SHA3::fromString( userHash );
        }
        if ( !hashStr.empty() ) {
            hash = CryptoHelper::SHA3::fromString( hashStr );
        }
        if ( !sig.empty() ) {
            signature = CryptoHelper::Signature::fromString( sig );
        }

        return Database::Transaction{ ad, version, timestamp, creator, hash, signature };
    } catch( const SQLite::Exception& e ) {
        // TODO correct error handling
        LOG( WARNING ) << "Statement is not a correct transaction statement." ;
        throw std::runtime_error( "Statement is not a correct transaction statement." );
    }
}

Database::Object Database::statementRowToObject( Database::Statement& object, std::map<std::string, Database::Value> attributes ) {
    try {
        AccessDomain ad{ static_cast<AccessDomain>( object.getColumn( "accessDomain" ).getInt() ) };
        unsigned long id{ static_cast<unsigned long>( object.getColumn( "id" ).getInt64() ) };
        unsigned version{ object.getColumn( "version" ).getUInt() };
        unsigned pAd{ object.getColumn( "parentAccessDomain" ).getUInt() };
        if ( 0 == pAd ) {
            pAd = 2; // TODO: fix default ad
        }
        ObjectRef parent{
            static_cast<Database::AccessDomain>( pAd ),
            static_cast<unsigned long>( object.getColumn( "parent" ).getUInt() )
        };
        ObjectStatus status{ static_cast<ObjectStatus>( object.getColumn( "status" ).getInt() ) };
        ObjectAction action{ static_cast<ObjectAction>( object.getColumn( "transactionAction" ).getInt() ) };

        return Database::Object{ ad, id, version, parent, attributes, status, action };
    } catch( const SQLite::Exception& e ) {
        // TODO correct error handling
        LOG( WARNING ) << "Statement is not a correct object statement." ;
        throw std::runtime_error( "Statement is not a correct object statement." );
    }
}

Database::ObjectMeta Database::statementRowToObjectMeta( Database::Statement& object ) {
    try {
        AccessDomain ad{ static_cast<AccessDomain>( object.getColumn( "accessDomain" ).getInt() ) };
        unsigned long id{ static_cast<unsigned long>( object.getColumn( "id" ).getInt64() ) };
        unsigned version{ object.getColumn( "version" ).getUInt() };
        unsigned pAd{ object.getColumn( "parentAccessDomain" ).getUInt() };
        if ( 0 == pAd ) {
            pAd = 2; // TODO: fix default ad
        }
        ObjectRef parent{
            static_cast<Database::AccessDomain>( pAd ),
            static_cast<unsigned long>( object.getColumn( "parent" ).getUInt() )
        };
        ObjectStatus status{ static_cast<ObjectStatus>( object.getColumn( "status" ).getInt() ) };
        ObjectAction action{ static_cast<ObjectAction>( object.getColumn( "transactionAction" ).getInt() ) };

        return Database::ObjectMeta{ ad, id, version, parent, status, action };
    } catch( const SQLite::Exception& e ) {
        // TODO correct error handling
        LOG( WARNING ) << "Statement is not a correct object statement." ;
        throw std::runtime_error( "Statement is not a correct object statement." );
    }
}

Database::Value Database::statementRowToValue( Database::Statement& attribute ) {
    try {
        Value::Type type{ static_cast<Value::Type>( attribute.getColumn( "type" ).getInt() ) };
        switch( type ) {
        case Value::Type::Typeless:
            return {};
        case Value::Type::Null:
            return { nullptr };
        case Value::Type::Boolean:
            return { static_cast<bool>( attribute.getColumn( "value" ).getInt() ) };
        case Value::Type::Number:
            return { attribute.getColumn( "value" ).getDouble() };
        case Value::Type::String:
            return { attribute.getColumn( "value" ).getString() };
        case Value::Type::Json:
            return { attribute.getColumn( "value" ).getString(), true };
        default:
            LOG( WARNING ) << "Attribute statement does not contain correct type.";
            throw std::runtime_error( "Attribute statement does not contain correct type." );
        }
    } catch( const SQLite::Exception& e ) {
        LOG( WARNING ) << "Statement is not a correct attribute statement.";
        throw std::runtime_error( "Statement is not a correct attribute statement." );
    }
}

/*
UserAccount Database::statementRowToUser( Database::Statement& user ) {

}
//*/

bool Database::dbExists( std::string filename ) {
    bool exists = true;
    try {
        Connection( filename, Helper::Database::OPEN_READONLY );
    } catch ( Helper::Database::Exception &e ) {
        // TODO: check for other errors as well.
        exists = false;
    }
    return exists;
}

Database::Manifest *Database::getManifest() {
    return manifest.get();
}

bool Database::Value::operator<( const Value& rhs ) {
    if ( t == rhs.t ) {
        switch( t ){
        case Type::Typeless:
        case Type::Null:
            return false;
        case Type::Boolean:
            return b < rhs.b;
        case Type::Number:
            return n < rhs.n;
        case Type::String:
        case Type::Json:
            return v.compare( rhs.v ) < 0;
        }
    } else {
        return static_cast<int>( t ) < static_cast<int>( rhs.t );
    }
    return false;
}

Database::Manifest::Manifest( Signer signer, Verifier verifier, std::string name, Helper::Date created, CryptoHelper::PublicKey creator, CryptoHelper::Signature signature, CryptoHelper::SHA3 hash ) :
    name( name ), created( created ), creator( creator ), hash( hash ), signature( signature ), signer( signer ), verifier( verifier ) {
}

void Database::Manifest::sign() {
    if ( !signer )
        return;
    CryptoHelper::SHA3Hasher hasher{};
    hasher.update( inner() );
    hash = hasher.finalize();
    signature = signer( hash );
}

bool Database::Manifest::verify() const {
    // TODO Need to verify hash as well
    if ( !verifier ) {
        LOG( WARNING ) << "Manifest does not have a pointer to Central and can not verify the manifest.";
        throw std::runtime_error( "Manifest does not have a pointer to Central and can not verify the manifest." );
    }
    return verifier( creator, hash, signature );
}

std::string Database::Manifest::inner() const {
    std::stringstream ss{};
    JSON::Serialize s{};
    std::ostream os( ss.rdbuf() );
    s.set_ostream( os );
    s.start_object();
        s.put( "created" );
        s.put( created.toString() );
        s.put( "creator" );
        s.put( creator.toString() );
        s.put( "name" );
        s.put( name );
    s.close_object();
    return ss.str();
}

std::string Database::Manifest::toString() const {
    std::stringstream ss{};
    JSON::Serialize s{};
    std::ostream os( ss.rdbuf() );
    s.set_ostream( os );
    s.start_object();
        s.put( "hash" );
        s.put( hash.toString() );
        s.put( "manifest" );
        s.start_object();
            s.put( "created" );
            s.put( created.toString() );
            s.put( "creator" );
            s.put( creator.toString() );
            s.put( "name" );
            s.put( name );
        s.close_object();
        s.put( "signature" );
        s.put( signature.toString() );
    s.close_object();
    return ss.str();
}

Database::Manifest Database::Manifest::fromString( const std::string& serialized, Verifier verifier, Signer signer ) {
    JSON::Value json{ JSON::Deserialize::generate_json_value( serialized ) };

    std::string name{ json.at( "manifest" ).at( "name" ).get_string() };
    Helper::Date created { json.at( "manifest" ).at( "created" ).get_string() };
    CryptoHelper::PublicKey creator{ CryptoHelper::PublicKey::fromPem( json.at( "manifest" ).at( "creator" ).get_string() ) };
    CryptoHelper::Signature signature { CryptoHelper::Signature::fromString( json.at( "signature" ).get_string() ) };
    CryptoHelper::SHA3 hash { CryptoHelper::SHA3::fromString( json.at( "hash" ).get_string() ) };

    return Manifest( signer, verifier, name, created, creator, signature, hash );
}

Database::Manifest Database::Manifest::fromJSON( const JSON::Value& json, Verifier verifier, Signer signer ) {
    std::string name{ json.at( "manifest" ).at( "name" ).get_string() };
    Helper::Date created { json.at( "manifest" ).at( "created" ).get_string() };
    CryptoHelper::PublicKey creator{ CryptoHelper::PublicKey::fromPem( json.at( "manifest" ).at( "creator" ).get_string() ) };
    CryptoHelper::Signature signature { CryptoHelper::Signature::fromString( json.at( "signature" ).get_string() ) };
    CryptoHelper::SHA3 hash { CryptoHelper::SHA3::fromString( json.at( "hash" ).get_string() ) };

    return Manifest( signer, verifier, name, created, creator, signature, hash );
}

} /* namespace Mist */
