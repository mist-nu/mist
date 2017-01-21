/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "simulator.hpp"

namespace Simulator
{

const std::string central1{ FS::path( "central1" ).string() };
const std::string thisCentral{ FS::path( "central2" ).string() };

TEST( Mist, CreateDbAndInviteUser ) {
    LOG( INFO ) << "Creating Central 2, a database and inviting user 1";
    removeCentral( thisCentral );

    // Create central
    M::Central central( thisCentral, true );
    central.create();
    central.init();

    // Create database
    M::Database* db{ central.createDatabase( "db2" ) };

    // Extract user pem
    std::ofstream user2Fs( thisCentral + ".user.pem", std::fstream::binary | std::fstream::trunc );
    user2Fs << central.getPublicKey().toString();
    user2Fs.close();

    // Read user pem
    std::ifstream user1Fs( central1 + ".user.pem", std::fstream::binary );
    std::string user1Pem(static_cast<std::stringstream const&>(std::stringstream() << user1Fs.rdbuf()).str());
    user1Fs.close();

    // Load PublicKey from Pem
    M::CryptoHelper::PublicKey user1Key{ M::CryptoHelper::PublicKey::fromPem( user1Pem ) };

    // Create user1 base on the publickey
    M::UserAccount user1( "user1", user1Key, user1Key.hash(), { M::Permission::P::admin } );

    // Invite user1 to db2
    db->inviteUser( user1 );

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

    // Dump the db so central 1 can receive the database
    db->dump( thisCentral );

    // Shutdown
    db->close();
    central.close();
}

} /* namespace Simulator */
