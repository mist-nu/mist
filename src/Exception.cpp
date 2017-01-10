/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include "Exception.h"

namespace Mist {

Exception::Exception( char const* errorMessage ) :
        std::runtime_error( errorMessage ), errorCode( -1 ), errorMessage( errorMessage ) {
}

Exception::Exception( const std::string& errorMessage ) :
        std::runtime_error( errorMessage ), errorCode( -1 ), errorMessage( errorMessage ) {
}

Exception::Exception( const std::string& errorMessage, const Mist::Error::ErrorCode& mistErrorCode ) :
        std::runtime_error( errorMessage ), errorCode( (int) mistErrorCode ), errorMessage( errorMessage ) {
}

Exception::Exception( const Mist::Error::ErrorCode& mistErrorCode ) :
        std::runtime_error( std::to_string( (int) mistErrorCode ) ), errorCode( (int) mistErrorCode ), errorMessage( "MistErrorCode: " + std::to_string( (int) mistErrorCode ) ) {

}

Exception::~Exception() {
    // TODO Auto-generated destructor stub
}

/*
 const char* Exception::what() const throw () {
 return errorMessage;
 }
 //*/

} /* namespace Mist */
