/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

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

} // namespace

const std::string central_remote{ FS::path( "central_remote" ).string() };

TEST( RemoteCentral, CreateTransactions ) {
    LOG( INFO ) << "Creating 'remote' transaction";
    removeCentral( central_remote );

    M::Central central( central_remote );
    central.create();
    central.init();

    // Create db
    D* db{ central.createDatabase( "test" ) };

    // Test manifest
    M::Database::Manifest* m{ db->getManifest() };
    EXPECT_TRUE( m->verify() );

    // Test Manifest::fromString( toString() )
    std::string storedManifest{ db->getManifest()->toString() };
    M::Database::Manifest manifest{ M::Database::Manifest::fromString( storedManifest,
            std::bind( &M::Central::verify, &central, _1, _2, _3 )
    ) };
    EXPECT_TRUE( manifest.verify() );

    // Start transaction
    std::unique_ptr<T> t{ std::move( db->beginTransaction() ) };

    // Add Object
    std::map<std::string, V> attributes;
    attributes.emplace( "Hello", "world" );
    attributes.emplace( "Next", V( 2 ) );
    t->newObject( {}, attributes );
    t->commit();
    t.reset();

    // New transaction
    t = std::move( db->beginTransaction() );
    attributes.clear();
    attributes.emplace( "Second", "test" );
    t->newObject( {}, attributes );
    t->commit();
    t.reset();

    db->dump( central_remote );

    // Shutdown
    db->close();
    central.close();
}
