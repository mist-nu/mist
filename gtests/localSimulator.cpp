/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "simulator.hpp"

namespace Simulator
{

const std::string central_remote{ FS::path( "central_remote" ).string() };
const std::string central_local{ FS::path( "central_local" ).string() };

TEST( LocalCentral, ReceiveTransactions ) {
    // If present, central from previous round.
    removeCentral( central_local );

    // Init central
    M::Central central( central_local, true );
    central.create();
    central.init();

    // Read "remote" database manifest
    std::fstream mfs( central_remote + ".manifest.json", std::fstream::in | std::fstream::binary );
    std::string mstr((std::istreambuf_iterator<char>(mfs)), std::istreambuf_iterator<char>());
    mfs.close();
    M::Database::Manifest manifest{
        M::Database::Manifest::fromString( mstr,
                std::bind( &M::Central::verify, &central, _1, _2, _3)
        )
    };

    M::Database* db{ central.receiveDatabase( manifest ) };

    // Read "remote" transactions
    std::fstream tfs( central_remote + ".json", std::fstream::in | std::fstream::binary );
    db->writeToDatabase( *tfs.rdbuf() );
    tfs.close();
    db->close();
    central.close();
}

} /* namespace Simulator */
