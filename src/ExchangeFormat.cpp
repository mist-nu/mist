/*
 * Format.cpp
 *
 *  Created on: Nov 26, 2016
 *      Author: andreas
 */

#include <ostream>
#include <stdexcept>

#include "CryptoHelper.h"
#include "ExchangeFormat.h"
#include "JSONstream.h"
#include "RemoteTransaction.h"

namespace Mist {

void putAttribute( JSON::Serialize* s, const std::pair<std::string,Database::Value>& attribute ) {
    s->put( attribute.first );
    using T = Database::Value::Type;
    switch( attribute.second.type() ) {
    case T::Typeless:
    case T::Null:
       s->put( nullptr );
       break;
    case T::Boolean:
       s->put( attribute.second.boolean() );
       break;
    case T::Number:
       s->put( static_cast<long double>( attribute.second.number() ) );
       break;
    case T::String:
    case T::Json:
       s->put( attribute.second.string() );
       break;
    default:
       // TODO: throw instread?
       LOG( WARNING ) << "Unhandled case";
       s->put( 0LL );
       break;
    }
}

enum class ExchangeState : int {
    Error = 0,

    Start,

    TransIdKeyword,
    TransIdHash,
    SigKey,
    SigHash,

    TransactionKeyword,
    TransactionObj,

    MetadataKeyword,
    MetadataObj,

    AccessDomainKeyword,
    AccessDomain,
    TimestampKeyword,
    Timestamp,
    UserKeyword,
    UserHash,

    ParentsKeyword,
    ParentsObj,
    ParentId,
    ParentDummy,

    VersionKeyword,
    Version,

    ObjectsKeyword,
    ObjectsObj,

    ChangedKeyword,
    ChangedObjects,
    ChangedId,
    ChangedObj,

    DeletedKeyword,
    DeletedObjects,
    DeletedId,
    DeletedDummy,

    MovedKeyword,
    MovedObjects,
    MovedId,
    MovedToParent,

    NewKeyword,
    NewObjects,
    NewId,
    NewObj,
    NewParentKeyword,
    NewParentId,

    AttributesKeyword,
    AttributesObj,
    AttributeName,
    AttributeValue,
};

using sb_t = std::basic_streambuf<char>;

void write( sb_t& sb, char c ) {
    if ( std::char_traits<char>::eof() == sb.sputc( c ) ) {
        throw FormatException( "Stream error while outputting exchange format data." );
    }
}

void write( sb_t& sb, const std::string& data ) {
    for( const char& c: data ) {
        write( sb, c );
    }
}

/*****************************************************************************/
using namespace std::placeholders;

Serializer::Serializer( Database* db ) : db( db ), sb( nullptr ), s{}, os{} {

}

void Serializer::readTransaction( sb_t& sb,
        const std::string& hash,
        Helper::Database::Database* connection ) {
    Database::Transaction transaction{ db->getTransactionMeta( hash, connection ) };
    initReading( sb );

    trans( transaction, connection );
}

void Serializer::readTransactionBody( sb_t& sb,
        const std::string& hash,
        Helper::Database::Database* connection ) {
    Database::Transaction transaction{ db->getTransactionMeta( hash, connection ) };
    initReading( sb );

    transBody( transaction, connection );
}

void Serializer::readTransactionBody( sb_t& sb,
        unsigned version,
        Helper::Database::Database* connection ) {
    Database::Transaction transaction{ db->getTransactionMeta( version, connection ) };
    initReading( sb );

    transBody( transaction, connection );
}

void Serializer::readTransactionList( sb_t& sb ) {
    initReading( sb );

    s->start_array();
    db->mapTransactions( std::bind( &Serializer::meta, this, _1, nullptr ), nullptr );
    s->close_array();
}

void Serializer::readTransactionMetadata( sb_t& sb,
        const std::string& hash ) {
    initReading( sb );
    meta( db->getTransactionMeta( hash ), nullptr );
}

void Serializer::readTransactionMetadataLatest( sb_t& sb ) {
    initReading( sb );

    s->start_array();
    db->mapTransactionLatest( std::bind( &Serializer::meta, this, _1, nullptr ), nullptr );
    s->close_array();
}

// TODO: make this like "readTransactionList" method
void Serializer::readTransactionMetadataFrom( sb_t& sb,
        const std::vector<std::string>& hashes ) {
    initReading( sb );

    s->start_array();
    db->mapTransactionsFrom( std::bind( &Serializer::meta, this, _1, nullptr ), hashes, nullptr );
    s->close_array();
}

void Serializer::readUser( std::basic_streambuf<char>& sb, const std::string& user ) {
    initReading( sb );
    try {
        //write( sb, db->mapUser( user ) );
        db->mapUser( std::bind( &Serializer::user, this, _1, _2, _3, _4 ), user );
    } catch (...) { // TODO: make more specific
        s->start_object();
        s->close_object();
    }
}

void Serializer::readUsers( std::basic_streambuf<char>& sb ) {
    initReading( sb );
    s->start_array();
    db->mapUsers( std::bind( &Serializer::user, this, _1, _2, _3, _4 ) );
    s->close_array();
}

void Serializer::readUsersFrom( std::basic_streambuf<char>& sb,
        const std::vector<std::string>& userIds ) {
    initReading( sb );
    s->start_array();
    db->mapUsersFrom( std::bind( &Serializer::user, this, _1, _2, _3, _4 ), userIds );
    s->close_array();
}

void Serializer::initReading( sb_t& sb ) {
    this->sb = &sb;
    s.reset( new JSON::Serialize() );
    os.reset( new std::basic_ostream<char>( &sb ) );
    s->set_ostream( os.get() );
}

void Serializer::trans( const Database::Transaction& transaction,
        Helper::Database::Database* connection ) const {
    s->start_object();
        s->put( "id" );
        s->put( transaction.hash.toString() );
        s->put( "signature" );
        s->put( transaction.signature.toString() );
        s->put( "transaction" );
        transBody( transaction, connection );
    s->close_object();
}

void Serializer::transBody( const Database::Transaction& transaction,
        Helper::Database::Database* connection ) const {
    s->start_object();
        s->put( "metadata" );
        s->start_object();
            s->put( "accessDomain" );
            s->put( static_cast<long long>( transaction.accessDomain ) );
            s->put( "timestamp" );
            s->put( transaction.date.toString() );
            s->put( "user" );
            s->put( transaction.creatorHash.toString() );
            s->put( "parents" );
            s->start_object();
            db->mapParents( std::bind( &Serializer::hash, this, _1 ), transaction, connection );
            s->close_object();
            s->put( "version" );
            s->put( "1.0" );
        s->close_object();
        s->put( "objects" );
        s->start_object();
            s->put( "changed" );
            s->start_object();
            db->mapObject( std::bind( &Serializer::changed, this, _1 ), transaction, connection );
            s->close_object();
            s->put( "deleted" );
            s->start_object();
            db->mapObject( std::bind( &Serializer::deleted, this, _1 ), transaction, connection );
            s->close_object();
            s->put( "moved" );
            s->start_object();
            db->mapObject( std::bind( &Serializer::moved, this, _1 ), transaction, connection );
            s->close_object();
            s->put( "new" );
            s->start_object();
            db->mapObject( std::bind( &Serializer::New, this, _1 ), transaction, connection );
            s->close_object();
        s->close_object();
    s->close_object();
}

void Serializer::meta( const Database::Transaction& transaction,
        Helper::Database::Database* connection ) const {
    s->start_object();
        s->put( "id" );
        s->put( transaction.hash.toString() );
        s->put( "signature" );
        s->put( transaction.signature.toString() );
        s->put( "transaction" );
        s->start_object();
            s->put( "metadata" );
            s->start_object();
                s->put( "accessDomain" );
                s->put( static_cast<long long>( transaction.accessDomain ) );
                s->put( "timestamp" );
                s->put( transaction.date.toString() );
                s->put( "user" );
                s->put( transaction.creatorHash.toString() );
                s->put( "parents" );
                s->start_object();
                db->mapParents( std::bind( &Serializer::hash, this, _1 ), transaction, connection );
                s->close_object();
                s->put( "version" );
                s->put( "1.0" );
            s->close_object();
        s->close_object();
    s->close_object();
}

void Serializer::hash( const Database::Transaction& transaction ) const {
    s->put( transaction.hash.toString() );
    s->create_true();
}

// TODO: fix this when more than strings can be read from db
void Serializer::changed( const Database::Object& object ) const {
    if ( Database::ObjectAction::Update != object.action ) {
        return;
    }

    s->put( std::to_string( object.id ) );
    s->start_object();
        s->put( "attributes" );
        s->start_object();
        for( const auto& attr : object.attributes ) {
           putAttribute( s.get(), attr );
        }
        s->close_object();
    s->close_object();
}

void Serializer::deleted( const Database::Object& object ) const {
    if ( Database::ObjectAction::Delete != object.action ) {
        return;
    }

    s->put( std::to_string( object.id ) );
    s->create_true();
}

void Serializer::moved( const Database::Object& object ) const {
    if ( Database::ObjectAction::Move != object.action ) {
        return;
    }

    s->put( std::to_string( object.id ) );
    s->put( static_cast<long long>( object.parent.id ) );
}

void Serializer::New( const Database::Object& object ) const {
    if ( Database::ObjectAction::New != object.action ) {
        return;
    }

    s->put( std::to_string( object.id ) );
    s->start_object();
        s->put( "parent" );
        s->put( static_cast<long long>( object.parent.id ) ); // TODO: AD
        s->put( "attributes" );
        s->start_object();
        for( const auto& attr : object.attributes ) {
           putAttribute( s.get(), attr );
        }
        s->close_object();
    s->close_object();
}

void Serializer::user( const std::string& id,
            const std::string& name,
            const std::string& permission,
            const std::string& publicKey ) const{
    s->start_object();
        s->put( "id" );
        s->put( id );
        s->put( "name" );
        s->put( name );
        s->put( "permission" );
        s->put( permission );
        s->put( "publicKey" );
        s->put( publicKey );
    s->close_object();
}

/*****************************************************************************/


Deserializer::Deserializer( Database* db ) :
        JSON::Event_handler(), db( db ), cb(), d{ new JSON::Deserialize() }, transaction{}, state{}, event_receiver{ JSON::Event_receiver::new_shared() } {
    event_receiver->set_event_handler( this );
    d.reset( new JSON::Deserialize() );
    d->add_event_receiver( event_receiver.get() );
}

void Deserializer::write( sb_t& sb ) {
    if ( !state.empty() && ExchangeState::Error == state.top() ) {
        formatError( "Exchange format parser error state." );
    }

    while( std::char_traits<char>::eof() != sb.in_avail() ) {
        int c{ sb.sbumpc() };
        if ( std::char_traits<char>::eof() == c ) {
            break;
        }
        if ( !d->put( static_cast<char>( c ) ) && !d->put( static_cast<char>( c ) ) ) {
            // TODO: Error
            formatError( "Can not write stream to db." );
        }
    }
}

void Deserializer::write( const char* buf, std::size_t length ) {
    if ( !state.empty() && ExchangeState::Error == state.top() ) {
        throw FormatException();
    }

    for( std::size_t i{0}; i < length; ++i ) {
        if ( !d->put( buf[i] ) && !d->put( buf[i] ) ) {
            // TODO: Error
        }
    }
}

void Deserializer::write( const std::string& buf ) {
    if ( !state.empty() && ExchangeState::Error == state.top() ) {
        throw FormatException();
    }

    for( std::string::const_iterator it{ buf.cbegin() };
            it != buf.cend();
            ++it ) {
        if ( !d->put( *it ) && !d->put( *it ) ) {
            // TODO: Error
        }
    }
}

void Deserializer::clear() {
    rollbackTransaction();
    while( !state.empty() ) {
        state.pop();
    }
    event_receiver->remove_transmitter();
    event_receiver = JSON::Event_receiver::new_shared();
    event_receiver->set_event_handler( this );
    d.reset( new JSON::Deserialize() );
    d->add_event_receiver( event_receiver.get() );
}

std::vector<Database::Meta> Deserializer::exchangeFormatToMeta( std::basic_streambuf<char>& sb ) {
    std::vector<Database::Meta> metaList{};
    auto cb = [&metaList] (const Database::Meta& meta ) -> void {
        metaList.push_back( meta );
    };
    Deserializer d( cb );
    d.write( sb );
    return metaList;
}

Deserializer::Deserializer( map_meta_f cb ) :
        JSON::Event_handler(), db( nullptr ), cb( cb ), d{ new JSON::Deserialize() }, transaction{}, state{}, event_receiver{ JSON::Event_receiver::new_shared() } {
        event_receiver->set_event_handler( this );
        d.reset( new JSON::Deserialize() );
        d->add_event_receiver( event_receiver.get() );
}

void Deserializer::event( JSON::Event e ) {
    parse( e );
}

void Deserializer::parse( E e ) {
    if ( E::Error == e ) {
        formatError( "JSON format error." );
    }

    if ( state.empty() ) {
        state.push( S::Start );
        //startTransaction();
    }

    switch( state.top() ) {
    case S::Start:
        if ( E::Object_start == e ) {
            // Start of whole transaction object
            d->pop();
            state.push( S::TransIdKeyword );
            return;
        } else if ( E::Object_end == e ) {
            // Transaction completed
            // TODO: verify hash value before commit
            d->pop();
            pop();
            commitTransaction();
            return;
        }
        formatError( "Invalid start/end of transaction." );
        break;
    case S::TransIdKeyword:
    case S::TransIdHash:
        parseTransId( e );
        break;
    case S::SigKey:
    case S::SigHash:
        parseSignature( e );
        break;
    case S::TransactionKeyword:
    case S::TransactionObj:
        parseTransaction( e );
        break;
    case S::MetadataKeyword:
    case S::MetadataObj:
        parseMetaData( e );
        break;
    case S::AccessDomainKeyword:
    case S::AccessDomain:
        parseAccessDomain( e );
        break;
    case S::TimestampKeyword:
    case S::Timestamp:
        parseTimestamp( e );
        break;
    case S::UserKeyword:
    case S::UserHash:
        parseUser( e );
        break;
    case S::ParentsKeyword:
    case S::ParentsObj:
    case S::ParentId:
    case S::ParentDummy:
        parseParents( e );
        break;
    case S::VersionKeyword:
    case S::Version:
        parseVersion( e );
        break;
    case S::ObjectsKeyword:
    case S::ObjectsObj:
        parseObjects( e );
        break;
    case S::ChangedKeyword:
    case S::ChangedObjects:
    case S::ChangedId:
    case S::ChangedObj:
        parseChanged( e );
        break;
    case S::DeletedKeyword:
    case S::DeletedObjects:
    case S::DeletedId:
    case S::DeletedDummy:
        parseDeleted( e );
        break;
    case S::MovedKeyword:
    case S::MovedObjects:
    case S::MovedId:
    case S::MovedToParent:
        parseMoved( e );
        break;
    case S::NewKeyword:
    case S::NewObjects:
    case S::NewId:
    case S::NewObj:
    case S::NewParentKeyword:
    case S::NewParentId:
        parseNew( e );
        break;
    //*
    case S::AttributesKeyword:
    case S::AttributesObj:
    case S::AttributeName:
    case S::AttributeValue:
        parseAttributes( e );
        break;
    //*/
    default:
        //formatError();
        std::logic_error( "Missing case in exchange format parser." );
    }
}

void Deserializer::parseTransId( E e ) {
    if ( S::TransIdKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // Start of "id"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "id" ) ) {
            state.top() = S::TransIdHash;
            return;
        }
    } else if ( S::TransIdHash == state.top() ) {
        if ( E::String_start == e ) {
            // Start of transaction hash id
            return;
        } else if ( E::String_end == e ) {
            transactionId = d->get_string();
            state.top() = S::SigKey; // <------- Next state
            return;
        }
    }

    formatError( "Invalid transaction id format." );
}

void Deserializer::parseSignature( E e ) {
    if ( S::SigKey == state.top() ) {
        if ( E::String_start == e ) {
            // Start of "signature"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "signature" ) ) {
            state.top() = S::SigHash;
            return;
        }
    } else if ( S::SigHash == state.top() ) {
        if ( E::String_start == e ) {
            // Start of signature hash
            return;
        } else if ( E::String_end == e ) {
            signature = d->get_string();
            state.top() = S::TransactionKeyword; // <------- Next state
            return;
        }
    }

    formatError( "Invalid transaction signature format." );
}

void Deserializer::parseTransaction( E e ) {
    if ( S::TransactionKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // start of "transactoin"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "transaction" ) ) {
            state.top() = S::TransactionObj;
            return;
        }
    } else if ( S::TransactionObj == state.top() ) {
        if ( E::Object_start == e ) {
            d->pop();
            state.push( S::MetadataKeyword ); // <------- Nested state
            return;
        } else if ( E::Object_end == e ) {
            // Transaction object end
            d->pop();
            pop();
            return;
        }
    }

    formatError( "Invalid transaction object." );
}

void Deserializer::parseMetaData( E e ) {
    if ( S::MetadataKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // start of "metadata"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "metadata" ) ) {
            state.top() = S::MetadataObj;
            return;
        }
    } else if ( S::MetadataObj == state.top() ) {
        if ( E::Object_start == e ) {
            d->pop();
            state.push( S::AccessDomainKeyword ); // <------- Nested state
            return;
        } else if ( E::Object_end == e ) {
            // Meta data done
            d->pop();
            if ( db ) {
                // TODO: verification of the meta data
                //parents = db->getTransactionsFrom( parentIds );
                parents.clear();
                try {
                    for ( const std::string& parent: parentIds ) {
                        parents.push_back( db->getTransactionMeta( parent ) );
                    }
                } catch ( const Mist::Exception& e ) {
                    if ( Error::ErrorCode::NotFound ==
                            static_cast<Error::ErrorCode>( e.getErrorCode() ) ) {
                        LOG( WARNING ) << "Parent not found when writing exchange format to db";
                    } else {
                        LOG( WARNING ) << "Failed to query db about transaction";
                    }
                }
                startTransaction(); // TODO verify this
            }
            state.top() = S::ObjectsKeyword; // <------- Next state
            return;
        }
    }

    formatError( "Invalid metadata format." );
}

void Deserializer::parseAccessDomain( E e ) {
    if ( S::AccessDomainKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // start of "accessDomain"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "accessDomain" ) ) {
            state.top() = S::AccessDomain;
            return;
        }
    } else if ( S::AccessDomain == state.top() ) {
        if ( E::Integer == e ) {
            accessDomain = d->get_integer();
            state.top() = S::TimestampKeyword; // <------- Next state
            return;
        }
    }

    formatError( "Invalid access domain format." );
}

void Deserializer::parseTimestamp( E e ) {
    if ( S::TimestampKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // start of "timestamp"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "timestamp" ) ) {
            state.top() = S::Timestamp;
            return;
        }
    } else if ( S::Timestamp == state.top() ) {
        if ( E::String_start == e ) {
            // start of date string
            return;
        } else if ( E::String_end == e ) {
            // TODO: verify that the timestamp follows the selected standard
            timestamp = d->get_string();
            state.top() = S::UserKeyword; // <------- Next state
            return;
        }
    }

    formatError( "Invalid timestamp format." );
}

void Deserializer::parseUser( E e ) {
    if ( S::UserKeyword == state.top() ) {
        if ( E::String_start == e  ) {
            // start of "user"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "user" ) ) {
            state.top() = S::UserHash;
            return;
        }
    } else if ( S::UserHash == state.top() ) {
        if ( E::String_start == e ) {
            // start of user hash
            return;
        } else if ( E::String_end == e ) {
            user = d->get_string();
            state.top() = S::ParentsKeyword; // <------- Next state
            return;
        }
    }

    formatError( "Invalid user format." );
}

void Deserializer::parseParents( E e ) {
    if ( S::ParentsKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // start of "parents"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "parents" ) ) {
            state.top() = S::ParentsObj;
            return;
        }
    } else if ( S::ParentsObj == state.top() ) {
        if ( E::Object_start == e ) {
            d->pop();
            state.push( S::ParentId );
            parentIds.clear();
            return;
        } else if ( E::Object_end == e ) {
            // No more parents
            d->pop();
            state.top() = S::VersionKeyword; // <------- Next state
            return;
        }
    } else if ( S::ParentId == state.top() ) {
        if ( E::String_start == e ) {
            // start of parents hash
            return;
        } else if ( E::String_end == e ) {
            parentIds.push_back( d->get_string() );
            state.top() = S::ParentDummy;
            return;
        } else if ( E::Object_end == e ) {
            // No more parent hashes
            d->pop();
            pop();
            return parseParents( e );
        }
    } else if ( S::ParentDummy == state.top() ) {
        if ( E::True == e) {
            d->pop();
            state.top() = S::ParentId;
            return;
        }
    }

    formatError( "Invalid parent format." );
}

void Deserializer::parseVersion( E e ) {
    if ( S::VersionKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // start of "version"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "version" ) ) {
            state.top() = S::Version;
            return;
        }
    } else if ( S::Version == state.top() ) {
        if ( E::String_start == e ) {
            // Start of version string
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "1.0" ) ) {
            // Correct version
            pop(); // <------- Pop state
            return;
        }
    }

    formatError( "Invalid version." );
}

void Deserializer::parseObjects( E e ) {
    if ( S::ObjectsKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // Start of "objects"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "objects" ) ) {
            state.top() = S::ObjectsObj;
            return;
        } else if ( E::Object_end == e ) {
            // No object data, only metadata
            d->pop();
            state.pop();
        }
    } else if ( S::ObjectsObj == state.top() ) {
        if ( E::Object_start == e ) {
            // Start of "objects" object
            d->pop();
            state.push( S::ChangedKeyword ); // <------- Next state
            return;
        } else if ( E::Object_end == e ) {
            // No more objects
            d->pop();
            pop(); // <------- Pop state
            return;
        }
    }

    formatError( "Invalid objects format." );
}

void Deserializer::parseChanged( E e ) {
    if ( S::ChangedKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // Start of "changed"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "changed" ) ) {
            state.top() = S::ChangedObjects;
            return;
        }
    } else if ( S::ChangedObjects == state.top() ) {
        if ( E::Object_start == e ) {
            d->pop();
            state.push( S::ChangedId );
            return;
        } else if ( E::Object_end == e ) {
            d->pop();
            state.top() = S::DeletedKeyword; // <------- Next state
            return;
        }
    } else if ( S::ChangedId == state.top() ) {
        if ( E::String_start == e) {
            // Start of object id
            return;
        } else if ( E::String_end == e ) {
            objId = d->get_string();
            state.top() = S::ChangedObj;
            return;
        } else if ( E::Object_end == e ) {
            // No more changed objects
            pop();
            return parseChanged( e );
        }
    } else if ( S::ChangedObj == state.top() ) {
        if ( E::Object_start == e ) {
            d->pop();
            state.push( S::AttributesKeyword );
            return;
        } else if ( E::Object_end == e ) {
            d->pop();
            changeObject();
            state.top() = S::ChangedId;
            return;
        }
    }

    formatError( "Invalid objects changed format." );
}

void Deserializer::parseDeleted( E e ) {
   if ( S::DeletedKeyword == state.top() ) {
       if ( E::String_start == e ) {
           // Start of "deleted"
           return;
       } else if ( E::String_end == e &&
               0 == d->get_string().compare( "deleted" ) ) {
           state.top() = S::DeletedObjects;
           return;
       }
   } else if ( S::DeletedObjects == state.top() ) {
       if ( E::Object_start == e ) {
           d->pop();
           state.push( S::DeletedId );
           return;
       } else if ( E::Object_end == e ) {
           // No more deleted objects
           d->pop();
           state.top() = S::MovedKeyword; // <------- Next state
           return;
       }
   } else if ( S::DeletedId == state.top() ) {
       if ( E::String_start == e ) {
           // Start of object id
           return;
       } else if ( E::String_end == e ) {
           objId = d->get_string();
           state.top() = S::DeletedDummy;
           return;
       } else if ( E::Object_end == e ) {
           // No more deleted objects
           pop();
           return parseDeleted( e );
       }
   } else if ( S::DeletedDummy == state.top() ) {
       if ( E::True == e ) {
           // Got "correct" dummy i.e. 'true'
           d->pop();
           deleteObject();
           state.top() = S::DeletedId;
           return;
       }
   }

   formatError( "Invalid objects deleted format." );
}

void Deserializer::parseMoved( E e ) {
    if ( S::MovedKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // Start of "moved"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "moved" ) ) {
            state.top() = S::MovedObjects;
            return;
        }
    } else if ( S::MovedObjects == state.top() ) {
        if ( E::Object_start == e ) {
            d->pop();
            state.push( S::MovedId );
            return;
        } else if ( E::Object_end == e ) {
            // No more moved objects
            d->pop();
            state.top() = S::NewKeyword; // <------- Next state
            return;
        }
    } else if ( S::MovedId == state.top() ) {
        if ( E::String_start == e ) {
            // Start of object id
            return;
        } else if ( E::String_end == e ) {
            objId = d->get_string();
            // default to this ad. TODO: verify this
            objParentAd = accessDomain;
            state.top() = S::MovedToParent;
            return;
        } else if ( E::Object_end == e ) {
            // No more moved objects
            pop();
            return parseMoved( e );
        }
    } else if ( S::MovedToParent == state.top() ) {
        if ( E::Integer == e ) {
            objParent = d->get_integer();
            moveObject();
            state.top() = S::MovedId;
            return;
        }
        /*
        if ( E::String_start == e ) {
            // Start of parent id
            return;
        } else if ( E::String_end == e ) {
            parent = d->get_string();
            moveObject();
            state.top() = S::MovedId;
            return;
        } // TODO: else if E::Integer => Access domain
        //*/
    }

    formatError( "Invalid objects moved format." );
}

void Deserializer::parseNew( E e ) {
    if ( S::NewKeyword ==  state.top() ) {
        if ( E::String_start == e ) {
            // Start of "new"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "new" ) ) {
            state.top() = S::NewObjects;
            return;
        }
    } else if ( S::NewObjects == state.top() ) {
        if ( E::Object_start == e ) {
            d->pop();
            state.push( S::NewId );
            return;
        } else if ( E::Object_end == e ) {
            // No more new objects
            d->pop();
            pop(); // <------- Pop state
            return;
        }
    } else if ( S::NewId == state.top() ) {
        if ( E::String_start == e ) {
            // Start of object id
            return;
        } else if ( E::String_end == e ) {
            objId =  d->get_string();
            state.top() = S::NewObj;
            return;
        } else if ( E::Object_end == e ) {
            // No more new objects
            pop();
            return parseNew( e );
        }
    } else if ( S::NewObj == state.top() ) {
        if ( E::Object_start == e ) {
            d->pop();
            // default to this ad. TODO: verify this
            objParentAd = accessDomain;
            state.push( S::NewParentKeyword );
            return;
        } else if ( E::Object_end == e ) {
            // End of object, on to the next one
            d->pop();
            newObject();
            state.top() = S::NewId;
            return;
        }
    } else if ( S::NewParentKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // Start of "parent"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "parent" ) ) {
            state.top() = S::NewParentId;
            return;
        }
    } else if ( S::NewParentId == state.top() ) {
        if ( E::Integer == e ) {
            objParent = d->get_integer();
            state.top() = S::AttributesKeyword; // <------- Next state
            return;
        }
        /*
        if ( E::String_start == e ) {
            // Start of parent id
            return;
        } else if ( E::String_end == e ) {
            parent = d->get_string();
            state.top() = S::AttributesKeyword; // <------- Next state
            return;
        } // TODO: else if E::Integer => Access domain
        //*/
    }

    formatError( "Invalid objects new format." );
}

void Deserializer::parseAttributes( E e ) {
    if ( S::AttributesKeyword == state.top() ) {
        if ( E::String_start == e ) {
            // Start of "attributes"
            return;
        } else if ( E::String_end == e &&
                0 == d->get_string().compare( "attributes" ) ) {
            state.top() = S::AttributesObj;
            return;
        }
    } else if ( S::AttributesObj == state.top() ) {
        if ( E::Object_start == e ) {
            d->pop();
            attributes.clear();
            state.push( S::AttributeName );
            return;
        } else if ( E::Object_end == e ) {
            // No more attributes
            d->pop();
            pop();  // <------- Pop state
            return;
        }
    } else if ( S::AttributeName ==  state.top() ) {
        if ( E::String_start == e ) {
            // Start of attribute name
            return;
        } else if ( E::String_end == e ) {
            attrName = d->get_string();
            state.top() = S::AttributeValue;
            return;
        } else if ( E::Object_end == e ) {
            // No more attributes
            pop();
            return parseAttributes( e );
        }
    } else if ( S::AttributeValue ==  state.top() ) {
        // TODO: Verify this behavior
        switch( e ) {
        case E::Null:
            // TODO: not implemented in Database::Value
            // change this when implemented.
            value = false;
            break;
        case E::Integer:
            value = static_cast<double>( d->get_integer() );
            break;
        case E::Float:
            value = static_cast<double>( d->get_double() );
            break;
        case E::True:
        case E::False:
            value = d->get_bool();
            break;
        case E::String_start:
            // Wait for complete string
            return;
        case E::String_end:
            value = d->get_string();
            break;
        default:
            // Got unexpected value
            //formatError();
            throw std::logic_error( "Missing case in attribute parser." );
            return;
        }
        attributes.emplace( attrName, value );
        state.top() = S::AttributeName;
        return;
    }

    formatError( "Invalid attributes format." );
}

void Deserializer::formatError( std::string err ) {
    rollbackTransaction();
    //state.push( S::Error );
    clear();
    throw FormatException( "Invalid Mist 1.0 format: " + err );
}

void Deserializer::startTransaction() {
    if ( !db )
        return;
    transaction.reset();
    transaction = std::move( db->beginRemoteTransaction(
            static_cast<Database::AccessDomain>( accessDomain ),
            parents,
            Helper::Date( timestamp ),
            CryptoHelper::SHA3( user ),
            CryptoHelper::SHA3( transactionId ),
            CryptoHelper::Signature( signature ) ) );
    transaction->init(); // TODO: this should not be needed, make it RAII instead?
}

void Deserializer::commitTransaction() {
    if ( cb ) {
        std::vector<CryptoHelper::SHA3> parentsSHA3{};
        for ( const std::string& str : parentIds ) {
            parentsSHA3.push_back( CryptoHelper::SHA3::fromString( str ) );
        }
        cb( Database::Meta{
            static_cast<Database::AccessDomain>( accessDomain ),
            parentsSHA3,
            Helper::Date( timestamp ),
            CryptoHelper::SHA3::fromString( user ),
            CryptoHelper::SHA3::fromString( transactionId ),
            CryptoHelper::Signature::fromString( signature )
        } );
    }
    if ( db && transaction ) {
        transaction->commit();
    }
}

void Deserializer::rollbackTransaction() {
    transaction.reset();
}

void Deserializer::changeObject() {
    if ( !db )
        return;
    // TODO: correct conversion for id
    transaction->updateObject( std::stoll( objId ), attributes );
}

void Deserializer::deleteObject() {
    if ( !db )
        return;
    // TODO: correct conversion for id
    transaction->deleteObject( std::stoll( objId ) );
}

void Deserializer::moveObject() {
    if ( !db )
        return;
    // TODO: correct conversion for id
    transaction->moveObject(
            std::stoll( objId ),
            Database::ObjectRef{
                    static_cast<Database::AccessDomain>( objParentAd ),
                    static_cast<unsigned long>( objParent )
            } );
}

void Deserializer::newObject() {
    if ( !db )
        return;
    // TODO: correct conversion for id
    transaction->newObject(
            std::stoll( objId ),
            Database::ObjectRef{
                    static_cast<Database::AccessDomain>( objParentAd ),
                    static_cast<unsigned long>( objParent )
            },
            attributes );
}

void Deserializer::pop() {
    if ( state.empty() ) {
        rollbackTransaction();
        throw FormatException( "Invalid state reached." );
    } else {
        state.pop();
    }
}

} /* namespace Mist */
