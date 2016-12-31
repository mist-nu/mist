/*
 * UserAccount.cpp
 *
 *  Created on: 18 Sep 2016
 *      Author: mattias
 */

#include <sstream>

#include "JSONstream.h"
#include "UserAccount.h"

namespace Mist {

Permission::Permission( const std::string& p ) {
    if ( "admin" == p ) {
        permission = P::admin;
    } else if ( "write" == p ) {
        permission = P::write;
    } else if ( "read" == p ) {
        permission = P::read;
    } else {
        throw UserAccountException( "Invalid permission" );
    }
}

std::string Permission::toString() const {
    switch ( permission ) {
    case P::admin:
        return "admin";
    case P::read:
        return "read";
    case P::write:
        return "write";
    default:
        throw UserAccountException();
    }
}

Permission Permission::deserialize( const std::string& serialized ) {
    if ( 0 == serialized.compare( "admin" ) ) {
        return Permission( Permission::P::admin );
    } else if ( 0 == serialized.compare( "read" ) ) {
        return Permission( Permission::P::read );
    } else if ( 0 == serialized.compare( "write" ) ) {
        return Permission( Permission::P::write );
    } else {
        throw UserAccountException();
    }
}

static void assertStrEq( const std::string& str1, const std::string& str2 ) {
    if ( 0 != str1.compare( str2 ) ) {
        throw UserAccountException();
    }
}

static void assertObjStart( JSON::Deserialize& d ) {
    d.read_istream();
    if ( !d.is_complete() && d.is_object() ) {
        d.pop();
    } else {
        throw UserAccountException();
    }
}

static void assertObjEnd( JSON::Deserialize& d ) {
    d.read_istream();
    if ( d.is_complete() && d.is_object() ) {
        d.pop();
    } else {
        throw UserAccountException();
    }
}

static std::string readNextString( JSON::Deserialize& d ) {
    while ( d.read_istream() && !d.is_complete() ) {

    }

    if ( d.is_complete() && d.is_string() ) {
        return d.get_string();
    }

    throw UserAccountException();
}

UserAccount::UserAccount( const std::string& name,
        const CryptoHelper::PublicKey& publicKey,
        const CryptoHelper::PublicKeyHash& id,
        const Permission& permission ) :
                id( id ), publicKey( publicKey ), name( name ), permission( permission ) {
    // TODO: verify hash or throw
}

UserAccount::UserAccount( std::string&& name,
            CryptoHelper::PublicKey&& publicKey,
            CryptoHelper::PublicKeyHash&& id,
            Permission&& permission ) :
                    id( std::move( id ) ),
                    publicKey( std::move( publicKey ) ),
                    name( std::move( name ) ),
                    permission( std::move( permission ) ) {
    // TODO: verify hash or throw
}

UserAccount::~UserAccount() {

}

const CryptoHelper::PublicKey& UserAccount::getPublicKey() const {
    return publicKey;
}

const CryptoHelper::PublicKeyHash& UserAccount::getPublicKeyHash() const {
    return id;
}

const std::string& UserAccount::getName() const {
    return name;
}

const Permission& UserAccount::getPermission() const {
    return permission;
}

std::string UserAccount::toString() const {
    std::stringstream ss{};
    JSON::Serialize s{};
    s.set_ostream( ss );

    s.start_object();
        s.put( "id" );
        s.put( this->id.toString() );

        s.put( "name" );
        s.put( this->name );

        s.put( "permission" );
        s.put( this->permission.toString() );

        s.put( "publicKey" );
        s.put( this->publicKey.toString() );
    s.close_object();

    return ss.str();
}

UserAccount UserAccount::deserialize( const std::string& serialized ) {
    std::stringstream ss{ serialized };
    JSON::Deserialize d{};
    d.set_istream( ss );

    assertObjStart( d );

    assertStrEq( readNextString( d ), "id" );
    std::string id{ readNextString( d ) };

    assertStrEq( readNextString( d ), "name" );
    std::string name{ readNextString( d ) };

    assertStrEq( readNextString( d ), "permission" );
    std::string permission{ readNextString( d ) };

    assertStrEq( readNextString( d ), "publicKey" );
    std::string pubKey{ readNextString( d ) };

    assertObjEnd( d );

    // TODO: Verify the public key creation
    return UserAccount( name,
            CryptoHelper::PublicKey::fromPem( pubKey ),
            CryptoHelper::PublicKeyHash( id ),
            Permission::deserialize( permission ) );
}

} /* namespace Mist */
