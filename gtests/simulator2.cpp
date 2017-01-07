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

    // Read user pem
    std::ifstream userFs( central1 + ".user.pem", std::fstream::binary );
    std::string userPem(static_cast<std::stringstream const&>(std::stringstream() << userFs.rdbuf()).str());
    userFs.close();

    // Load PublicKey from Pem
    M::CryptoHelper::PublicKey user1Key{ M::CryptoHelper::PublicKey::fromPem( userPem ) };

    // Create user1 base on the publickey
    M::UserAccount user1( "user1", user1Key, user1Key.hash(), { M::Permission::P::admin } );

    // Invite user1 to db2
    db->inviteUser( user1 );

    // Dump the db so central 1 can receive the database
    db->dump( thisCentral );

    // Shutdown
    db->close();
    central.close();
}

} /* namespace Simulator */
