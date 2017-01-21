/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

/* Exchange format, 3 repetitions means that there can be 0 or more such entries
{
    id: string (hash of transaction),
    signature: string, id signed by private key of user,
    transaction: {
        metadata: {
            accessDomain: number,
            timestamp: string (iso standard date),
            user: string (hash of public key)
            parents: {
                hash: true,
                hash: true,
                hash: true,
            },
            version: 1.0,
        },
        objects: {
            changed: {
                id: { attributes: { name: value, name: value, name: value, } },
                id: { attributes: { name: value, name: value, name: value, } },
                id: { attributes: { name: value, name: value, name: value, } },
            },
            deleted: {
                id: true,
                id: true,
                id: true,
            },
            moved: {
                id: number | [ad, number],
                id: number | [ad, number],
                id: number | [ad, number],
            },
            new: {
                id: { parent: number | [ad, number], attributes: { name: value, name: value, name: value, } },
                id: { parent: number | [ad, number], attributes: { name: value, name: value, name: value, } },
                id: { parent: number | [ad, number], attributes: { name: value, name: value, name: value, } },
            }
        }
    }
}
*/

#ifndef INCLUDE_EXCHANGEFORMAT_H_
#define INCLUDE_EXCHANGEFORMAT_H_

#include <memory>
#include <stack>
#include <streambuf>
#include <string>
#include <vector>

#include "Database.h"
#include "Exception.h"
#include "JSONstream.h"

namespace Mist {

enum class ExchangeState : int;

class FormatException: public Mist::Exception {
public:
    explicit FormatException() : FormatException( "Invalid Mist 1.0 format." ) {}
    explicit FormatException( const std::string& what ) : Mist::Exception( std::string( "Format exception: " ) + what ) {}
    explicit FormatException( const char* what ) : Mist::Exception( std::string( "Format exception: " ) + std::string( what ) ) {}
};

using E = JSON::Event;
using S = ExchangeState;

class Serializer {
public:
    Serializer( Database* db );
    virtual ~Serializer() = default;

    virtual void readTransaction( std::basic_streambuf<char>& sb,
            const std::string& hash,
            Helper::Database::Database* connection );
    virtual void readTransactionBody( std::basic_streambuf<char>& sb,
            const std::string& hash,
            Helper::Database::Database* connection );
    virtual void readTransactionBody( std::basic_streambuf<char>& sb,
            unsigned version,
            Helper::Database::Database* connection );
    virtual void readTransactionList( std::basic_streambuf<char>& sb );
    virtual void readTransactionMetadata( std::basic_streambuf<char>& sb,
            const std::string& hash );
    virtual void readTransactionMetadataLatest( std::basic_streambuf<char>& sb );
    virtual void readTransactionMetadataFrom( std::basic_streambuf<char>& sb,
            const std::vector<std::string>& hashes );

    // TODO: change the user format? it does not contain any verifiable data
    virtual void readUser( std::basic_streambuf<char>& sb, const std::string& user );
    virtual void readUsers( std::basic_streambuf<char>& sb );
    virtual void readUsersFrom( std::basic_streambuf<char>& sb,
            const std::vector<std::string>& userIds );

protected:
    virtual void initReading( std::basic_streambuf<char>& sb ); // TODO: get isolated connection

    // TODO: change many of these to use Database::Meta instead!
    virtual void trans( const Database::Transaction& transaction,
            Helper::Database::Database* connection ) const;
    virtual void transBody( const Database::Transaction& transaction,
            Helper::Database::Database* connection ) const;
    virtual void meta( const Database::Transaction& transaction,
            Helper::Database::Database* connection ) const;
    virtual void hash( const Database::Transaction& transaction) const;

    virtual void changed( const Database::Object& object ) const;
    virtual void deleted( const Database::Object& object ) const;
    virtual void moved( const Database::Object& object) const;
    virtual void New( const Database::Object& object ) const;

    virtual void user( const std::string& id,
            const std::string& name,
            const std::string& permission,
            const std::string& publicKey ) const;

    Database* db;
    std::basic_streambuf<char>* sb;

    std::unique_ptr<JSON::Serialize> s;
    std::unique_ptr<std::basic_ostream<char>> os;
};

class Deserializer : public JSON::Event_handler {
public:
    Deserializer( Database* db );
    virtual ~Deserializer() = default;

    virtual void write( std::basic_streambuf<char>& sb );
    virtual void write( const char* buf, std::size_t len );
    virtual void write( const std::string& buf );

    static std::vector<Database::Meta> exchangeFormatToMeta( std::basic_streambuf<char>& sb );

    virtual void clear();

protected:
    using map_meta_f = Database::map_meta_f;
    Deserializer( map_meta_f cb );

    virtual void event( E e ) override;

    virtual void parse( E e ); // TODO: verify hash value before commit
    virtual void parseTransId( E e );
    virtual void parseSignature( E e );
    virtual void parseTransaction( E e );
    virtual void parseMetaData( E e ); // TODO: verify meta data
    virtual void parseAccessDomain( E e );
    virtual void parseTimestamp( E e );
    virtual void parseUser( E e );
    virtual void parseParents( E e );
    virtual void parseVersion( E e );
    virtual void parseObjects( E e );
    virtual void parseChanged( E e );
    virtual void parseDeleted( E e );
    virtual void parseMoved( E e );
    virtual void parseNew( E e );
    virtual void parseAttributes( E e ); // TODO: verify that the attributes are sorted

    virtual void formatError( std::string err = "" ); // TODO and remove this?

    virtual void startTransaction();
    virtual void commitTransaction();
    virtual void rollbackTransaction();
    virtual void changeObject();
    virtual void deleteObject();
    virtual void moveObject();
    virtual void newObject();

    virtual void pop();

    Database* db; // TODO: refactor to use weak pointer instead?
    bool alreadyExists{ false };
    map_meta_f cb;
    std::unique_ptr<JSON::Deserialize> d;
    std::unique_ptr<Mist::RemoteTransaction> transaction;
    std::stack<ExchangeState> state;
    std::shared_ptr<JSON::Event_receiver> event_receiver;

    // Store meta data since that is needed before we can start streaming to the database
    unsigned accessDomain{};
    std::string transactionId{}, signature{}, timestamp{}, user{}, version{};
    std::vector<std::string> parentIds{}; // TODO: should the parents be numeric instead of string hash?

    std::vector<Database::Transaction> parents{};

    // Store objects, since they can't be partially streamed with the current API
    unsigned objParentAd{};
    long long objParent{};
    std::string objId{}, attrName{};
    Database::Value value{};
    std::map<std::string, Database::Value> attributes{};
};

} /* namespace Mist */

#endif /* INCLUDE_EXCHANGEFORMAT_H_ */
