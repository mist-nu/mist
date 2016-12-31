/*
 * PrivateUserAccount.cpp
 *
 *  Created on: 18 Sep 2016
 *      Author: mattias
 */

#include <sstream>

#include "JSONstream.h"
#include "PrivateUserAccount.h"

namespace Mist {

inline void throwFormatException() {
    throw PrivateUserAccountException( "Unknown serialized format." );
}

static void assertStrEq( const std::string& str1, const std::string& str2 ) {
    if ( 0 != str1.compare( str2 ) ) {
        throwFormatException();
    }
}

static void assertObjStart( JSON::Deserialize& d ) {
    d.read_istream();
    if ( !d.is_complete() && d.is_object() ) {
        d.pop();
    } else {
        throwFormatException();
    }
}

static void assertObjEnd( JSON::Deserialize& d ) {
    d.read_istream();
    if ( d.is_complete() && d.is_object() ) {
        d.pop();
    } else {
        throwFormatException();
    }
}

static std::string readNextString( JSON::Deserialize& d ) {
    while ( d.read_istream() && !d.is_complete() ) {

    }

    if ( d.is_complete() && d.is_string() ) {
        return d.get_string();
    }

    throwFormatException();
    return "";
}

PrivateUserAccount::PrivateUserAccount( const std::string& name,
        const CryptoHelper::PrivateKey& privKey ) :
            UserAccount( name, privKey.getPublicKey(), privKey.getPublicKey().hash(), Permission::P::admin ), privateKey( privKey ) {

}

PrivateUserAccount::~PrivateUserAccount() {
    // TODO Auto-generated destructor stub
}

std::string PrivateUserAccount::toString() const {
    std::stringstream ss{};
    JSON::Serialize s{};
    s.set_ostream( ss );

    s.start_object();
        s.put( "name" );
        s.put( this->name );

        s.put( "privateKey" );
        s.put( "" );
	// s.put( this->privateKey.toString() );
    s.close_object();

    return ss.str();
}

PrivateUserAccount PrivateUserAccount::deserialize( const std::string& serialized ) {
    std::stringstream ss{ serialized };
    JSON::Deserialize d{};
    d.set_istream( ss );

    assertObjStart( d );

    assertStrEq( readNextString( d ), "name" );
    std::string name{ readNextString( d ) };
    assertStrEq( readNextString( d ), "privateKey" );
    std::string privateKey{ readNextString( d ) };

    assertObjEnd( d );

    return PrivateUserAccount( name, CryptoHelper::PrivateKey() );
    //return PrivateUserAccount( name, CryptoHelper::PrivateKey( privateKey ) );
}

} /* namespace Mist */
