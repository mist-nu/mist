/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "simulator.hpp"

namespace Simulator
{

const std::string central2{ FS::path( "central2" ).string() };
const std::string thisCentral{ FS::path( "central1" ).string() };

TEST( Mist, ReceiveDatabase ) {
    LOG( INFO ) << "Creating Central 1 and extracting user";

    // Create central
    M::Central central( thisCentral, true );
    central.create();
    central.init();

    // Read "remote" database manifest
    std::fstream mfs( central2 + ".manifest.json", std::fstream::in | std::fstream::binary );
    std::string mstr((std::istreambuf_iterator<char>(mfs)), std::istreambuf_iterator<char>());
    mfs.close();
    M::Database::Manifest manifest{
        M::Database::Manifest::fromString( mstr,
                std::bind( &M::Central::verify, &central, _1, _2, _3)
        )
    };

    M::Database* db{ central.receiveDatabase( manifest ) };

    // Read "remote" transactions
    std::fstream tfs( central2 + ".json", std::fstream::in | std::fstream::binary );
    db->writeToDatabase( *tfs.rdbuf() );
    tfs.close();

    // Write a transaction to db2

    // Attributes
    std::map<std::string, V> attributes;

    // Transaction pointer
    std::unique_ptr<M::Transaction> t{};

    // Object references
    M::Database::ObjectRef object{ M::Database::AccessDomain::Normal, 0 };
    M::Database::ObjectRef object2{ object };
    M::Database::ObjectRef object3{ object };

    // New object
    t = std::move( db->beginTransaction() );
    attributes.clear();
    attributes.emplace( "Hello", "world" );
    object.id = t->newObject( object, attributes );
    t->commit();
    t.reset();

    // Test value types
    t = std::move( db->beginTransaction() );
    attributes.clear();
    attributes.emplace( "Null", nullptr );
    attributes.emplace( "Boolean", true );
    attributes.emplace( "Number", 2.0 );
    attributes.emplace( "String", "text" );
    attributes.emplace( "Json", V( "{}", true ) );
    object.id = t->newObject( object, attributes );
    t->commit();
    t.reset();

    // Multiple objects
    t = std::move( db->beginTransaction() );
    attributes.clear();
    attributes.emplace( "a", "v" );
    object.id = t->newObject( object, attributes );
    object2.id = t->newObject( object, attributes );
    object3.id = t->newObject( object2, attributes );
    t->commit();
    t.reset();

    // Change objects
    t = std::move( db->beginTransaction() );
    attributes.clear();
    attributes.emplace( "changed", "object" );
    t->updateObject( object.id, attributes );
    attributes.clear();
    attributes.emplace( "2", "changed" );
    t->updateObject( object2.id, attributes );
    t->commit();
    t.reset();

    // Move object
    t = std::move( db->beginTransaction() );
    attributes.clear();
    t->moveObject( object3.id, object );
    t->commit();
    t.reset();

    // Delete object
    t = std::move( db->beginTransaction() );
    attributes.clear();
    t->deleteObject( object3.id );
    t->commit();
    t.reset();

    // Shutdown
    db->close();
    central.close();
}

} /* namespace Simulator */
