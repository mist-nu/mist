/*
 * Helper.cpp
 *
 *  Created on: 18 Sep 2016
 *      Author: mattias
 */

#include "Helper.h"

namespace Mist {
namespace Helper {

namespace Database {

SavePoint::SavePoint( Database* connection, const std::string& name ) :
        name( name ), released( false ), connection( connection ) {
    if ( nullptr == connection ) {
        throw std::runtime_error( "Null pointer given to SavePoint!" );
    }
    connection->exec( "SAVEPOINT " + name );
}

SavePoint::~SavePoint() {
    rollback();
}

void SavePoint::save() {
    if ( !released ) {
        try {
            connection->exec( "RELEASE SAVEPOINT " + name ); // Same as commit for a regular transaction begin
            released = true;
        } catch ( Exception& e ) {
            LOG( WARNING ) << "Release savepoint failed: " << e.what();
            throw e;
        } catch (...) {
            LOG( WARNING ) << "Release savepoint failed.";
            throw;
        }
    }
}

void SavePoint::rollback() {
    if ( !released ) {
        try {
            connection->exec( "ROLLBACK TO SAVEPOINT " + name );
        } catch ( Exception& e ) {
            LOG( WARNING ) << "Rollback to savepoint failed: " << e.what();
            throw e;
        } catch (...) {
            LOG( WARNING ) << "Rollback to savepoint failed.";
            throw;
        }
    }
}

} /* namespace Database */

std::string Date::now( int diffSeconds ) {
    std::time_t now_c;
    std::time( &now_c );
    now_c += diffSeconds;
    char time_c[100];
    std::size_t len = std::strftime( time_c, sizeof( time_c ), "%F %T", std::gmtime( &now_c ) );
    return std::string( time_c, len );
}

} /* namespace Helper */
} /* namespace Mist */
