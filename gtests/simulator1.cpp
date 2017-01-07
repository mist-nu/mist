/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "simulator.hpp"

namespace Simulator
{

const std::string thisCentral{ FS::path( "central1" ).string() };

TEST( Mist, GenerateCentral ) {
    LOG( INFO ) << "Creating Central 1 and extracting user";
    removeCentral( thisCentral );

    // Create central
    M::Central central( thisCentral, true );
    central.create();
    central.init();

    // Extract user pem
    std::ofstream userFs( thisCentral + ".user.pem", std::fstream::binary | std::fstream::trunc );
    userFs << central.getPublicKey().toString();
    userFs.close();

    // Shutdown
    central.close();
}

} /* namespace Simulator */
