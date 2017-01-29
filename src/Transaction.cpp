/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "Exception.h"
#include "Helper.h"
#include "Transaction.h"

namespace Mist {

namespace {

unsigned long long cryptoRandom() {
    // TODO: Consider a max recursion counter.
    unsigned long long randomValue = CryptoHelper::cryptoRandom();
    if ( randomValue <= Database::RESERVED )
        return cryptoRandom();
    return randomValue;
}

Database::ObjectStatus convertStatusToOld( Database::ObjectStatus objectStatus ) {
    switch ( objectStatus ) {
    case Database::ObjectStatus::Current:
        return Database::ObjectStatus::Old;
    case Database::ObjectStatus::Deleted:
        return Database::ObjectStatus::OldDeleted;
    case Database::ObjectStatus::DeletedParent:
        return Database::ObjectStatus::OldDeletedParent;
    case Database::ObjectStatus::InvalidNew:
        return Database::ObjectStatus::OldInvalidNew;
    case Database::ObjectStatus::InvalidParent:
        return Database::ObjectStatus::OldInvalidParent;
    default:
        throw Mist::Exception( "Transaction: Can not convert status" );
    }
}

}

Transaction::Transaction( Database *db, Database::AccessDomain accessDomain, unsigned version ) :
        db( db ),
        connection( std::move( db->getIsolatedDbConnection() ) ),
        transaction( new Helper::Database::Transaction( *connection.get() ) ),
        accessDomain( accessDomain ),
        version( version ),
        parentAccessDomains{},
        valid( true ) {
    // TODO: Get a isolated transaction connection to the db.
    if ( db == nullptr ) {
        // TODO: handle this.
        // throw ?
    }
    LOG( DBUG ) << "Transaction( Database:" << db << " , AccessDomain:" << (int) accessDomain << " version:" << version;
}

Transaction::~Transaction() {
    rollback();
}

void Transaction::addAccessDomainDependency( Database::AccessDomain accessDomain, unsigned version ) {
    parentAccessDomains.insert( accessDomain );
    // TODO: If the parentAccessDomains is changed to a map, it should be reflected here as well.
}

unsigned long Transaction::allocateObjectId() {
    // TODO: Check if we need to allocate 64 bit numbers
    // TODO: Consider a max recursion counter.
    unsigned long newId = static_cast<unsigned>( cryptoRandom() );
    if ( newId == Database::ROOT_OBJECT_ID ) {
        return allocateObjectId();
    }

    // TODO: error handling of query call, either here or in the Database.
    //Database::Statement query = db->query( "SELECT id FROM Object WHERE id=? AND accessDomain=?" );
    Database::Statement query( *connection.get(), "SELECT id FROM Object WHERE id=? AND accessDomain=?" );
    query.bind( 1, (long long) newId ); // TODO: verify correct behavior.
    query.bind( 2, (unsigned int) accessDomain ); // TODO: verify correct behavior.
    if ( query.executeStep() > 0 ) {
        query.clearBindings(); // TODO: check if it is this or reset that should be called.
        query.reset();
        return allocateObjectId();
    }
    return newId;
}

unsigned long Transaction::newObject( const Database::ObjectRef &parent, const std::map<std::string, Database::Value> &attributes ) {
    LOG( DBUG ) << "Creating new object with parent " << parent.id;
    if ( !valid ) {
        valid = false;
        LOG ( WARNING ) << "Invalid transaction";
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidTransaction );
    }
    /*
    if ( false ) { // TODO: Something like "if ( db->validCrossAccessDomainParent( parent.accessDomain, accessDomain) )"
        valid = false;
        LOG ( WARNING ) << "Invalid parent";
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidParent );
    }
    //*/

    // TODO: Refactor to be more similar to updateObject by creating an Database::Object

    // TODO: Why are these created here? Could they be placed inside the following 'if' block?
    Database::AccessDomain parentAccessDomain;
    unsigned parentVersion;

    if ( parent.id == Database::USERS_OBJECT_ID ) {
        // TODO: verify user permission?
    } else if ( parent.id != Database::ROOT_OBJECT_ID ) {
        // TODO: handle query exceptions.
        Database::Statement query( *connection.get(),
                "SELECT accessDomain, id, version, transactionAction "
                "FROM Object "
                "WHERE accessDomain=? AND id=? AND status=? " );
        // TODO: verify correct behavior when casting during the binding.
        query << (int) parent.accessDomain << (long long) parent.id << (int) Database::ObjectStatus::Current;
        if ( query.executeStep() ) { // TODO: handle throws from execute
            parentAccessDomain = (Database::AccessDomain) query.getColumn( "accessDomain" ).getInt(); // TODO: verify correct behavior.
            parentVersion = (unsigned) query.getColumn( "version" ).getInt64(); // TODO: verify correct behavior.
            if ( parentAccessDomain != accessDomain ) {
                addAccessDomainDependency( parentAccessDomain, parentVersion );
            }
        } else {
            valid = false;
            LOG ( WARNING ) << "Parent not found.";
            throw Mist::Exception( Mist::Error::ErrorCode::NotFound );
        }
    }

    unsigned long newId{ allocateObjectId() }; // TODO: what happens if this id is generated somewhere else but it has not arrived here yet?
    LOG ( DBUG ) << "New object id: " << newId;
    Database::Statement query( *connection.get(),
            "INSERT INTO Object (accessDomain, id, version, status, parent, parentAccessDomain, transactionAction) "
            "VALUES (?, ?, ?, ?, ?, ?, ?)" );
    query << static_cast<int>( accessDomain )
            << static_cast<long long>( newId )
            << version
            << static_cast<int>( Database::ObjectStatus::Current )
            << static_cast<long long>( parent.id )
            << static_cast<int>( parent.accessDomain )
            << static_cast<int>( Database::ObjectAction::New );
    if ( query.exec() == 0 ) { // TODO: zero-rows affected
        valid = false;
        LOG ( WARNING ) << "Could not insert new object: " << newId;
        throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
    }

    Database::Statement insertAttribute( *connection.get(),
            "INSERT INTO Attribute (accessDomain, id, version, name, type, value) "
            "VALUES (?, ?, ?, ?, ?, ?) " );
    for ( auto const & kv : attributes ) {
        insertAttribute << static_cast<int>( accessDomain )
                << static_cast<long long>( newId )
                << version
                << kv.first
                << static_cast<int>( kv.second.t );
        using T = Database::Value::Type;
        switch ( kv.second.t ){
        case T::Typeless:
            break;
        case T::Null:
            break;
        case T::Boolean:
            insertAttribute << kv.second.b;
            break;
        case T::Number:
            insertAttribute << kv.second.n;
            break;
        case T::String:
            insertAttribute << kv.second.v;
            break;
        case T::Json:
            insertAttribute << kv.second.v;
            break;
        default:
            LOG( WARNING ) << "Attribute statement does not contain correct type.";
            throw std::runtime_error( "Attribute statement does not contain correct type." );
        }

        if ( insertAttribute.exec() ) {
            insertAttribute.clearBindings();
            insertAttribute.reset();
        } else {
            valid = false;
            LOG ( WARNING ) << "Could not insert attributes into the database";
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
    }

    affectedObjects.insert( parent );
    affectedObjects.insert( { accessDomain, newId } );

    return newId;
}

/*
 * Check that new parent exists
 * TODO: Check that object exists (may be deleted)
 * TODO: Check for loops
 * If object was created in same transaction
 *     New, Move, MoveUpdate -> Update parent
 *     Update -> Update parent and make into MoveUpdate
 *     Delete -> Update parent and make into Move, copy attributes from last version
 */
void Transaction::moveObject( unsigned long id, const Database::ObjectRef &newParent ) {
    LOG( DBUG ) << "Move object " << id;
    if ( !valid ) {
        LOG( WARNING ) << "Current Transaction object is NOT valid.";
        valid = false;
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidTransaction );
    }
    // TODO: ? "if ( db->validCrossAccessDomainParent( newParent.accessDomain, accessDomain) ) throw ...InvalidParent" ?
    if ( accessDomain == newParent.accessDomain && id == newParent.id ) {
        // TODO: should this be done?
        LOG( WARNING ) << "Invalid Parent, transaction no longer valid.";
        valid = false;
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidParent );
    }

    // TODO: Refactor to be more similar to updateObject by creating an Database::Object

    Mist::Database::Statement query( *connection.get(),
            "SELECT id, parent, parentAccessDomain, version, status, transactionAction "
            "FROM Object "
            "WHERE accessDomain=? AND id=? AND status <= ? " );
    query <<
            (int) accessDomain <<
            (long long) id <<
            (int) Database::ObjectStatus::DeletedParent;
    if ( !query.executeStep() ) {
        LOG( WARNING ) << "Not Found, transaction no longer valid.";
        valid = false;
        throw Mist::Exception( Mist::Error::ErrorCode::NotFound );
    }

    if ( newParent.id == (unsigned long) query.getColumn( "parent" ).getInt64() &&
            newParent.accessDomain == (Database::AccessDomain) query.getColumn( "parentAccessDomain" ).getInt() ) {
        // TODO: Nothing to move, it is already there. throw?
        LOG( DBUG ) << "NOOP";
        return;
    }

    // Needs to be in this scope since it's used further down at the moment.
    Mist::Database::Statement parentQuery( *connection.get(),
            "SELECT accessDomain, id, version, parent, parentAccessDomain "
            "FROM Object "
            "WHERE accessDomain=? AND id=? AND status=?" );

    if ( newParent.id != db->ROOT_OBJECT_ID ) {
        parentQuery <<
                (int) newParent.accessDomain <<
                (long long) newParent.id <<
                (int) Database::ObjectStatus::Current;
        if ( parentQuery.executeStep() ) {
            Database::AccessDomain parentAccessDomain = (Database::AccessDomain) parentQuery.getColumn( "accessDomain" ).getInt();
            if ( parentAccessDomain != accessDomain ) {
                // TODO: verify this.
                addAccessDomainDependency( parentAccessDomain, parentQuery.getColumn( "version" ).getUInt() );
            }
            // TODO: skip access doamin check?
        } else { // Got 0 rows
            LOG( WARNING ) << "Not Found, transaction no longer valid";;
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::NotFound );
        }
    }

    if ( query.getColumn( "version" ).getUInt() != version ) {
        Database::Statement updateObj( *connection.get(),
                "UPDATE Object SET status=? WHERE accessDomain=? AND id=? AND version=?" );
        updateObj <<
                (int) convertStatusToOld( { (Database::ObjectStatus) query.getColumn( "status" ).getUInt() } ) <<
                (int) accessDomain <<
                (long long) id <<
                query.getColumn( "version" ).getUInt();
        if ( updateObj.exec() == 0 ) { // 0 rows affected.
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid.";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }

        Database::Statement insertObj( *connection.get(),
                "INSERT INTO Object (accessDomain, id, version, status, parentAccessDomain, parent, transactionAction) "
                "VALUES (?, ?, ?, ?, ?, ?, ?)" );
        insertObj <<
                (int) accessDomain <<
                (long long) id <<
                version <<
                (int) Database::ObjectStatus::Current <<
                (int) newParent.accessDomain <<
                (long long) newParent.id <<
                (int) Database::ObjectAction::Move;
        if ( insertObj.exec() == 0 ) { // 0 rows affected.
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid.";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }

        Database::Statement insertAttr( *connection.get(),
                "INSERT INTO Attribute (accessDomain, id, version, name, type, value) "
                "SELECT accessDomain, id, ?, name, type, value "
                "FROM Attribute "
                "WHERE accessDomain=? AND id=? AND version=? " );
        insertAttr <<
                version <<
                (int) accessDomain <<
                (long long) id <<
                query.getColumn( "version" ).getUInt();
        if ( insertAttr.exec() == 0 ) { // 0 rows affected.
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid.";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
    } else {
        /*
         * Same transaction.
         *     New, Move, MoveUpdate -> Update parent
         *     Update -> Update parent and make into MoveUpdate
         *     Delete -> Update parent and make into Move, copy attributes from last version
         */
        Database::Statement updateObj( *connection.get(),
                "UPDATE Object SET transactionAction=?, status=?, parentAccessDomain=?, parent=? "
                "WHERE accessDomain=? AND id=? AND version=?" );
        updateObj <<
                (int) (
                ( (Database::ObjectAction) query.getColumn( "transactionAction" ).getUInt() ) == Database::ObjectAction::Update ?
                        Database::ObjectAction::MoveUpdate :
                        ( ( (Database::ObjectAction) query.getColumn( "transactionAction" ).getUInt() ) == Database::ObjectAction::Delete ?
                                Database::ObjectAction::Move : (Database::ObjectAction) query.getColumn( "transactionAction" ).getUInt() ) ) <<
                (int) Database::ObjectStatus::Current <<
                (int) newParent.accessDomain <<
                (long long) newParent.id <<
                (int) accessDomain <<
                (long long) id <<
                version;
        if ( updateObj.exec() == 0 ) { // 0 rows affected.
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid.";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }

        if ( ( (Database::ObjectAction) query.getColumn( "transactionAction" ).getUInt() ) == Database::ObjectAction::Delete ) {
            // TODO: return?
        } else {
            Database::Statement insertAttr( *connection.get(),
                    "INSERT INTO Attribute (accessDomain, id, version, name, type, value ) "
                    "SELECT accessDomain, id, ?, name, type, value "
                    "FROM Attribute "
                    "WHERE accessDomain=? AND id=? AND version=( "
                        "SELECT MAX(version) "
                        "FROM Object "
                        "WHERE accessDomain=? AND id=? AND status <= ? AND version < ? ) " );
            insertAttr <<
                    version <<
                    (int) accessDomain <<
                    (long long) id <<
                    (int) accessDomain <<
                    (long long) id <<
                    (int) Database::ObjectStatus::DeletedParent <<
                    version;
            if ( insertAttr.exec() == 0 ) { // 0 rows affected.
                LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
                valid = false;
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }
        }
        Database::ObjectRef oldParent { (Database::AccessDomain) parentQuery.getColumn( "parentAccessDomain" ).getUInt(), (unsigned long) parentQuery.getColumn( "parent" ).getInt64() };

        affectedObjects.insert( oldParent );
        affectedObjects.insert( { accessDomain, id } );
        affectedObjects.insert( newParent );
    }
}

/*
 * Check that object exists (may be deleted)
 * If object was deleted - check that parent still exists
 * If object was created in same transaction
 *     New, Change, ChangeMove -> Update attributes
 *     Move -> Update attributes and make into ChangeMove
 *     Delete -> Replace with change
 */
void Transaction::updateObject( unsigned long id, const std::map<std::string, Database::Value> &attributes ) {
    LOG( DBUG ) << "Update object: " << id;
    if ( !valid ) {
        LOG( WARNING ) << "Transaction no longer valid";
        valid = false;
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidTransaction );
    }
    // TODO: accessDomain check?

    Database::Statement query( *connection.get(),
            "SELECT id, version, transactionAction, parent, parentAccessDomain "
            "FROM Object "
            "WHERE accessDomain=? AND id=? AND status < ? " );
    query <<
            (int) accessDomain <<
            (long long) id <<
            (int) Database::ObjectStatus::DeletedParent;

    if ( !query.executeStep() ) {
        LOG( WARNING ) << "Not Found, transaction no longer valid";
        valid = false;
        throw Mist::Exception( Mist::Error::ErrorCode::NotFound );
    }

    Database::ObjectRef parentRef {
        (Database::AccessDomain) query.getColumn( "parentAccessDomain" ).getUInt(),
        (unsigned long) query.getColumn( "parent" ).getInt64()
    };

    std::map<std::string, Database::Value> emptyAttributeMap {};
    Database::Object obj { accessDomain, id, query.getColumn( "version" ).getUInt(), parentRef, emptyAttributeMap, Database::ObjectStatus::DeletedParent, (Database::ObjectAction) query.getColumn( "transactionAction" ).getUInt() };

    if ( obj.parent.id != Database::ROOT_OBJECT_ID ) {
        obj.status = Database::ObjectStatus::Current;
        Database::Statement parentQuery( *connection.get(),
                "SELECT id, version, transactionAction "
                "FROM Object "
                "WHERE accessDomain=? AND id=? AND status=?" );
        parentQuery <<
                query.getColumn( "parentAccessDomain" ).getUInt() <<
                query.getColumn( "parent" ).getInt64() <<
                static_cast<int>( Database::ObjectStatus::Current );
        if ( !parentQuery.executeStep() ) {
            LOG( WARNING ) << "Parent object not found";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::NotFound );
        }
    }

    if ( obj.version != version ) {
        Database::Statement updateObj( *connection.get(),
                "UPDATE Object SET status=? WHERE accessDomain=? AND id=? AND version=?" );
        updateObj <<
                (int) convertStatusToOld( obj.status ) <<
                (int) obj.accessDomain <<
                (long long) obj.id << obj.version;
        if ( updateObj.exec() == 0 ) { // 0 rows affected
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }

        obj.status = Database::ObjectStatus::Current;
        obj.action = Database::ObjectAction::Update;

        Database::Statement insertObj( *connection.get(),
                "INSERT INTO Object (accessDomain, id, version, status, parent, parentAccessDomain, transactionAction) "
                "VALUES (?, ?, ?, ?, ?, ?, ?)" );
        insertObj <<
                static_cast<unsigned>( accessDomain ) <<
                static_cast<long long>( id ) <<
                version <<
                static_cast<unsigned>( Database::ObjectStatus::Current ) <<
                static_cast<long long>( obj.parent.id ) <<
                static_cast<unsigned>( obj.parent.accessDomain ) <<
                static_cast<unsigned>( Database::ObjectAction::Update );
        if ( insertObj.exec() == 0 ) { // 0 rows affected.
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
    } else {
        /*
         * Same transaction.
         *     New, Update, MoveUpdate -> Update attributes
         *     Move -> Update attributes and make into UpdateMove
         *     Delete -> Replace with Update
         */
        Database::Statement deleteAttr( *connection.get(),
                "DELETE FROM Attribute WHERE accessDomain=? AND id=? AND version=?" );
        deleteAttr <<
                (int) obj.accessDomain <<
                (long long) obj.id <<
                obj.version;
        if ( deleteAttr.exec() == 0 ) { // 0 rows affected.
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }

        if ( obj.action == Database::ObjectAction::Move ) {
            obj.action = Database::ObjectAction::MoveUpdate;
            Database::Statement updateObj( *connection.get(),
                    "UPDATE Object SET transactionAction=? WHERE accessDomain=? AND id=? AND version=?" );
            updateObj <<
                    (int) obj.action <<
                    (int) obj.accessDomain <<
                    (long long) obj.id <<
                    obj.version;
            if ( updateObj.exec() == 0 ) { // 0 rows affected.
                LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
                valid = false;
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }
        } else if ( obj.action == Database::ObjectAction::Delete ) {
            obj.action = Database::ObjectAction::Update;
            obj.status = Database::ObjectStatus::Current;

            Database::Statement updateObj( *connection.get(),
                    "UPDATE Object SET transactionAction=?, status=? WHERE accessDomain=? AND id=? AND version=?" );
            updateObj <<
                    (int) obj.action <<
                    (int) obj.status <<
                    (int) obj.accessDomain <<
                    (long long) obj.id <<
                    obj.version;
            if ( updateObj.exec() == 0 ) { // 0 rows affected.
                LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
                valid = false;
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }
        }
    }

    Database::Statement insertIntoAttribute( *connection.get(),
            "INSERT INTO Attribute (accessDomain, id, version, name, type, value) "
            "VALUES (?, ?, ?, ?, ?, ?)" );
    for ( auto const & kv : attributes ) {
        insertIntoAttribute <<
                static_cast<int>( obj.accessDomain ) <<
                static_cast<long long>( obj.id ) <<
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
            insertIntoAttribute.clearBindings();
            insertIntoAttribute.reset();
        } else {
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
    }

    affectedObjects.insert( obj.parent );
    affectedObjects.insert( { obj.accessDomain, obj.id } );
}

/*
 * Check that object exists (must not be deleted, but could be deletedParent)
 * Check that object does not have any children that are not also deleted (only for this access domain)
 * If object was created in same truncation
 *     New -> Just delete object from database without any trace
 *     Update -> Remove attributes, make into Delete
 *     Move, MoveUpdate -> Restore parent, remove attributes, make into Delete
 */
void Transaction::deleteObject( unsigned long id ) {
    LOG( DBUG ) << "Delete object " << id;
    if ( !valid ) {
        valid = false;
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidTransaction );
    }
    // TODO: accessDomain check?

    Database::Statement query( *connection.get(),
            "SELECT id, version, transactionAction, parent, parentAccessDomain "
            "FROM Object "
            "WHERE accessDomain=? AND id=? AND status < ?" );
    query << (int) accessDomain << (long long) id << (int) Database::ObjectStatus::DeletedParent;
    if ( query.executeStep() == 0 ) { // 0 rows
        LOG( WARNING ) << "Not Found, transaction no longer valid";
        valid = false;
        throw Mist::Exception( Mist::Error::ErrorCode::NotFound );
    }
    Database::ObjectRef parentRef { (Database::AccessDomain) query.getColumn( "parentAccessDomain" ).getUInt(), (unsigned long) query.getColumn( "parent" ).getInt64() };
    std::map<std::string, Database::Value> emptyAttributeMap { };
    Database::Object obj { accessDomain, id, query.getColumn( "version" ).getUInt(), parentRef, emptyAttributeMap, Database::ObjectStatus::DeletedParent, (Database::ObjectAction) query.getColumn( "transactionAction" ).getUInt() };

    if ( obj.status == Database::ObjectStatus::Deleted ) {
        // TODO: Why check for this? Didn't we just specify status = DeletedParent a few lines ago?
        // noop
        return;
    }

    Database::Statement queryChild( *connection.get(),
            "SELECT COUNT(id) AS count "
            "FROM Object "
            "WHERE accessDomain=? AND parentAccessDomain=? AND parent=? AND status=?" );
    queryChild <<
            (int) obj.accessDomain <<
            (int) obj.accessDomain <<
            (long long) obj.id <<
            (int) Database::ObjectStatus::Current;
    if ( queryChild.executeStep() == 0 ) { // 0 rows
        LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
        valid = false;
        throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
    }

    if ( queryChild.getColumn( "count" ).getInt() > 0 ) {
        LOG( WARNING ) << "Not Empty, transaction no longer valid";
        valid = false;
        throw Mist::Exception( Mist::Error::ErrorCode::NotEmpty );
    }

    if ( obj.version != version ) {
        Database::Statement updateObj( *connection.get(),
                "UPDATE Object SET status=? WHERE accessDomain=? AND id=? AND version=? " );
        updateObj <<
                (int) convertStatusToOld( obj.status ) <<
                (int) obj.accessDomain <<
                (long long) obj.id <<
                obj.version;
        if ( updateObj.exec() == 0 ) { // 0 rows
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }

        Database::Statement insertIntoObj( *connection.get(),
                "INSERT INTO Object (accessDomain, id, version, status, transactionAction) "
                "VALUES (?, ?, ?, ?, ?)" );
        insertIntoObj <<
                (int) obj.accessDomain <<
                (long long) obj.id <<
                version <<
                (int) Database::ObjectStatus::Deleted <<
                (int) Database::ObjectAction::Delete;
        if ( insertIntoObj.exec() == 0 ) { // 0 rows
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
    } else {
        /*
         * We need to handle objects that are part of the transaction.
         *     New -> Just delete object from database without any trace
         *     Update -> Remove attributes, make into Delete
         *     Move, MoveUpdate -> Restore parent, remove attributes, make into Delete
         */
        Database::Statement deleteAttr( *connection.get(),
                "DELETE FROM Attribute WHERE accessDomain=? AND id=? AND version=?" );
        deleteAttr <<
                (int) obj.accessDomain <<
                (long long) obj.id <<
                obj.version;
        if ( deleteAttr.exec() == 0 ) { // 0 rows
            LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
            valid = false;
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }

        if ( obj.action == Database::ObjectAction::New ) {
            Database::Statement deleteObj( *connection.get(),
                    "DELETE FROM Object WHERE accessDomain=? AND id=? AND version=?" );
            deleteObj <<
                    (int) obj.accessDomain <<
                    (long long) obj.id <<
                    version;
            if ( deleteObj.exec() == 0 ) { // 0 rows
                LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
                valid = false;
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }
        } else if ( obj.action == Database::ObjectAction::Move ||
                obj.action == Database::ObjectAction::MoveUpdate ) {
            Database::Statement queryParent( *connection.get(),
                    "SELECT parentAccessDomain, parent "
                    "FROM Object "
                    "WHERE accessDomain=? AND id=? AND version=( "
                        "SELECT MAX(version) "
                        "FROM Object "
                        "WHERE accessDomain=? AND id=? AND status <= ? AND version < ? )" );
            queryParent <<
                    (int) obj.accessDomain <<
                    (long long) obj.id <<
                    (int) obj.accessDomain <<
                    (long long) obj.id <<
                    (int) Database::ObjectStatus::OldDeletedParent <<
                    version;
            if ( queryParent.executeStep() == 0 ) {
                LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
                valid = false;
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }

            Database::Statement updateObj( *connection.get(),
                    "UPDATE Object SET status=?, transactionAction=?, parentAccessDomain=?, parent=? "
                    "WHERE accessDomain=? AND id=? AND version=? " );
            updateObj <<
                    (int) Database::ObjectStatus::Deleted <<
                    (int) Database::ObjectAction::Delete <<
                    queryParent.getColumn( "parentAccessDomain" ).getUInt() <<
                    (long long) queryParent.getColumn( "parent" ).getInt64() <<
                    (int) obj.accessDomain <<
                    (long long) obj.id <<
                    version;
            if ( updateObj.exec() == 0 ) {
                LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
                valid = false;
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }
        } else { // Database::ObjectAction::.Update
            Database::Statement updateObj( *connection.get(),
                    "UPDATE Object SET status=?, transactionAction=? "
                    "WHERE accessDomain=? AND id=? AND version=?" );
            updateObj <<
                    (int) Database::ObjectStatus::Deleted <<
                    (int) Database::ObjectAction::Delete <<
                    (int) obj.accessDomain <<
                    (long long) obj.id <<
                    version;
            if ( updateObj.exec() == 0 ) {
                LOG( WARNING ) << "Unexpected Database Error, transaction no longer valid";
                valid = false;
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }
        }
    }

    affectedObjects.insert( obj.parent );
    affectedObjects.insert( { obj.accessDomain, obj.id } );
}

void Transaction::commit() {
    if ( !valid ) {
        LOG( WARNING ) << "Current Transaction object is NOT valid.";
        throw Mist::Exception( Mist::Error::ErrorCode::InvalidTransaction );
    }
    valid = false;
    LOG( DBUG ) << "Commit";

    Database::Statement insertTransaction( *connection.get(),
            "INSERT INTO 'Transaction' (accessDomain, version, timestamp, userHash, hash, signature) "
            "VALUES (?, ?, STRFTIME('%Y-%m-%d %H:%M:%f','now'), ?, NULL, NULL)" );
    // TODO: Wait here if it is less than a millisecond since the last local commit.
    try {
        insertTransaction << static_cast<int>( accessDomain ) << version << db->getUserHash();
        if ( insertTransaction.exec() == 0 ) {
            LOG( WARNING ) << "Unexpected Database Error";
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
    } catch (...) {
        LOG( WARNING ) << "Unexpected Database Error";
        throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
    }

    Database::Statement selectParents( *connection.get(),
            "SELECT t.accessDomain AS accessDomain, t.version AS version, timestamp, userHash, hash, signature "
            "FROM 'Transaction' AS t "
            "LEFT OUTER JOIN TransactionParent tp "
                "ON t.accessDomain=tp.parentAccessDomain AND t.version=tp.parentVersion "
            "WHERE tp.version IS NULL AND t.version!=? " ); // AND t.accessDomain=? " );
    // TODO: if the "access domain" concepts gets implemented
    // link the transaction with other access domains if this transaction creates or moves objects
    // so they get a parent from the other access domain

    Database::Statement insertTransactionParent( *connection.get(),
            "INSERT INTO TransactionParent (accessDomain, version, parentAccessDomain, parentVersion) "
                    "VALUES (?, ?, ?, ?)" );
    try {
        selectParents << version; // << static_cast<int>( accessDomain );
        while ( selectParents.executeStep() ) {
            insertTransactionParent << static_cast<int>( accessDomain )
                    << version
                    << selectParents.getColumn( "accessDomain" ).getUInt()
                    << selectParents.getColumn( "version" ).getUInt();
            if ( insertTransactionParent.exec() == 0 ) {
                LOG( WARNING ) << "Unexpected Database Error";
                throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
            }
            insertTransactionParent.clearBindings();
            insertTransactionParent.reset();
        }
    } catch (...) {
        LOG( WARNING ) << "Unexpected Database Error";
        throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
    }

    Database::Transaction thisMeta;
    try {
        thisMeta = db->getTransactionMeta( version, connection.get() );
    } catch (...) {
        LOG( WARNING ) << "Database Error: failed to get transaction meta data.";
        throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
    }

    // Calculate the transaction hash
    CryptoHelper::SHA3 hash;
    try {
        hash = db->calculateTransactionHash( thisMeta, connection.get() );
        LOG( DBUG ) << "Meta: version: " << thisMeta.version << " hash: " << thisMeta.hash.toString();
        LOG( DBUG ) << "New Hash: " << hash.toString();
    } catch ( const std::runtime_error& e ) {
        LOG( WARNING ) << "Hashing failed.";
        throw e;
    }

    // Sign the transaction
    CryptoHelper::Signature signature;
    try {
        signature = db->signTransaction( hash );
    } catch ( const std::runtime_error& e ) {
        LOG( WARNING ) << "Signing failed.";
        throw e;
    }

    // Update the database with the this users hash, transaction hash and signature
    Database::Statement updateTransaction( *connection.get(),
            "UPDATE 'Transaction' "
            "SET hash=?, signature=? "
            "WHERE version=?" );
    try {
        updateTransaction << hash.toString() << signature.toString() << version;
        if ( updateTransaction.exec() == 0 ) {
            // 0 rows affected
            // TODO: handle this.
            LOG( WARNING ) << "Unexpected Database Error";
            throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
        }
        thisMeta.hash = hash;
    } catch (...) {
        LOG( WARNING ) << "Unexpected Database Error";
        throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
    }
    //*/

    /*    try {
        version = db->reorderTransaction( thisMeta, connection.get() );
    } catch (...) {
        LOG( WARNING ) << "Reordering failed.";
        throw;
	}*/

    // TODO: some sort of lock here,
    // to prevent changes to the database before "objectChanged" has finished
    try {
        transaction->commit();
    } catch (...) {
        LOG( WARNING ) << "Commit failed.";
        throw Mist::Exception( Mist::Error::ErrorCode::UnexpectedDatabaseError );
    }
    db->commit( this );
    LOG( DBUG ) << "Transaction commited.";

    for( const Database::ObjectRef& oref : affectedObjects ) {
        db->objectChanged( oref );
    }
    // TODO: release above suggested lock
}

void Transaction::rollback() {
    valid = false;
    LOG( DBUG ) << "Rollback";
    // rollback helper transaction by deleting it.
    transaction.reset();
    //db->rollback( this );
}

Database::Object Transaction::getObject( int accessDomain, long long id, bool includeDeleted ) const {
    return db->getObject( connection.get() , accessDomain, id, includeDeleted );
}

Database::QueryResult Transaction::query( int accessDomain, long long id, const std::string& select,
            const std::string& filter, const std::string& sort,
            const std::map<std::string, Database::Value>& args,
            int maxVersion, bool includeDeleted ) {
    return db->query( connection.get(), accessDomain, id, select, filter, sort, args, maxVersion, includeDeleted );
}

Database::QueryResult Transaction::queryVersion( int accessDomain, long long id, const std::string& select,
            const std::string& filter, const std::map<std::string, Database::Value>& args,
            bool includeDeleted ) {
    return db->queryVersion( connection.get(), accessDomain, id, select, filter, args, includeDeleted );
}

} /* namespace Mist */
