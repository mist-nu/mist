/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <functional>
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

namespace
{

namespace M = Mist;
namespace FS = M::Helper::filesystem;

using namespace std::placeholders;

using T = M::Transaction;
using RT = M::RemoteTransaction;
using D = M::Database;
using V = D::Value;
using AD = D::AccessDomain;
using ORef = D::ObjectRef;

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

const std::string central_remote{ FS::path( "central_remote" ).string() };
const std::string central_local{ FS::path( "central_local" ).string() };

TEST( LocalCentral, ReceiveTransactions ) {
    // If present, central from previous round.
    removeCentral( central_local );

    // Init central
    M::Central central( central_local );
    central.create();
    central.init();

    // Read "remote" database manifest
    std::fstream mfs( central_remote + ".manifest.json", std::fstream::in | std::fstream::binary );
    std::string mstr((std::istreambuf_iterator<char>(mfs)), std::istreambuf_iterator<char>());
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

} // namespace
