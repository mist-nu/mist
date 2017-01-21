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

TEST( Mist, SyncTransactions ) {
    LOG( INFO ) << "\"Sync\" transactions.";

    // Create central
    M::Central central( thisCentral, true );
    central.init();

    // Open database
    M::Database* db{ central.getDatabase( 1u ) };

    // Read "remote" transactions
    std::fstream tfs( central1 + ".json", std::fstream::in | std::fstream::binary );
    db->writeToDatabase( *tfs.rdbuf() );
    tfs.close();

    // Dump the db so the other central can receive the database
    db->dump( thisCentral );

    // Shutdown
    db->close();
    central.close();
}

TEST( Mist, CompareDatabases ) {
    std::fstream db1( central1 + ".json", std::fstream::in | std::fstream::binary );
    std::fstream db2( thisCentral + ".json", std::fstream::in | std::fstream::binary );

    EXPECT_TRUE( std::equal( std::istreambuf_iterator<char>( db1.rdbuf() ),
            std::istreambuf_iterator<char>(),
            std::istreambuf_iterator<char>( db2.rdbuf() ) ) );
}

} /* namespace Simulator */
