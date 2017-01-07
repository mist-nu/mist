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

namespace Simulator
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
    LOG( INFO ) << "Removing central: " << p;
    tryRemoveDb(p + "/settings.db");
    tryRemoveDb(p + "/content.db");
    tryRemoveDb(p + ".tmp/settings.db");
    tryRemoveDb(p + ".tmp/content.db");
    tryRemoveDb(p + + "/0.db" );
    tryRemoveDb(p + + "/1.db" );
    tryRemoveDb(p + + "/2.db" );
    tryRemoveDb(p + + "/3.db" );
}

} /* namespace Simulator */
