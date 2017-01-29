/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "Exception.h"
#include "RemoteTransaction.h"

namespace Mist {

RemoteTransaction::RemoteTransaction(
        Database *db,
        Database::AccessDomain accessDomain,
        unsigned version,
        std::vector<Database::Transaction> parents,
        Helper::Date timestamp,
        CryptoHelper::SHA3 userHash,
        CryptoHelper::SHA3 hash,
        CryptoHelper::Signature signature) :
                db( db ),
                connection{ nullptr == db ? nullptr : std::move( db->getIsolatedDbConnection() ) },
                transaction( new Helper::Database::Transaction( *connection.get() ) ),
                accessDomain( accessDomain ),
                version( version ),
                parents( parents ),
                timestamp( timestamp ),
                userHash( userHash ),
                hash( hash ),
                signature( signature ),
                last( true ),
                valid( true ) {
    if ( db == nullptr ) {
        // TODO: handle this.
        LOG( WARNING ) << "db == nullptr !";
        throw std::runtime_error( "Nullpointer given to RemoteTransaction constructor" );
    }
}

RemoteTransaction::~RemoteTransaction() {
    rollback();
}

void RemoteTransaction::init() {
    // Reject transactions from the future
    std::string now = Helper::Date::now();
    if ( timestamp.toString() >= now ) {
        LOG( WARNING ) << "Rejecting transaction from the future: " <<
                "local time: " << now << " transaction time: " << timestamp.toString();
        valid = false;
        throw std::runtime_error( "Rejecting transaction from the future" );
    }

    // Insert transaction
    Database::Statement insertTransaction( *connection.get(),
            "INSERT INTO 'Transaction' (accessDomain, version, timestamp, userHash, hash, signature) "
            "VALUES (?, ?, ?, ?, ?, ?) " );
    insertTransaction <<
            static_cast<int>( accessDomain ) <<
            version <<
            timestamp.toString() <<
            userHash.toString() <<
            hash.toString() <<
            signature.toString();
    if ( 0 == insertTransaction.exec() ) {
        LOG( WARNING ) << "Could not insert transaction";
        throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
    }

    // Read back the meta info from the db
    Database::Transaction meta{ db->getTransactionMeta( version, connection.get() ) };
    // Reorder the transaction to its correct place
    version = db->reorderTransaction( meta, connection.get() );

    // Check that we have all parents
    if ( !parents.empty() ) {
        // TODO: This block is already required to be done by the constructor,
        // as it takes vector<transaction> parents as an argument.
        // Should it be required in the constructor or done here?
        std::string q = "SELECT accessDomain, version, timestamp, userHash, hash, signature "
                "FROM 'Transaction' "
                "WHERE version < " + std::to_string( version ) + " AND hash IN ( ";
        for ( auto parent : parents ) {
            q += " '" + parent.hash.toString() + "',";
        }
        q.pop_back();
        q += ")";

        std::vector<Database::Transaction> rows {};
        Database::Statement query( *connection.get(), q );
        while ( query.executeStep() ) {
            rows.push_back( Database::statementRowToTransaction( query ) );
        }

        if ( rows.size() != parents.size() ) {
            valid = false;
            LOG ( WARNING ) << "Parent transaction not found.";
            throw Mist::Exception( Mist::Error::ErrorCode::ParentTransactionNotFound );
        }
        parents = rows; // TODO: Currently it's the same as before? Meaning the whole block is a costly no-op.
        // Further more it is never again used and only .empty is called at the end
    }

    // Insert transaction parents
    Database::Statement insertTransactionParent( *connection.get(),
            "INSERT INTO transactionParent (accessDomain, version, parentAccessDomain, parentVersion) "
            "VALUES (?, ?, ?, ?)");
    for ( auto parent : parents ) {
        insertTransactionParent << static_cast<unsigned int>( accessDomain ) <<
                version <<
                static_cast<int>( parent.accessDomain ) <<
                parent.version;
        if ( 0 == insertTransactionParent.exec() ) {
            LOG( WARNING ) << "Could not insert transaction parent";
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
        insertTransactionParent.clearBindings();
        insertTransactionParent.reset();
    }

    Database::Statement queryMaxVersion( *connection.get(),
            "SELECT MAX(version) AS max FROM 'Transaction'" );
    if ( !queryMaxVersion.executeStep() ) {
        // Most likely empty
        // TODO: could be some other error
        valid = false;
        LOG ( WARNING ) << "Could not get a new local version";
        throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
    }
    unsigned maxVersion = queryMaxVersion.getColumn( "max" ).getUInt();

    this->last = this->version >= maxVersion;

    parents.clear();
}

/**
 * Minimal error checking. Most error checking is done in the second pass, when the whole transaction
 * is stored in the database.
 */
void RemoteTransaction::newObject( unsigned long id, const Database::ObjectRef& parent, const std::map<std::string, Database::Value>& attributes ) {
    LOG( DBUG ) << "New object: " << id;
    if ( !valid ) {
        LOG( WARNING ) << "Invalid transaction";
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidTransaction );
    }

    // Check that the same object id is NOT used multiple times in the same transaction.
    Database::Statement queryId( *connection.get(),
            "SELECT id FROM Object "
            "WHERE accessDomain=? AND id=? AND version=?" );
    queryId <<
            static_cast<unsigned>( accessDomain ) <<
            static_cast<long long>( id ) <<
            version;
    if ( queryId.executeStep() ) {
        valid = false;
        LOG( WARNING ) << "Object collision: " << queryId.getColumn( "id" ).getInt64();
        throw Mist::Exception( Mist::Error::ErrorCode::ObjectCollisionInTransaction );
    }

    Database::ObjectMeta object{
        accessDomain,
        id,
        version,
        parent,
        olderVersionOfObjectExists( id ) ? Database::ObjectStatus::InvalidNew : Database::ObjectStatus::Current,
        Database::ObjectAction::New
    };

    bool validParent{ Database::ObjectStatus::Current == getParentStatus( object ) };
    if ( !validParent && Database::ObjectStatus::Current == object.status ) {
        object.status = Database::ObjectStatus::InvalidParent;
    }

    // Insert the object
    Database::Statement insertObject( *connection.get(),
            "INSERT INTO Object (accessDomain, id, version, status, parent, parentAccessDomain, transactionAction) "
            "VALUES (?, ?, ?, ?, ?, ?, ?)" );
    insertObject <<
            static_cast<unsigned>( accessDomain ) <<
            static_cast<long long>( id ) <<
            version <<
            static_cast<unsigned>( object.status ) <<
            static_cast<long long>( parent.id ) <<
            static_cast<unsigned>( parent.accessDomain ) <<
            static_cast<unsigned>( Database::ObjectAction::New );

    if ( insertObject.exec() == 0 ) {
        // TODO: query failed, handle it.
        valid = false;
        LOG( WARNING ) << "Could not insert object";
        throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
    }

    // Insert the objects attributes
    Database::Statement insertIntoAttribute( *connection.get(),
            "INSERT INTO Attribute (accessDomain, id, version, name, type, value) "
            "VALUES (?, ?, ?, ?, ?, ?)" );
    for ( auto const & kv : attributes ) {
        insertIntoAttribute <<
                static_cast<int>( accessDomain ) <<
                static_cast<long long>( id ) <<
                version <<
                kv.first <<
                static_cast<int>( kv.second.t );
        using T = Database::Value::Type;
        switch ( kv.second.t ){
        case T::Typeless:
            break;
        case T::Null:
            break;
        case T::Boolean:
            insertIntoAttribute << kv.second.b;
            break;
        case T::Number:
            insertIntoAttribute << kv.second.n;
            break;
        case T::String:
            insertIntoAttribute << kv.second.v;
            break;
        case T::Json:
            insertIntoAttribute << kv.second.v;
            break;
        default:
            LOG( WARNING ) << "Attribute statement does not contain correct type.";
            throw std::runtime_error( "Attribute statement does not contain correct type." );
        }

        if ( insertIntoAttribute.exec() ) {
            insertIntoAttribute.reset();
            insertIntoAttribute.clearBindings();
        } else {
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
    }

    affectedObjects.insert( parent );
    affectedObjects.insert( { accessDomain, id } );
}

/*
 * Minimal error checking. Most error checking is done in the second pass, when the whole transaction
 * is stored in the database.
 */
void RemoteTransaction::moveObject( unsigned long id, Database::ObjectRef newParent ) {
    LOG( DBUG ) << "Move object: " << id;
    if ( !valid ) {
        valid = false;
        LOG( WARNING ) << "Invalid transaction";
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidTransaction );
    }

    /* Not implemented yet.
    Database::ObjectRef obj { accessDomain, id };
    auto reObj = renumber.find( obj );
    if ( reObj != renumber.end() ) {
        id = reObj->second;
    }
    //*/

    Database::Statement queryId( *connection.get(),
            "SELECT id FROM Object WHERE accessDomain=? AND id=? AND version=?" );
    queryId.bind( 1, (unsigned) accessDomain );
    queryId.bind( 2, (long long) id );
    queryId.bind( 3, version );
    if ( queryId.executeStep() ) {
        valid = false;
        LOG( WARNING ) << "Object collision";
        throw Mist::Exception( Mist::Error::ErrorCode::ObjectCollisionInTransaction );
    }

    Database::Statement getOldParent( *connection.get(),
            "SELECT accessDomain, id, MAX(version) "
            "FROM Object "
            "WHERE id=( "
                "SELECT parent "
                "FROM Object "
                "WHERE id=? "
                "ORDER BY version DESC "
            ") " );
    getOldParent << static_cast<long long>( id );
    Database::ObjectRef oldParent{ accessDomain, 0 };
    if ( getOldParent.executeStep() ) {
        oldParent.accessDomain = static_cast<Database::AccessDomain>( getOldParent.getColumn( "accessDomain" ).getUInt() );
        oldParent.id = static_cast<unsigned long>( getOldParent.getColumn( "id" ).getInt64() );
    }

    Database::Statement queryInsertIntoObject( *connection.get(),
            "INSERT INTO Object (accessDomain, id, version, status, parent, parentAccessDomain, transactionAction) "
            "VALUES (?, ?, ?, ?, ?, ?, ?)" );
    queryInsertIntoObject.bind( 1, (unsigned) accessDomain );
    queryInsertIntoObject.bind( 2, (long long) id );
    queryInsertIntoObject.bind( 3, version );
    //queryInsertIntoObject.bind( 4, 0 ); // TODO: Do we really want to insert 0/null here? trying to read this value into Database::ObjectStatus later on will not work.
    queryInsertIntoObject.bind( 4, static_cast<int>( Database::ObjectStatus::Current ) );
    queryInsertIntoObject.bind( 5, (long long) newParent.id );
    queryInsertIntoObject.bind( 6, (unsigned) newParent.accessDomain );
    queryInsertIntoObject.bind( 7, (unsigned) Database::ObjectAction::Move );
    if ( queryInsertIntoObject.exec() == 0 ) {
        // TODO: query failed, handle it.
        valid = false;
        LOG( WARNING ) << "Could not insert object";
        throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
    }

    Database::Statement queryObject( *connection.get(),
            "SELECT id, parent, parentAccessDomain, version, status, transactionAction "
            "FROM Object "
            "WHERE accessDomain=? AND id=? AND version=(SELECT MAX(version) "
            "FROM Object WHERE accessDomain=? AND id=? AND status <= ? AND version < ?)" );
    queryObject.bind( 1, (unsigned) accessDomain );
    queryObject.bind( 2, (long long) id );
    queryObject.bind( 3, (unsigned) accessDomain );
    queryObject.bind( 4, (long long) id );
    queryObject.bind( 5, (unsigned) Database::ObjectStatus::OldDeletedParent );
    queryObject.bind( 6, version );
    if ( queryObject.executeStep() != 0 ) {
        Database::Statement queryAttribute( *connection.get(),
                // TODO: wrong query?
                "INSERT INTO Attribute (accessDomain, id, version, name, type, value) "
                "SELECT accessDomain, id, ?, name, type, value "
                "FROM Attribute "
                "WHERE accessDomain=? AND id=? AND version=?" );
        queryAttribute.bind( 1, version );
        queryAttribute.bind( 2, (unsigned) accessDomain );
        queryAttribute.bind( 3, static_cast<long long>( id ) );
        queryAttribute.bind( 4, queryObject.getColumn( "version" ).getUInt() );
        queryAttribute.exec();
        /*
        if ( queryAttribute.exec() == 0 ) {
            // TODO: query failed, handle it.
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
        //*/
    } else {
        // TODO: Did NOT get any object from the database, should this be handled?
    }

    // TODO: call object changed on this->db, the same way as in the local transactions?
    affectedObjects.insert( oldParent );
    affectedObjects.insert( newParent );
    affectedObjects.insert( { accessDomain, id } );
}

/*
 * Minimal error checking. Most error checking is done in the second pass, when the whole transaction
 * is stored in the database.
 */
void RemoteTransaction::updateObject( unsigned long id, std::map<std::string, Database::Value> attributes ) {
    LOG( DBUG ) << "Update object: " << id;
    if ( !valid ) {
        valid = false;
        LOG( WARNING ) << "Invalid transaction";
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidTransaction );
    }

    /* Not implemented yet.
    Database::ObjectRef obj { accessDomain, id };
    auto const & reObj = renumber.find( obj );
    if ( reObj != renumber.end() ) {
        id = reObj->second;
    }
    //*/

    Database::Statement getParent( *connection.get(),
            "SELECT accessDomain, id, MAX(version) "
            "FROM Object "
            "WHERE id=( "
                "SELECT parent "
                "FROM Object "
                "WHERE id=? "
                "ORDER BY version DESC "
            ") " );
    getParent << static_cast<long long>( id );
    Database::ObjectRef parent{ accessDomain, 0 };
    if ( getParent.executeStep() ) {
        parent.accessDomain = static_cast<Database::AccessDomain>( getParent.getColumn( "accessDomain" ).getUInt() );
        parent.id = static_cast<unsigned long>( getParent.getColumn( "id" ).getInt64() );
    }

    Database::Statement queryId( *connection.get(),
            "SELECT id, transactionAction "
            "FROM Object "
            "WHERE accessDomain=? AND id=? AND version=?" );
    queryId.bind( 1, (unsigned) accessDomain );
    queryId.bind( 2, (long long) id );
    queryId.bind( 3, version );
    if ( queryId.executeStep() ) {
        if ( ( (Database::ObjectAction) queryId.getColumn( "transactionAction" ).getUInt() ) == Database::ObjectAction::Move ) {
            Database::Statement queryUpdateObject( *connection.get(),
                    "UPDATE Object SET transactionAction=? "
                    "WHERE accessDomain=? AND id=? AND version=?" );
            queryUpdateObject <<
                    static_cast<int>( Database::ObjectAction::MoveUpdate ) <<
                    static_cast<unsigned>( accessDomain ) <<
                    static_cast<long long>( id ) <<
                    version;
            if ( queryUpdateObject.exec() == 0 ) {
                // TODO: query failed, handle it.
                valid = false;
                LOG( WARNING ) << "Could not update object";
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }

            // TODO: Do we really want to delete something here?
            Database::Statement quertDeleteAttribute( *connection.get(),
                    "DELETE FROM Attribute WHERE accessDomain=? AND id=? AND version=?" );
            quertDeleteAttribute.bind( 1, (unsigned) accessDomain );
            quertDeleteAttribute.bind( 2, (long long) id );
            quertDeleteAttribute.bind( 3, version );
            if ( quertDeleteAttribute.exec() == 0 ) {
                // TODO: query failed, handle it.
                valid = false;
                LOG( WARNING ) << "Could not delete attribute";
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }
        } else {
            valid = false;
            LOG( WARNING ) << "Object collision";
            throw Mist::Exception( Mist::Error::ErrorCode::ObjectCollisionInTransaction );
        }
    } else {
        Database::Statement queryObject( *connection.get(),
                "SELECT id, parent, parentAccessDomain, version, status, transactionAction "
                "FROM Object "
                "WHERE accessDomain=? AND id=? AND version=(SELECT MAX(version) "
                "FROM Object WHERE accessDomain=? AND id=? AND status <= ? AND version < ? ) ");
        queryObject.bind( 1, (unsigned) accessDomain );
        queryObject.bind( 2, (long long) id );
        queryObject.bind( 3, (unsigned) accessDomain );
        queryObject.bind( 4, (long long) id );
        queryObject.bind( 5, (unsigned) Database::ObjectStatus::OldDeletedParent );
        queryObject.bind( 6, version );
        if ( !queryObject.executeStep() ) {
            // TODO: Not found??? verify this.
            Database::Statement queryInsertIntoObject( *connection.get(),
                    "INSERT INTO Object (accessDomain, id, parent, parentAccessDomain, version, status, transactionAction) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?)" );
            queryInsertIntoObject.bind( 1, (unsigned) accessDomain );
            queryInsertIntoObject.bind( 2, (long long) id );
            //queryInsertIntoObject.bind( 3, 0 );
            //queryInsertIntoObject.bind( 4, 0 );
            queryInsertIntoObject.bind( 5, version );
            queryInsertIntoObject.bind( 6, static_cast<unsigned>( Database::ObjectStatus::OldDeletedParent ) );
            // TODO: missing from original .ts file, unsure of what it should be.
            //throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError ); // TODO: remove this after the line above has been fixed.
            queryInsertIntoObject.bind( 7, (unsigned) Database::ObjectAction::Update );
            if ( queryInsertIntoObject.exec() == 0 ) {
                // TODO: query failed, handle it.
                valid = false;
                LOG( WARNING ) << "Could not insert object";
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }
        } else {
            Database::Statement queryInsertIntoObject( *connection.get(),
                    "INSERT INTO Object (accessDomain, id, parent, parentAccessDomain, version, status, transactionAction) "
                    "SELECT accessDomain, id, parent, parentAccessDomain, ?, NULL, ? "
                    "FROM Object "
                    "WHERE accessDomain=? AND id=? AND version=?" );
            queryInsertIntoObject.bind( 1, version );
            queryInsertIntoObject.bind( 2, (unsigned) Database::ObjectAction::Update );
            queryInsertIntoObject.bind( 3, (unsigned) accessDomain );
            queryInsertIntoObject.bind( 4, (long long) id );
            queryInsertIntoObject.bind( 5, queryObject.getColumn( "version" ).getUInt() );
            if ( queryInsertIntoObject.exec() == 0 ) {
                // TODO: query failed, handle it.
                valid = false;
                LOG( WARNING ) << "Could not insert object";
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }

        }
    }

    Database::Statement insertIntoAttribute( *connection.get(),
            "INSERT INTO Attribute (accessDomain, id, version, name, type, value) "
            "VALUES (?, ?, ?, ?, ?, ?)" );
    for ( auto const & kv : attributes ) {
        insertIntoAttribute <<
                static_cast<int>( accessDomain ) <<
                static_cast<long long>( id ) <<
                version <<
                kv.first <<
                static_cast<int>( kv.second.t );
        using T = Database::Value::Type;
        switch ( kv.second.t ){
        case T::Typeless:
            break;
        case T::Null:
            break;
        case T::Boolean:
            insertIntoAttribute << kv.second.b;
            break;
        case T::Number:
            insertIntoAttribute << kv.second.n;
            break;
        case T::String:
            insertIntoAttribute << kv.second.v;
            break;
        case T::Json:
            insertIntoAttribute << kv.second.v;
            break;
        default:
            LOG( WARNING ) << "Attribute statement does not contain correct type.";
            throw std::runtime_error( "Attribute statement does not contain correct type." );
        }

        if ( insertIntoAttribute.exec() ) {
            insertIntoAttribute.reset();
            insertIntoAttribute.clearBindings();
        } else {
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
    }

    affectedObjects.insert( parent );
    affectedObjects.insert( { accessDomain, id } );
}

void RemoteTransaction::deleteObject( unsigned long id ) {
    LOG( DBUG ) << "Delete object: " << id;
    if ( !valid ) {
        LOG( WARNING ) << "Invalid transaction";
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidTransaction );
    }

    Database::ObjectRef obj { accessDomain, id };
    auto const & reObj = renumber.find( obj );
    if ( reObj != renumber.end() ) {
        // TODO: Not sure what this does. Can someone comment this?
        id = reObj->second;
    }

    Database::Statement getParent( *connection.get(),
            "SELECT accessDomain, id "
            "FROM Object "
            "WHERE id=( "
                "SELECT parent "
                "FROM Object "
                "WHERE id=? AND version <= ? "
            ") " );
    getParent <<
            static_cast<long long>( id ) <<
            version;
    Database::ObjectRef parent{ accessDomain, 0 };
    if ( getParent.executeStep() ) {
        parent.accessDomain = static_cast<Database::AccessDomain>( getParent.getColumn( "accessDomain" ).getUInt() );
        parent.id = static_cast<unsigned long>( getParent.getColumn( "id" ).getInt64() );
    }

    Database::Statement queryId( *connection.get(),
            "SELECT id, transactionAction "
            "FROM Object WHERE accessDomain=? AND id=? AND version=?" );
    queryId.bind( 1, (unsigned) accessDomain );
    queryId.bind( 2, (long long) id );
    queryId.bind( 3, version );
    if ( queryId.executeStep() ) {
        valid = false;
        LOG( WARNING ) << "Object collision";
        throw Mist::Exception( Mist::Error::ErrorCode::ObjectCollisionInTransaction );
    }

    Database::Statement queryObject( *connection.get(),
            "SELECT id, parent, parentAccessDomain, version, status, transactionAction "
            "FROM Object "
            "WHERE accessDomain=? AND id=? AND version=(SELECT version "
            "FROM Object WHERE accessDomain=? AND id=? AND status <= ? AND version < ?)" );
    queryObject.bind( 1, (unsigned) accessDomain );
    queryObject.bind( 2, (long long) id );
    queryObject.bind( 3, (unsigned) accessDomain );
    queryObject.bind( 4, (long long) id );
    queryObject.bind( 5, (unsigned) Database::ObjectStatus::OldDeletedParent );
    queryObject.bind( 6, version );
    if ( queryObject.executeStep() == 0 ) {
        // TODO: Not found???
        Database::Statement queryInsertIntoObject( *connection.get(),
                "INSERT INTO Object (accessDomain, id, parent, parentAccessDomain, version, status, transactionAction) "
                "VALUES (?, ?, ?, ?, ?, ?, ?)" );
        queryInsertIntoObject.bind( 1, (unsigned) accessDomain );
        queryInsertIntoObject.bind( 2, (long long) id );
        queryInsertIntoObject.bind( 3, 0 );
        queryInsertIntoObject.bind( 4, 0 );
        queryInsertIntoObject.bind( 5, version );
        queryInsertIntoObject.bind( 6, static_cast<unsigned>( Database::ObjectStatus::OldDeletedParent ) );
        // TODO: missing from original .ts file, unsure of what it should be.
        //throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError ); // TODO: remove this after the line above has been fixed.
        queryInsertIntoObject.bind( 7, (unsigned) Database::ObjectAction::Delete );
        if ( queryInsertIntoObject.exec() == 0 ) {
            // TODO: query failed, handle it.
            valid = false;
            LOG( WARNING ) << "Could not insert object";
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
    } else {
        Database::Statement queryInsertIntoObject( *connection.get(),
                // TODO: wrong query?
                "INSERT INTO Object (accessDomain, id, parent, parentAccessDomain, version, status, transactionAction) "
                "SELECT accessDomain, id, parent, parentAccessDomain, ?, NULL, ? "
                "FROM Object "
                "WHERE accessDomain=? AND id=? AND version=?" );
        queryInsertIntoObject.bind( 1, version );
        queryInsertIntoObject.bind( 2, (unsigned) Database::ObjectAction::Delete );
        queryInsertIntoObject.bind( 3, (unsigned) accessDomain );
        queryInsertIntoObject.bind( 4, (long long) id );
        queryInsertIntoObject.bind( 5, queryObject.getColumn( "version" ).getUInt() );
        if ( queryInsertIntoObject.exec() == 0 ) {
            // TODO: query failed, handle it.
            valid = false;
            LOG( WARNING ) << "Could not insert object";
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
    }

    // TODO: call object changed on this->db, the same way as in the local transactions?
    affectedObjects.insert( parent );
    affectedObjects.insert( { accessDomain, id } );
}

void RemoteTransaction::commit() {
    if ( !valid ) { // TODO: atomic test and set to make threading secure?
        LOG( WARNING ) << "Invalid transaction";
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidTransaction );
    }
    valid = false;
    LOG( DBUG ) << "Commit";

    // Check for collisions
    Database::Statement object( *connection.get(),
            "SELECT accessDomain, id, version, status, parent, parentAccessDomain, transactionAction "
            "FROM Object "
            "WHERE version=? "
            "ORDER BY id " );
    object << version;
    while( object.executeStep() ) {
        // TODO: check new objects to see if there is a collision
        Database::ObjectAction action{
            static_cast<Database::ObjectAction>(
                    object.getColumn( "transactionAction" ).getUInt()
                )
        };

        if ( Database::ObjectAction::New == action ) {
            checkNew( Database::statementRowToObjectMeta( object ) );
        }
    }

    // Check and verify transaction hash value
    Database::Transaction meta{ db->getTransactionMeta( version, connection.get() ) };
    if ( !( hash == db->calculateTransactionHash( meta, connection.get() ) ) ) {
        rollback();
        LOG( WARNING ) << "Transaction hash value is not a match";
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidTransaction );
    }

    // TODO: some sort of lock here,
    // to prevent changes to the database before "objectChanged" has finished
    transaction->commit();
    db->commit( this );
    LOG( DBUG ) << "Transaction commited.";

    for( const Database::ObjectRef& oref : affectedObjects ) {
        db->objectChanged( oref );
    }
    // TODO: release above suggested lock
}

void RemoteTransaction::rollback() {
    transaction.reset();
    db->rollback( this );
    valid = false;
}

void RemoteTransaction::checkNew( Database::ObjectMeta& object ) {
    // Check the objects status
    Database::ObjectStatus parentStatus{ getParentStatus( object ) };
    if ( object.status != parentStatus ) {
        // TODO
    }
    if ( Database::ObjectStatus::Current == object.status ) {
        // TODO
    }

    // Check if we have a collision
    Database::Statement isColliding( *connection.get(),
            "SELECT version "
            "FROM Object "
            "WHERE accessDomain=? AND id=? AND version < ? " );
    isColliding <<
            static_cast<unsigned>( object.accessDomain ) <<
            static_cast<long long>( object.id ) <<
            version;
    if ( isColliding.executeStep() ) {
        /*
         * We have a number conflict. We must check whether this is a number
         * conflict because the same number has been used in parallel transactions,
         * or a conflict because of an error with this transaction.
         */

        std::unique_ptr<Helper::Database::SavePoint> savePoint{
            new Helper::Database::SavePoint( connection.get(), "renumber" ) };

        // TODO Check Version In Transaction History ???

        unsigned long newId{ findNewId( object.id ) };
        Database::ObjectRef oRef{};
        oRef.accessDomain = object.accessDomain;
        oRef.id = object.id;

        // Update the object id
        Database::Statement updateObject( *connection.get(),
                "UPDATE Object "
                "SET id=? "
                "WHERE accessDomain=? AND id=? AND version=? ");
        updateObject <<
                static_cast<long long>( newId ) <<
                static_cast<unsigned>( accessDomain ) <<
                static_cast<long long>( object.id ) <<
                version;
        updateObject.exec();

        Database::Statement updateAttributes( *connection.get(),
                "UPDATE Attribute "
                "SET id=? "
                "WHERE accessDomain=? AND id=? AND version=? ");
        updateAttributes <<
                static_cast<long long>( newId ) <<
                static_cast<unsigned>( accessDomain ) <<
                static_cast<long long>( object.id ) <<
                version;
        updateAttributes.exec();

        // Change the local objects id to reflect the change in the database
        object.id = newId;

        // Map the old id to the new id
        Database::Statement insertRenumber(  *connection.get(),
                "INSERT INTO Renumber (accessDomain, version, oldId, newId) "
                "VALUES (?, ?, ?, ?) " );
        insertRenumber <<
                static_cast<unsigned>( accessDomain ) <<
                version <<
                static_cast<long long>( object.id ) <<
                static_cast<long long>( newId );
        insertRenumber.exec();

        // Apply the savepoint
        savePoint->save();
        // Insert this as late as possible in case something throws.
        // Otherwise this transaction will be in a inconsistent state.
        renumber.emplace( oRef, newId );
        // TODO: verify the above
        // TODO: implement the use of renumber everywhere! ( e.g. when reading transactions )
    }
}

void RemoteTransaction::checkNew( Database::ObjectMeta&& object ) {
    // TODO make better use of rvalue, this is just expensive
    Database::ObjectMeta tmp{ std::move( object ) };
    checkNew( tmp );
}

Database::ObjectStatus RemoteTransaction::getParentStatus( const Database::ObjectMeta& object ) const {
    using Status = Database::ObjectStatus;
    std::string parentQuery{};
    if ( last ) {
        parentQuery =
                "SELECT accessDomain, id, version, status, parent, parentAccessDomain, transactionAction "
                "FROM Object "
                "WHERE accessDomain=? AND id=? AND status < ? ";

    } else {
        parentQuery =
                "SELECT accessDomain, id, version, status, parent, parentAccessDomain, transactionAction "
                "FROM Object "
                "WHERE accessDomain=? AND id=? AND version=( "
                    "SELECT MAX(version) "
                    "FROM Object "
                    "WHERE accessDomain=? AND id=? AND version <= ? AND status < ? ) ";
    }
    Database::Statement parentRow( *connection.get(), parentQuery );
    if ( last ) {
        parentRow <<
                static_cast<unsigned>( object.parent.accessDomain ) <<
                static_cast<long long>( object.parent.id ) <<
                static_cast<unsigned>( Status::Old );
    } else {
        parentRow <<
                static_cast<unsigned>( object.parent.accessDomain ) <<
                static_cast<long long>( object.parent.id ) <<
                static_cast<unsigned>( object.parent.accessDomain ) <<
                static_cast<long long>( object.parent.id ) <<
                static_cast<unsigned>( Status::InvalidMove );
    }

    if ( !parentRow.executeStep() ) {
        // No parent
        return Status::InvalidParent;
    }

    Database::ObjectMeta parent{ Database::statementRowToObjectMeta( parentRow ) };
    if ( isInvalidStatus( parent.status ) ) {
        // Invalid parent
        return Status::InvalidParent;
    } else if ( isDeletedStatus( parent.status ) ) {
        return Status::Deleted;
    } else {
        return Status::Current;
    }
}

bool RemoteTransaction::isInvalidStatus( const Database::ObjectStatus& status ) const {
    using Status = Database::ObjectStatus;
    switch ( status ) {
    case Status::InvalidNew:
    case Status::InvalidParent:
    case Status::OldInvalidNew:
    case Status::OldInvalidParent:
        return true;
    default:
        return false;
    }
}

bool RemoteTransaction::isDeletedStatus( const Database::ObjectStatus& status ) const {
    using Status = Database::ObjectStatus;
    switch ( status ) {
    case Status::Deleted:
    case Status::DeletedParent:
    case Status::OldDeleted:
    case Status::OldDeletedParent:
        return true;
    default:
        return false;
    }
}

// TODO: change this to some stable/predictable (crypto) rehashing function
unsigned long RemoteTransaction::nextNumber( unsigned long num ) const {
    num = static_cast<unsigned>( num + 1 );
    if ( num <= Database::RESERVED ) {
        num = Database::RESERVED + 1;
    }
    return num;
}

unsigned long RemoteTransaction::findNewId( unsigned long id ) const {
    unsigned long newId{ nextNumber( id ) };
    Database::Statement alreadyExist( *connection.get(),
            "SELECT id "
            "FROM Object "
            "WHERE accessDomain=? AND id=? AND version <= ? ");
    alreadyExist <<
            static_cast<unsigned>( accessDomain ) <<
            static_cast<long long>( newId ) <<
            version;
    if ( alreadyExist.executeStep() ) {
        return findNewId( newId );
    }
    return newId;
}

Database::ObjectRef RemoteTransaction::getParent( unsigned long id ) const {
    Database::Statement getParent( *connection.get(),
            "SELECT accessDomain, id "
            "FROM Object "
            "WHERE id=( "
                "SELECT parent "
                "FROM Object "
                "WHERE id=? "
                "ORDER BY version DESC "
            ") " );
    getParent << static_cast<long long>( id );
    if ( getParent.executeStep() ) {
        Database::ObjectRef parent{ accessDomain, 0 };
        parent.accessDomain = static_cast<Database::AccessDomain>( getParent.getColumn( "accessDomain" ).getUInt() );
        parent.id = static_cast<unsigned long>( getParent.getColumn( "id" ).getInt64() );
        return parent;
    } else {
        LOG( DBUG ) << "Parent object not found";
        throw Exception( Error::ErrorCode::NotFound );
    }
}

bool RemoteTransaction::objectExists( unsigned long id ) const {
    Database::Statement queryId( *connection.get(),
            "SELECT id FROM Object "
            "WHERE accessDomain=? AND id=? AND version=?" );
    queryId <<
            static_cast<unsigned>( accessDomain ) <<
            static_cast<long long>( id ) <<
            version;
    if ( queryId.executeStep() ) {
        return true;
    }
    return false;
}

bool RemoteTransaction::olderVersionOfObjectExists( unsigned long id ) const {
    Database::Statement queryId( *connection.get(),
            "SELECT id FROM Object "
            "WHERE accessDomain=? AND id=? AND version < ?" );
    queryId <<
            static_cast<unsigned>( accessDomain ) <<
            static_cast<long long>( id ) <<
            version;
    if ( queryId.executeStep() ) {
        return true;
    }
    return false;
}

void RemoteTransaction::insertObject( unsigned long id, unsigned status,
        unsigned long parentId, unsigned parentAccessDomain, unsigned action ) {
    Database::Statement insertObject( *connection.get(),
            "INSERT INTO Object (accessDomain, id, parent, parentAccessDomain, version, status, transactionAction) "
            "VALUES (?, ?, ?, ?, ?, ?, ?)" );
    insertObject <<
            static_cast<unsigned>( accessDomain ) <<
            static_cast<long long>( id ) <<
            static_cast<long long>( parentId ) <<
            parentAccessDomain <<
            version <<
            status <<
            action;
    if ( !insertObject.exec() ) {
        valid = false;
        LOG( WARNING ) << "Failed to insert object into database";
        throw Exception( Error::ErrorCode::UnexpectedDatabaseError );
    }
}



} /* namespace Mist */
