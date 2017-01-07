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
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>( now );
    auto now_s = std::chrono::time_point_cast<std::chrono::seconds>( now );
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>( now_ms - now_s ).count();

    time_t cnow = std::chrono::system_clock::to_time_t(now);
    char buffer[100];
    std::size_t len = std::strftime( buffer, sizeof( buffer ), "%F %T", std::gmtime( &cnow ) );
    std::string time( buffer, len );
    time += '.';
    if ( ms < 10 )
        time += '0';
    if ( ms < 100 )
        time += '0';
    time += std::to_string( ms );
    return time;
}

} /* namespace Helper */
} /* namespace Mist */
