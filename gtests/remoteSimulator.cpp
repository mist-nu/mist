/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "Central.h"
#include "CryptoHelper.h"
#include "Database.h"
#include "Helper.h"
#include "JSONstream.h"
#include "Transaction.h"

#include "gtest/gtest.h"

namespace M = Mist;
namespace FS = M::Helper::filesystem;

using namespace std::placeholders;

using T = M::Transaction;
using RT = M::RemoteTransaction;
using D = M::Database;
using V = D::Value;
using AD = D::AccessDomain;
using ORef = D::ObjectRef;

namespace
{

void tryRemoveFile(std::string p) {
    FS::path filename(p);
    if (FS::exists(filename)) {
        FS::remove(filename);
    }
}

void tryRemoveDb(std::string p) {
    tryRemoveFile(p);
    tryRemoveFile(p + "-wal");
    tryRemoveFile(p + "-shm");
}

void removeCentral(std::string p) {
    LOG(INFO) << "Removing central: " << p;
    tryRemoveDb(p + "/settings.db");
    tryRemoveDb(p + "/content.db");
    tryRemoveDb(p + ".tmp/settings.db");
    tryRemoveDb(p + ".tmp/content.db");
    tryRemoveDb(p + + "/0.db" );
    tryRemoveDb(p + + "/1.db" );
    tryRemoveDb(p + + "/2.db" );
    tryRemoveDb(p + + "/3.db" );
}

const std::string fileName{ "simulatorRun" };

inline bool fileExists( const std::string& fileName ) {
    std::ifstream fs( fileName );
    bool exists{ fs.good() };
    fs.close();
    return exists;
}

bool runCheck( int i ) {
    if ( !fileExists( fileName ) ) {
        std::fstream fs( fileName, std::fstream::out | std::fstream::trunc );
        fs << ( 1 == i ? 2 : 1 );
        fs.close();
        return 1 == i;
    }
    std::ifstream ifs( fileName );
    int run{ 0 };
    ifs >> run;
    ifs.close();
    if ( i == run ) {
        std::ofstream ofs( fileName, std::fstream::trunc );
        ofs << run + 1;
        ofs.close();
        return true;
    }
    return false;
}

void resetRun() {
    std::remove( fileName.c_str() );
}

const std::string central_remote{ FS::path( "central" ).string() };

TEST( RemoteCentral, AddUser ) {
    if( runCheck( 2 ) ) {

    }
}

TEST( RemoteCentral, CreateTransactions ) {
    //if( runCheck( 1 ) ) {
        LOG( INFO ) << "Creating 'remote' transaction";
        std::string thisCentral{ central_remote + "1" };
        removeCentral( thisCentral );

        M::Central central( thisCentral, true );
        central.create();
        central.init();

        // Create db
        D* db{ central.createDatabase( "test" ) };

        // Verify manifest
        M::Database::Manifest* m{ db->getManifest() };
        EXPECT_TRUE( m->verify() );

        // Attributes
        std::map<std::string, V> attributes;

        // Object parent
        M::Database::ObjectRef object{ M::Database::AccessDomain::Normal, 0 };
        M::Database::ObjectRef object2{ object };
        M::Database::ObjectRef object3{ object };

        // Transaction pointer
        std::unique_ptr<T> t{};

        // Empty transaction
        t = std::move( db->beginTransaction() );
        t->commit();
        t.reset();

        // New object
        t = std::move( db->beginTransaction() );
        attributes.clear();
        attributes.emplace( "Hello", "world" );
        object.id = t->newObject( object, attributes );
        t->commit();
        t.reset();

        // Test value types
        t = std::move( db->beginTransaction() );
        attributes.clear();
        attributes.emplace( "Null", nullptr );
        attributes.emplace( "Boolean", true );
        attributes.emplace( "Number", 2.0 );
        attributes.emplace( "String", "text" );
        attributes.emplace( "Json", V( "{}", true ) );
        object.id = t->newObject( object, attributes );
        t->commit();
        t.reset();

        // Multiple objects
        t = std::move( db->beginTransaction() );
        attributes.clear();
        attributes.emplace( "a", "v" );
        object.id = t->newObject( object, attributes );
        object2.id = t->newObject( object, attributes );
        object3.id = t->newObject( object2, attributes );
        t->commit();
        t.reset();

        // Change objects
        t = std::move( db->beginTransaction() );
        attributes.clear();
        attributes.emplace( "changed", "object" );
        t->updateObject( object.id, attributes );
        attributes.clear();
        attributes.emplace( "2", "changed" );
        t->updateObject( object2.id, attributes );
        t->commit();
        t.reset();

        // Move object
        t = std::move( db->beginTransaction() );
        attributes.clear();
        t->moveObject( object3.id, object );
        t->commit();
        t.reset();

        // Delete object
        t = std::move( db->beginTransaction() );
        attributes.clear();
        t->deleteObject( object3.id );
        t->commit();
        t.reset();

        // Dump the whole database
        db->dump( thisCentral );

        // Shutdown
        db->close();
        central.close();
    //}
}

} // namespace
