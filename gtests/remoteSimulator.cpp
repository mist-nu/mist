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
#include "Database.h"
#include "Helper.h"
#include "JSONstream.h"
#include "RemoteTransaction.h"
#include "Transaction.h"

#include "gtest/gtest.h"

namespace M = Mist;
namespace FS = M::Helper::filesystem;

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

const std::string transaction_file( "transactions.json" );
const std::string central_remote{ FS::path( "central_remote" ).string() };

TEST( RemoteCentral, CreateTransactions ) {
    LOG( INFO ) << "Creating 'remote' transaction";
    removeCentral( central_remote );

    M::Central central( central_remote );
    central.create();
    central.init();

    // Create db
    D* db{ central.createDatabase( "test" ) };

    // Start transaction
    std::unique_ptr<T> t{ std::move( db->beginTransaction() ) };

    // Add Object
    std::map<std::string, V> attributes;
    attributes.emplace( "Hello", V( "world" ) );
    attributes.emplace( "Next", V( 2 ) );
    t->newObject( {}, attributes );
    t->commit();
    t.reset();

    // New transaction
    t = std::move( db->beginTransaction() );
    attributes.clear();
    attributes.emplace( "Second", V( "test" ) );
    t->newObject( {}, attributes );
    t->commit();
    t.reset();

    // Get tranasaction list
    std::stringstream json{};
    db->readTransactionList( *json.rdbuf() );
    JSON::Value value{ JSON::Deserialize::generate_json_value( json.str() ) };
    std::string transaction1{ value.array.at( 0 ).object.at( "id" ).get_string() };
    std::string transaction2{ value.array.at( 1 ).object.at( "id" ).get_string() };

    // Read transaction as json to file
    std::fstream fs( transaction_file, std::fstream::out | std::fstream::trunc | std::fstream::binary );
    db->readTransaction( *fs.rdbuf(), transaction1 );
    db->readTransaction( *fs.rdbuf(), transaction2 );
    fs.close();

    // Shutdown
    db->close();
    central.close();
}

