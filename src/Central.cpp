/*
 * (c) 2016 VISIARC AB
 *
 * Free software licensed under GPLv3.
 */

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <memory>

#include <g3log/g3log.hpp>

#include "CryptoHelper.h"
#include "Central.h"
#include "JSONstream.h"

#include "crypto/key.hpp"
#include "io/address.hpp"
#include "io/socket.hpp"
#include "conn/conn.hpp"
#include "h2/client_request.hpp"
#include "h2/client_response.hpp"
#include "h2/client_stream.hpp"
#include "h2/server_request.hpp"
#include "h2/server_response.hpp"
#include "h2/server_stream.hpp"
#include "h2/session.hpp"

namespace
{

void getAllData(mist::h2::ServerRequest request,
    std::function<void(std::string)> cb) {
    std::shared_ptr<std::string> buf
        = std::make_shared<std::string>();
    request.setOnData([cb, buf](const std::uint8_t* data,
        std::size_t length)
    {
        if (!data)
            cb(std::move(*buf));
        else
            *buf += std::string(reinterpret_cast<const char*>(data), length);
    });
}

void getAllData(mist::h2::ClientResponse response,
    std::function<void(std::string)> cb)
{
    std::shared_ptr<std::string> buf
        = std::make_shared<std::string>();
    response.setOnData(
        [cb, buf](const std::uint8_t* data, std::size_t length)
    {
        if (!data)
            cb(std::move(*buf));
        else
            *buf += std::string(reinterpret_cast<const char*>(data), length);
    });
}

void execInStream(mist::io::IOContext& ioCtx,
    mist::h2::ServerRequest req,
    std::function<void(std::streambuf&)> fn) {
    getAllData(req, [=](std::string data)
    {
        std::istringstream is(data);
        fn(*is.rdbuf());
    });
    //ioCtx.queueJob([strm, fn] {
    //    auto streamGuard(strm->make_guard());
    //	fn();
    //});
}

void execInStream(mist::io::IOContext& ioCtx,
    mist::h2::ClientResponse res,
    std::function<void(std::streambuf&)> fn) {
    getAllData(res, [=](std::string data)
    {
        std::istringstream is(data);
        fn(*is.rdbuf());
    });
}

void execOutStream(mist::io::IOContext& ioCtx,
    mist::h2::ClientRequest req,
    std::function<void(std::streambuf&)> fn) {
    std::ostringstream os;
    fn(*os.rdbuf());
    auto result = os.str();
    req.end(result);
}

void execOutStream(mist::io::IOContext& ioCtx,
    mist::h2::ServerResponse res,
    std::function<void(std::streambuf&)> fn) {
    std::ostringstream os;
    fn(*os.rdbuf());
    auto result = os.str();
    res.end(result);
}

std::unique_ptr<g3::LogWorker> logWorker;
std::unique_ptr<g3::SinkHandle<g3::FileSink>> logHandle;

void
initializeLogging(const std::string& directory, const std::string& prefix)
{
    logWorker = g3::LogWorker::createLogWorker();
    logHandle = logWorker->addDefaultLogger(prefix, directory);
    g3::initializeLogging(logWorker.get());
}

} // namespace

Mist::Central::Central( std::string path, bool loggerAlreadyInitialized ) :
        path( path ), contentDatabase( nullptr ), settingsDatabase( nullptr ),
        databases { },
        ioCtx(), sslCtx( ioCtx, path ),
        connCtx( sslCtx, std::bind(&Mist::Central::authenticatePeer, this, std::placeholders::_1) ),
        dbService( connCtx.newService("mistDb") ),
        privKey( *this ) {

    using namespace std::placeholders;

    // Initialize logger
    if (!loggerAlreadyInitialized) {
        initializeLogging(path, "central");
    }

    // Initialize sync
    sync.started = false;
    sync.forceAnonymous = false;

    dbService.setOnPeerConnectionStatus(
        std::bind(&Mist::Central::onPeerConnectionStatus, this, _1, _2));
    dbService.setOnPeerRequest(
        std::bind(&Mist::Central::peerRestRequest, this, _1, _2, _3));

    // Start the sync step timeout
    ioCtx.setTimeout(1000, std::bind(&Central::syncStep, this));
}

void Mist::Central::startEventLoop()
{
    // Start the IOContext event loop
    ioCtx.queueJob([=]() { ioCtx.exec(); });
}

namespace
{
char nibbleToHexadecimal(int c) {
    return c < 10 ? '0' + c : 'a' + c - 10;
}
std::string hex(const std::vector<std::uint8_t> buf)
{
    std::string out{};
    for (std::uint8_t c : buf) {
        out += nibbleToHexadecimal(c >> 4);
        out += nibbleToHexadecimal(c & 7);
    }
    return out;
}
}

void Mist::Central::init( boost::optional<std::string> privKey ) {
    if ( !dbExists( path + "/settings.db" ) || !dbExists( path + "/content.db" ) ) {
        // TODO: does not exist
        throw std::runtime_error("Database does not exist");
    }

    try {
        settingsDatabase.reset( new Helper::Database::Database( path + "/settings.db", Helper::Database::OPEN_READWRITE ) );
        contentDatabase.reset( new Helper::Database::Database( path + "/content.db", Helper::Database::OPEN_READWRITE ) );
    } catch ( Helper::Database::Exception &e ) {
        // TODO: could not open db
    }

    if ( !privKey ) {
      //        try {
            Helper::Database::Statement query( *settingsDatabase, "SELECT value FROM Setting WHERE key=?" );
            query.bind( 1, "userAccount" );
            assert( query.executeStep() );
	    auto encodedKey(query.getColumn( "value" ).getString());
	    auto key(mist::crypto::base64Decode(encodedKey));
	    privKey = std::string(reinterpret_cast<const char*>(key.data()), key.size());
	    //        } catch ( Helper::Database::Exception &e ) {
            // TODO: handle this.
	    //        }
    }
    sslCtx.loadPKCS12(*privKey, "");
    LOG(DBUG) << "My fingerprint is " << getPublicKey().fingerprint();
}

void Mist::Central::create( boost::optional<std::string> privKey ) {
    bool externalUserAccount = static_cast<bool>(privKey);
    if (!privKey) {
        privKey = mist::crypto::createP12TestRsaPrivateKeyAndCert(sslCtx, "", "CN=mist", 2048, mist::crypto::ValidityTimeUnit::Year, 100);
    }

    if ( dbExists( path + "/settings.db" ) || dbExists( path + "/content.db" ) ) {
        // TODO: check for .tmp as well, and handle things.
        return;
    }

    Mist::Helper::filesystem::create_directory(path);
    Mist::Helper::filesystem::create_directory(path + "/.tmp");

    try {
        settingsDatabase.reset( new Helper::Database::Database( path + "/.tmp/settings.db", Helper::Database::OPEN_CREATE | Helper::Database::OPEN_READWRITE ) );
        contentDatabase.reset( new Helper::Database::Database( path + "/.tmp/content.db", Helper::Database::OPEN_CREATE | Helper::Database::OPEN_READWRITE ) );
    } catch ( Helper::Database::Exception &e ) {
        // TODO: could not open db, handle it.
    }

    try {
        int rc = settingsDatabase->exec( "PRAGMA journal_mode=WAL;" );
        if ( rc == Helper::Database::OK ) {
            settingsDatabase.reset();
            settingsDatabase.reset( new Helper::Database::Database( path + "/.tmp/settings.db", Helper::Database::OPEN_READWRITE ) );

        } else {
            // TODO: could not set WAL-mode
        }
    } catch ( Helper::Database::Exception &e ) {
        // TODO: could not query db
    }

    try {
        int rc = contentDatabase->exec( "PRAGMA journal_mode=WAL;" );
        if ( rc == Helper::Database::OK ) {
            contentDatabase.reset();
            contentDatabase.reset( new Helper::Database::Database( path + "/.tmp/content.db", Helper::Database::OPEN_READWRITE ) );
        } else {
            // TODO: could not set WAL-mode.
        }
    } catch ( Helper::Database::Exception &e ) {
        // TODO: could not query db
    }

    try {
        Helper::Database::Transaction settingsTransaction( *settingsDatabase );
        settingsDatabase->exec( "CREATE TABLE Setting (key TEXT PRIMARY KEY, value TEXT)" );
        settingsDatabase->exec( "CREATE TABLE Database (hash TEXT PRIMARY KEY, localId INTEGER, creator INTEGER, name TEXT, manifest TEXT)" );
        settingsDatabase->exec( "CREATE TABLE User (keyHash TEXT PRIMARY KEY, publicKey TEXT, name TEXT, status TEXT, anonymous INTEGER)" );
        settingsDatabase->exec( "CREATE TABLE UserDatabase (userKeyHash TEXT, dbHash TEXT)");
        settingsDatabase->exec( "CREATE TABLE UserServicePermission (userKeyHash TEXT, service TEXT)" );
        settingsDatabase->exec( "CREATE TABLE AddressLookupServer (address TEXT, port INTEGER )" );
        if ( !externalUserAccount ) {
            auto keyData(reinterpret_cast<const std::uint8_t*>(privKey->data()));
            std::vector<std::uint8_t> key(keyData, keyData + privKey->length());
            Helper::Database::Statement query( *settingsDatabase, "INSERT INTO Setting (key, value) VALUES (?, ?)" );
            query.bind( 1, "userAccount" );
            query.bind( 2, mist::crypto::base64Encode(key) );
            query.exec();
        }
        settingsTransaction.commit();
        settingsDatabase.reset();

        Helper::Database::Transaction contentTransaction( *contentDatabase );
        contentDatabase->exec( "CREATE TABLE Content (globalId INTEGER, hash TEXT PRIMARY KEY, value BLOB)" );
        contentTransaction.commit();
        contentDatabase.reset();
    } catch ( Helper::Database::Exception &e ) {
        // TODO: handle this
    }

    // TODO: rename db.
    Mist::Helper::filesystem::rename(path + "/.tmp/settings.db", path + "/settings.db");
    Mist::Helper::filesystem::rename(path + "/.tmp/content.db", path + "/content.db");
    Mist::Helper::filesystem::remove(path + "/.tmp");
}

void Mist::Central::close() {
    // TODO: what do we want to close?
    for ( auto it = databases.begin(); it != databases.end(); ++it ) {
        it->second->close();
        delete it->second;
    }
    databases.clear();
}

void Mist::Central::startServeTor(std::string torPath,
    mist::io::port_range_list torIncomingPort,
    mist::io::port_range_list torOutgoingPort,
    mist::io::port_range_list controlPort,
    std::function<void()> startCb,
    std::function<void(boost::system::error_code)> exitCb)
{
    connCtx.startServeTor(torIncomingPort,
        torOutgoingPort, controlPort,
        torPath, path, startCb, exitCb);
}

void Mist::Central::startServeDirect(
    mist::io::port_range_list directIncomingPort)
{
    connCtx.serveDirect(directIncomingPort);
}

Mist::CryptoHelper::PublicKey Mist::Central::getPublicKey() const {
  return Mist::CryptoHelper::PublicKey::fromDer(sslCtx.derPublicKey());
}

Mist::CryptoHelper::PrivateKey Mist::Central::getPrivateKey() const {
  return privKey;
}

Mist::CryptoHelper::Signature
Mist::Central::sign(const CryptoHelper::SHA3& hash) const {
  auto buf(sslCtx.sign(hash.data(), hash.size()));
  return Mist::CryptoHelper::Signature::fromBuffer(buf);
}

bool
Mist::Central::verify(const CryptoHelper::PublicKey& key,
        const CryptoHelper::SHA3& hash,
        const CryptoHelper::Signature& sig) const {
    return sslCtx.verify(key.toDer(), hash.data(), hash.size(),
        sig.data(), sig.size());
}

Mist::Database* Mist::Central::getDatabase( CryptoHelper::SHA3 hash ) {
    Helper::Database::Statement query(*settingsDatabase, "SELECT hash, localId, creator, name, manifest FROM Database WHERE hash=?");
    query.bind(1, hash.toString());
    if (query.executeStep()) {
        try {
            return getDatabase( query.getColumn( "localId" ).getUInt() );
        } catch (Helper::Database::Exception &e) {
            // TODO: could not get "localId" column from settingsDb
            return nullptr;
        }
    } else {
        // TODO: got no rows, handle this.
        return nullptr;
    }
}

Mist::Database* Mist::Central::getDatabase( unsigned localId ) {
    auto iter = databases.find(localId);
    if (iter != databases.end()) { // found in databases
        return iter->second;
    } else {
        Helper::Database::Statement query(*settingsDatabase, "SELECT hash FROM Database WHERE localId=?");
        query.bind(1, localId);

        if (query.executeStep()) {
            try {
                Mist::Database* db = new Mist::Database(this, path + "/" + std::to_string(localId) + ".db");
                db->init( std::unique_ptr<Database::Manifest>(new Database::Manifest(getDatabaseManifest( CryptoHelper::SHA3::fromString( query.getColumn( "hash" ).getString() ) ))) );
                databases.insert(std::make_pair(localId, db));
                return db;
            } catch (Helper::Database::Exception &e) {
                // TODO: could not get "localId" column from settingsDb
                return nullptr;
            }
        } else {
            // TODO: got no rows, handle this.
            return nullptr;
        }
    }
}

Mist::Database* Mist::Central::createDatabase( std::string name ) {
    Helper::Database::Transaction transaction( *settingsDatabase );
    Helper::Database::Statement query( *settingsDatabase, "SELECT IFNULL(MAX(localId),0)+1 AS localId FROM Database" );
    try {
        if ( query.executeStep() ) {
            // Create local id
            unsigned localId = query.getColumn( "localId" );
            // TODO: check if the file '${localID}.db' exists.

            // Create manifest
            using namespace std::placeholders;
            std::unique_ptr<Database::Manifest> manifest( new Database::Manifest(
                    std::bind( &Central::sign, this, _1 ),
                    std::bind( &Central::verify, this, _1, _2, _3),
                    name, Helper::Date::now(), getPublicKey() ) );
            manifest->sign();

            // Add database to central settings
            this->databases.emplace(localId, new Mist::Database( this, path + "/" + std::to_string( localId ) + ".db" ) );
            Mist::Database* db = databases.at( localId );
            Helper::Database::Statement query( *settingsDatabase, "INSERT INTO Database (hash, localId, creator, name, manifest) VALUES (?, ?, ?, ?, ?)" );
            query.bind( 1, manifest->getHash().toString() );
            query.bind( 2, localId );
            query.bind( 3, manifest->getCreator().hash().toString() );
            query.bind( 4, name );
            query.bind( 5, manifest->toString() );
            query.exec();
            transaction.commit();

            // Add creator permission
            addDatabasePermission( manifest->getCreator().hash(), manifest->getHash() );

            db->create( localId, std::move( manifest ) );

            return db;
        } else {
            // TODO: handle this
        }
    } catch ( Helper::Database::Exception &e ) {
        // TODO: handle this.
    }
    return nullptr; // TODO: make sure this does not happen?
}

Mist::Database* Mist::Central::receiveDatabase( const Mist::Database::Manifest& receivedManifest ) {
    if( !verify( receivedManifest.getCreator(),
            receivedManifest.getHash(),
            receivedManifest.getSignature() ) ) {
        // TODO: some sort of error
        throw std::runtime_error( "Could not receive database, manifest failed verification." );
    }

    Helper::Database::Transaction transaction( *settingsDatabase );
    Helper::Database::Statement query( *settingsDatabase, "SELECT IFNULL(MAX(localId),0)+1 AS localId FROM Database" );
    try {
        if ( query.executeStep() ) {
            // Create local id
            unsigned localId = query.getColumn( "localId" );
            // TODO: check if the file '${localID}.db' exists.

            // Create a copy of the manifest
            using namespace std::placeholders;
            std::unique_ptr<Database::Manifest> manifest( new Database::Manifest(
                    nullptr,
                    std::bind( &Central::verify, this, _1, _2, _3),
                    receivedManifest.getName(),
                    receivedManifest.getCreated(),
                    receivedManifest.getCreator(),
                    receivedManifest.getSignature(),
                    receivedManifest.getHash() ) );
            if( !manifest->verify() ) {
                throw std::runtime_error( "Could not copy manifest, manifest failed verification." );
            }

            // Add database to settings
            this->databases.emplace(localId, new Mist::Database( this, path + "/" + std::to_string( localId ) + ".db" ) );
            Mist::Database* db = databases.at( localId );
            Helper::Database::Statement query( *settingsDatabase, "INSERT INTO Database (hash, localId, creator, name, manifest) VALUES (?, ?, ?, ?, ?)" );
            query.bind( 1, manifest->getHash().toString() );
            query.bind( 2, localId );
            query.bind( 3, manifest->getCreator().hash().toString() );
            query.bind( 4, manifest->getName() );
            query.bind( 5, manifest->toString() );
            query.exec();
            transaction.commit();

            // Add creator permission
            addDatabasePermission( manifest->getCreator().hash(), manifest->getHash() );

            db->create( localId, std::move( manifest ) );

            return db;
        } else {
            // TODO: handle this
        }
    } catch ( Helper::Database::Exception &e ) {
        // TODO: handle this.
    }
    return nullptr; // TODO: make sure this does not happen?
}

std::vector<Mist::Database::Manifest> Mist::Central::listDatabases() {

    std::vector<Mist::Database::Manifest> manifests;

    Helper::Database::Statement query(*settingsDatabase,
        "SELECT hash, localId, creator, name, manifest FROM Database");

    while (query.executeStep()) {
        try {
            /* name */
            std::string name(query.getColumn("name").getString());

            using namespace std::placeholders;
            manifests.push_back(
                    Database::Manifest::fromString( query.getColumn( "manifest" ).getString(),
                            std::bind( &Central::verify, this, _1, _2, _3)
            ) );
        } catch (Helper::Database::Exception& e) {
            // TODO: column error on manifest
            continue;
        }
    }

    return manifests;
}

Mist::Database::Manifest Mist::Central::getDatabaseManifest( const CryptoHelper::SHA3& hash) {
    std::vector<Mist::Database::Manifest> manifests;

    Helper::Database::Statement query(*settingsDatabase,
        "SELECT hash, localId, creator, name, manifest FROM Database WHERE hash=?");
    query.bind( 1, hash.toString() );
    if (query.executeStep()) {
            using namespace std::placeholders;
            return Database::Manifest::fromString( query.getColumn( "manifest" ).getString(),
                            std::bind( &Central::verify, this, _1, _2, _3) );
    }
    throw std::runtime_error("Could not find database");
}

Mist::Central::~Central() {
    close();
    // TODO: clean-up this->databases
}

bool Mist::Central::dbExists( std::string filename ) {
    bool exists = true;
    try {
        Helper::Database::Database( filename, Helper::Database::OPEN_READONLY );
    } catch ( Helper::Database::Exception &e ) {
        // TODO: check for other errors as well.
        exists = false;
    }
    return exists;
}

void Mist::Central::addPeer( const Mist::CryptoHelper::PublicKey& key,
    const std::string& name, Mist::PeerStatus status, bool anonymous) {
    try {
        Helper::Database::Transaction transaction(*settingsDatabase);
        Helper::Database::Statement query(*settingsDatabase,
            "INSERT INTO User (keyHash, publicKey, name, status, anonymous) VALUES (?,?,?,?,?)");
        query.bind(1, key.hash().toString());
        query.bind(2, key.toString());
        query.bind(3, name);
        std::string statusString;
        {
            if (status == Mist::PeerStatus::Direct)
                statusString = "Direct";
            else if (status == Mist::PeerStatus::DirectAnonymous)
                statusString = "DirectAnonymous";
            else if (status == Mist::PeerStatus::Indirect)
                statusString = "Indirect";
            else if (status == Mist::PeerStatus::IndirectAnonymous)
                statusString = "IndirectAnonymous";
            else if (status == Mist::PeerStatus::Blocked)
                statusString = "Blocked";
            else
                throw std::runtime_error("Invalid PeerStatus value");
        }
        query.bind(4, statusString);
        query.bind(5, anonymous ? 1 : 0);
        query.exec();
        transaction.commit();
    } catch(Helper::Database::Exception& e) {
        // TODO:
    }
}

void Mist::Central::changePeer( const Mist::CryptoHelper::PublicKeyHash& keyHash,
    const std::string& name, Mist::PeerStatus status, bool anonymous) {
    Helper::Database::Transaction transaction(*settingsDatabase);
    Helper::Database::Statement query(*settingsDatabase,
        "UPDATE User SET name=?, status=?, anonymous=? WHERE keyHash=?");
    query.bind(1, name);
    std::string statusString;
    {
        if (status == Mist::PeerStatus::Direct)
            statusString = "Direct";
        else if (status == Mist::PeerStatus::DirectAnonymous)
            statusString = "DirectAnonymous";
        else if (status == Mist::PeerStatus::Indirect)
            statusString = "Indirect";
        else if (status == Mist::PeerStatus::IndirectAnonymous)
            statusString = "IndirectAnonymous";
        else if (status == Mist::PeerStatus::Blocked)
            statusString = "Blocked";
        else
            throw std::runtime_error("Invalid PeerStatus value");
    }
    query.bind(2, statusString);
    query.bind(3, anonymous ? 1 : 0);
    query.bind(4, keyHash.toString());
    query.exec();
    transaction.commit();
}

void Mist::Central::removePeer( const Mist::CryptoHelper::PublicKeyHash& keyHash ) {
    Helper::Database::Transaction transaction(*settingsDatabase);
    Helper::Database::Statement query(*settingsDatabase,
        "DELETE FROM User WHERE keyHash=?");
    query.bind(1, keyHash.toString());
    query.exec();
    transaction.commit();
}

std::vector<Mist::Peer> Mist::Central::listPeers() const {
    Helper::Database::Statement query(*settingsDatabase,
        "SELECT keyHash FROM User");
    std::vector<Mist::Peer> peerVector;
    while (query.executeStep()) {
        auto keyHash(Mist::CryptoHelper::PublicKeyHash::fromString(query.getColumn("keyHash").getString()));
        peerVector.push_back(getPeer(keyHash));
    }
    return peerVector;
}

Mist::Peer Mist::Central::getPeer( const Mist::CryptoHelper::PublicKeyHash& keyHash ) const {
    Helper::Database::Statement query(*settingsDatabase,
        "SELECT keyHash, publicKey, name, status, anonymous"
        " FROM User WHERE keyHash=?");
    query.bind(1, keyHash.toString());
    if (!query.executeStep())
        throw std::runtime_error("Unable to find peer");
    Mist::Peer peer;
    peer.id = keyHash;
    peer.key = Mist::CryptoHelper::PublicKey::fromPem(query.getColumn("publicKey").getString());
    peer.name = query.getColumn("name").getString();
    {
        std::string status(query.getColumn("status").getString());
        if (status == "Direct")
            peer.status = Mist::PeerStatus::Direct;
        else if (status == "DirectAnonymous")
            peer.status = Mist::PeerStatus::DirectAnonymous;
        else if (status == "Indirect")
            peer.status = Mist::PeerStatus::Indirect;
        else if (status == "IndirectAnonymous")
            peer.status = Mist::PeerStatus::IndirectAnonymous;
        else if (status == "Blocked")
            peer.status = Mist::PeerStatus::Blocked;
        else
            throw std::runtime_error("Invalid PeerStatus value");
    }
    peer.anonymous = static_cast<bool>(query.getColumn("anonymous").getInt() != 0);
    return peer;
}

std::vector<Mist::Database::Manifest>
Mist::Central::getPendingInvites() const
{
  std::set<Database::Manifest> manifestSet;
  for (auto& peerPendingInvites : pendingInvites) {
    manifestSet.insert(peerPendingInvites.second.first);
  }
  return std::vector<Database::Manifest>(manifestSet.begin(),
    manifestSet.end());
}

void
Mist::Central::addDatabasePermission( const CryptoHelper::PublicKeyHash& keyHash, const CryptoHelper::SHA3& dbHash ) {
    Helper::Database::Transaction transaction(*settingsDatabase);
    Helper::Database::Statement query(*settingsDatabase,
        "INSERT INTO UserDatabase (userKeyHash, dbHash) VALUES (?,?)");
    query.bind(1, keyHash.toString());
    query.bind(2, dbHash.toString());
    query.exec();
    transaction.commit();
}

void
Mist::Central::removeDatabasePermission( const CryptoHelper::PublicKeyHash& keyHash, const CryptoHelper::SHA3& dbHash ) {
    Database::Manifest manifest{ getDatabaseManifest( dbHash ) };
    if ( keyHash == manifest.getCreator().hash() ) {
        LOG( DBUG ) << "Can not remove the creators database permission.";
        throw std::runtime_error( "Can not remove the creators database permission." );
    }

    Helper::Database::Transaction transaction(*settingsDatabase);
    Helper::Database::Statement query(*settingsDatabase,
        "DELETE FROM UserDatabase"
        " WHERE userKeyHash=? AND dbHash=?");
    query.bind(1, keyHash.toString());
    query.bind(2, dbHash.toString());
    query.exec();
    transaction.commit();
}

std::vector<Mist::CryptoHelper::SHA3>
Mist::Central::listDatabasePermissions( const CryptoHelper::PublicKeyHash& keyHash ) {
    Helper::Database::Statement query(*settingsDatabase,
        "SELECT dbHash"
        " FROM UserDatabase WHERE userKeyHash=?");
    query.bind(1, keyHash.toString());
    std::vector<CryptoHelper::SHA3> dbHashes;
    while (query.executeStep()) {
        auto dbHash = query.getColumn("dbHash").getString();
	dbHashes.push_back(CryptoHelper::SHA3(dbHash));
    }
    return dbHashes;
}

bool
Mist::Central::hasDatabasePermission( const CryptoHelper::PublicKeyHash& keyHash, const CryptoHelper::SHA3& dbHash ) {
    Helper::Database::Statement query(*settingsDatabase,
        "SELECT dbHash"
        " FROM UserDatabase WHERE userKeyHash=? AND dbHash=?");
    query.bind(1, keyHash.toString());
    query.bind(2, dbHash.toString());
    return static_cast<bool>(query.executeStep());
}

void
Mist::Central::addServicePermission( const CryptoHelper::PublicKeyHash& keyHash,
        const std::string& service ) {
    Helper::Database::Transaction transaction(*settingsDatabase);
    Helper::Database::Statement query(*settingsDatabase,
        "INSERT INTO UserServicePermission (userKeyHash, service) VALUES (?,?)");
    query.bind(1, keyHash.toString());
    query.bind(2, service);
    query.exec();
    transaction.commit();
}

void
Mist::Central::removeServicePermission( const CryptoHelper::PublicKeyHash& keyHash,
        const std::string& service ) {
    Helper::Database::Transaction transaction(*settingsDatabase);
    Helper::Database::Statement query(*settingsDatabase,
        "DELETE FROM UserServicePermission"
        " WHERE userKeyHash=? AND service=?");
    query.bind(1, keyHash.toString());
    query.bind(2, service);
    query.exec();
    transaction.commit();
}

std::vector<std::string>
Mist::Central::listServicePermissions( const CryptoHelper::PublicKeyHash& keyHash ) {
    Helper::Database::Statement query(*settingsDatabase,
        "SELECT service"
        " FROM UserServicePermission WHERE userKeyHash=?");
    query.bind(1, keyHash.toString());
    std::vector<std::string> services;
    while (query.executeStep()) {
        services.push_back(query.getColumn("service").getString());
    }
    return services;
}

namespace
{
void getJsonResponse(mist::h2::ClientRequest request,
        std::function<void(boost::optional<const JSON::Value&>)> cb) {
    auto response(request.stream().response());
    std::shared_ptr<std::stringstream> ss
        = std::make_shared<std::stringstream>();
    response.setOnData(
        [cb, response, ss](const std::uint8_t* data, std::size_t length)
    {
        std::uint64_t id(reinterpret_cast<std::uint64_t>(reinterpret_cast<void*>(response._impl.get())));
        if (!data) {
            auto sss(ss->str());
            if (sss.empty()) {
                cb(boost::none);
            } else {
                auto value(JSON::Deserialize::generate_json_value(sss));
                cb(value);
            }
        } else {
            ss->write(reinterpret_cast<const char*>(data), length);
        }
    });
}

void innerGetJsonResponse(mist::h2::ClientResponse response,
        std::function<void(boost::optional<const JSON::Value&>)> cb) {
    std::shared_ptr<std::stringstream> ss
        = std::make_shared<std::stringstream>();
    response.setOnData(
        [cb, ss](const std::uint8_t* data, std::size_t length)
    {
        //desr->writesome(reinterpret_cast<const char*>(data), length);
        if (!data) {
            auto sss(ss->str());
            if (sss.empty()) {
                cb(boost::none);
            } else {
                auto value(JSON::Deserialize::generate_json_value(sss));
                cb(value);
            }
        } else {
            ss->write(reinterpret_cast<const char*>(data), length);
        }
    });
}
} // namespace

mist::Service&
Mist::Central::getService(const std::string& service)
{
    std::unique_lock<std::recursive_mutex> lock(serv.mux);
    return *serv.services[service];
}

void
Mist::Central::openServiceSocket(
    const CryptoHelper::PublicKeyHash keyHash, std::string service,
    std::string path, service_open_socket_callback cb)
{
    auto& syncState = getPeerSyncState(keyHash);
    getService(service).openWebSocket(syncState.getConnPeer(), path,
        [keyHash, cb](mist::Peer&, std::string, std::shared_ptr<mist::io::Socket> socket)
    {
        cb(keyHash, socket);
    });
}

void
Mist::Central::openServiceRequest(
    const CryptoHelper::PublicKeyHash keyHash, std::string service,
    std::string method, std::string path, service_open_request_callback cb)
{
    auto& syncState = getPeerSyncState(keyHash);
    getService(service).submitRequest(syncState.getConnPeer(), method, path,
        [keyHash, cb](mist::Peer&, mist::h2::ClientRequest request)
    {
        cb(keyHash, request);
    });
}

void Mist::Central::addAddressLookupServer( const std::string& address, std::uint16_t port ) {
    // TODO:
    removeAddressLookupServer(address, port);
    Helper::Database::Transaction transaction(*settingsDatabase);
    Helper::Database::Statement query(*settingsDatabase,
        "INSERT INTO AddressLookupServer (address, port) VALUES (?, ?)");
    query.bind(1, address);
    query.bind(2, port);
    query.exec();
    transaction.commit();
}

void Mist::Central::removeAddressLookupServer( const std::string& address, std::uint16_t port ) {
    Helper::Database::Transaction transaction(*settingsDatabase);
    Helper::Database::Statement query(*settingsDatabase, "DELETE FROM AddressLookupServer WHERE address=? AND port=?");
    query.bind(1, address);
    query.bind(2, port);
    query.exec();
    transaction.commit();
}

std::vector<std::pair<std::string, std::uint16_t>>
Mist::Central::listAddressLookupServers() const {
    Helper::Database::Statement query(*settingsDatabase,
        "SELECT address, port FROM AddressLookupServer");
    std::vector<std::pair<std::string, std::uint16_t>> addressLookupServer;
    while (query.executeStep()) {
        addressLookupServer.push_back(
            std::make_pair(query.getColumn("address").getString(),
        query.getColumn("port").getInt()));
    }
    return addressLookupServer;
}

void Mist::Central::startSync(new_database_callback newDatabase, bool forceAnonymous) {
    LOG(INFO) << "Starting sync globally";
    std::lock_guard<std::recursive_mutex> lock(sync.mux);
    sync.started = true;
    sync.forceAnonymous = forceAnonymous;
    sync.newDatabase = newDatabase;
}

void Mist::Central::syncStep() {
    std::lock_guard<std::recursive_mutex> lock(sync.mux);
    ioCtx.setTimeout(10000, std::bind(&Central::syncStep, this));
    if (!sync.started)
        return;

    connCtx.onionAddress([=](const std::string& onionAddress)
    {
        for (auto server : listAddressLookupServers())
        {
            auto address(server.first);
            auto port(server.second);
            auto addr(mist::io::Address::fromAny(address, port));
            connCtx.submitRequest(addr, "POST", "/peer", address, {},
                [=](boost::optional<mist::h2::ClientRequest> request,
                    boost::system::error_code ec)
            {
                if (!ec) {
                    request->end(R"([{"address":")" + onionAddress + "\""
                        + R"(,"port":443)"
                        + R"(,"type":"tor")"
                        + R"(}])");
                    /*request->setOnResponse([request, address, port]
                    (mist::h2::ClientResponse response)
                    {
                        LOG(DBUG) << "Got response from " << address
                            << ":" << port;
                    });*/
                } else {
                    LOG(DBUG) << "Unable to connect to lookup server " << address
                        << ":" << port << ": " << ec.message();
                }
            });
        }
    });

    for (auto peer : listPeers()) {
        getPeerSyncState(peer.id).startSync();
    }
}

void Mist::Central::stopSync() {
    LOG(INFO) << "Stopping sync globally";
    std::lock_guard<std::recursive_mutex> lock(sync.mux);
    sync.started = false;
    for (auto peer : listPeers()) {
        getPeerSyncState(peer.id).stopSync();
    }
}

void Mist::Central::authenticatePeer( mist::Peer& peer ) {
    // We only allow peers we added ourselves
    peer.setAuthenticated( false );
}

/*
 * Per-peer sync state
 */

Mist::Central::PeerSyncState::PeerSyncState( Mist::Central& central,
    const CryptoHelper::PublicKey& publicKey )
    : state(State::Reset), central(central), pubKey(publicKey),
      keyHash(pubKey.hash()),
      peer(central.connCtx.addAuthenticatedPeer(pubKey.toDer())),
      anonymous(true)
{
}

void
Mist::Central::PeerSyncState::startSync()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    if (state == State::Reset || state == State::Disconnected) {
        LOG(INFO) << shortFinger() << "Starting sync";
        queryAddressServers();
    }
}

void
Mist::Central::PeerSyncState::stopSync()
{
    LOG(INFO) << shortFinger() << "Stopping sync";
    std::lock_guard<std::recursive_mutex> lock(mux);
    // TODO:
}

/*void
Mist::Central::PeerSyncState::queryDatabases()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    state = State::QueryDatabases;
    databases.clear();

    central.dbService.submitRequest(peer, "GET", "/databases/",
        [=](mist::Peer& peer, mist::h2::ClientRequest request)
    {
        getJsonResponse(request,
            //[=](std::unique_ptr<JSON::basic_json_value> value)
            [=](const JSON::Value& value)
        {
            if (value.is_array()) {
                //JSON::Array& arr = static_cast<JSON::Array&>(*value);
                const std::vector<JSON::Value>& arr = value.array;
                for (const auto& database : arr) {
                    // TODO: Deserialize Database::Manifest
                }
            } else {
                throw;
            }
            queryDatabasesDone();
        });
    });
}

void
Mist::Central::PeerSyncState::queryDatabasesDone()
{
}*/

std::string
Mist::Central::PeerSyncState::shortFinger() const
{
    return pubKey.fingerprint().substr(0, 6) + ": ";
}

void
Mist::Central::PeerSyncState::queryTransactions()
{
    LOG(INFO) << shortFinger() << "queryTransactions";
    std::lock_guard<std::recursive_mutex> lock(mux);
    state = State::QueryTransactions;

    // Try all databases in case user permissions are not up to date
    databaseHashes.clear();
    for (auto& manifest : central.listDatabases()) {
        databaseHashes.push_back(manifest.getHash());
    }

    databaseHashesIterator = databaseHashes.begin();
    queryTransactionsNext();
}

namespace
{

boost::optional<Mist::CryptoHelper::SHA3> getMetadataHash(
        const JSON::Value& transaction) {
    try {
        if (!transaction.is_object())
            return boost::none;
        auto& id = transaction.at("id");
        if (id.is_string()) {
            return Mist::CryptoHelper::SHA3(id.get_string());
        }
    } catch (std::out_of_range&) {
        // id not found in object
    }
}

std::vector<Mist::CryptoHelper::SHA3> getMetadataParents(
        const JSON::Value& transaction) {
    std::vector<Mist::CryptoHelper::SHA3> parentHashes;
    try {
        auto& parents = transaction.object.at("transaction")
            .object.at("metadata")
            .object.at("parents");
        if (parents.is_array()) {
            for (auto& parent : parents.array) {
                if (parent.is_string()) {
                    parentHashes.push_back(
                        Mist::CryptoHelper::SHA3(parent.get_string()));
                }
            }
        }
    } catch (std::out_of_range&) {
        // object key error
    }
    return parentHashes;
}

} // namespace

void
Mist::Central::PeerSyncState::queryTransactionsNext()
{
    // Find if the peer has any transactions we are missing
    std::lock_guard<std::recursive_mutex> lock(mux);
    if (databaseHashesIterator == databaseHashes.end()) {
        queryTransactionsDone();
    } else {
        auto hash = *databaseHashesIterator;

        LOG(INFO) << shortFinger() << "queryTransactionsNext " << hash.toString();
        currentDatabase = central.getDatabase( hash );
        if (!currentDatabase) {
            // TODO Log error
            LOG(INFO) << shortFinger() << "could not find database";
            databaseHashesIterator = std::next(databaseHashesIterator);
            queryTransactionsNext();
            return;
        }

        transactionsToDownload.clear();
        transactionParentsToDownload.clear();
        transactionToDownloadInOrder.clear();

        std::string requestUrl;
        auto transactionList(currentDatabase->getTransactionLatest());
        if (transactionList.empty()) {
            // First time, get all transactions
            requestUrl = "/transactions/"
                + mist::h2::urlEncode(hash.toString());
        } else {
            std::string transactionList;
            for (auto& tran : currentDatabase->getTransactionLatest()) {
                if (transactionList.length())
                    transactionList += ",";
                transactionList += mist::h2::urlEncode(tran.hash.toString());
            }
            requestUrl = "/transactions/"
                + mist::h2::urlEncode(hash.toString())
                + "/?from=" + transactionList;
        }

        LOG(INFO) << shortFinger() << "Getting transaction " << hash.toString();
        central.dbService.submitRequest(peer, "GET", requestUrl,
            [=](mist::Peer& _peer, mist::h2::ClientRequest request)
        {
            // Not found
            // Peer does not have all my latest transactions. We need to ask for its latest
            // transactions to find out if it has any transactions that we do not have

            // Get status of request befo
            request.setOnResponse(
                [=](mist::h2::ClientResponse response)
            {
                if (*response.statusCode() == 404) {
                    LOG(INFO) << shortFinger() << "Transaction not found";
                    central.dbService.submitRequest(peer, "GET",
                        "/transactions/" + mist::h2::urlEncode(hash.toString())
                        + "/latest",
                        [=](mist::Peer& peer, mist::h2::ClientRequest request)
                    {
                        getJsonResponse(request,
                            //[=](std::unique_ptr<JSON::basic_json_value> value)
                            [=](boost::optional<const JSON::Value&> value)
                        {
                            if (value->is_array()) {
                                //JSON::Array& arr = static_cast<JSON::Array&>(*value);
                                const std::vector<JSON::Value>& arr = value->array;
                                for (const auto& transaction : arr) {
                                    // If transaction exists, do nothing

                                    // If transaction does not exists, add it to transactions to download

                                    // If a transaction parent transaction does not exist, add it to transactions to download and transactionParentsToDownload

                                    auto tranHash = getMetadataHash(transaction);
                                    bool tranExists = true;
                                    if (tranHash) {
                                        try {
                                            currentDatabase->getTransactionMeta(*tranHash);
                                            LOG(INFO) << shortFinger() << "Transaction exists";
                                        } catch (std::runtime_error&) {
                                            // Transaction does not exist
                                            transactionsToDownload[tranHash->toString()] = transaction;
                                            tranExists = false;
                                        }
                                    }
                                    if (!tranExists) {
                                        LOG(INFO) << shortFinger() << "Transaction does not exist";
                                        auto parentHashes = getMetadataParents(transaction);
                                        for (auto& parentHash : parentHashes) {
                                            try {
                                                currentDatabase->getTransactionMeta(parentHash);
                                                LOG(INFO) << shortFinger() << "Parent exists: " << parentHash.toString();
                                            } catch (std::runtime_error&) {
                                                // Parent does not exist
                                                transactionParentsToDownload.insert(parentHash.toString());
                                                LOG(INFO) << shortFinger() << "Parent does not exist: " << parentHash.toString();
                                            }
                                        }
                                    }
                                }
                                if (!transactionParentsToDownload.empty() || !transactionsToDownload.empty()) {
                                    queryTransactionsGetNextParent();
                                } else {
                                    // Nothing to do
                                    databaseHashesIterator = std::next(databaseHashesIterator);
                                    queryTransactionsNext();
                                }
                            } else {
                                throw std::runtime_error("Malformed JSON response");
                            }
                        });
                    });
                } else if (*response.statusCode() == 200) {
                    LOG(INFO) << shortFinger() << "Transaction found";
                    innerGetJsonResponse(response,
                        //[=](std::unique_ptr<JSON::basic_json_value> value)
                        [=](boost::optional<const JSON::Value&> value)
                    {
                        assert(value);
                        if (value->is_array()) {
                            const std::vector<JSON::Value>& arr = value->array;
                            for (const auto& transaction : arr) {
                                auto hash = getMetadataHash(transaction);
                                if (hash) {
                                    transactionToDownloadInOrder.push_back(hash->toString());
                                }
                            }
                        } else {
                            throw std::runtime_error("Malformed JSON response");
                        }
                        if (transactionToDownloadInOrder.empty()) {
                            databaseHashesIterator = std::next(databaseHashesIterator);
                            queryTransactionsNext();
                        } else {
                            queryTransactionsDownloadNextTransaction(transactionToDownloadInOrder.begin());
                        }
                    });
                } else {
                    // If we are here it probably means that the user has not
                    // yet accepted an invite to the database.
                    LOG(INFO) << shortFinger() << "Transaction refused";
                    queryTransactionsNext();
                }
            });
            request.end();
        });
    }
}

void
Mist::Central::PeerSyncState::queryTransactionsGetNextParent()
{
    if (!transactionParentsToDownload.empty()) {
        auto trHash = *(transactionParentsToDownload.begin());

        transactionParentsToDownload.erase( trHash );

        LOG(DBUG) << shortFinger() << "queryTransactionsGetNextParent get parent " << trHash;

        central.dbService.submitRequest(peer, "HEAD", "/transactions/"
            + mist::h2::urlEncode(currentDatabase->getManifest()->getHash().toString()
            + "/" + mist::h2::urlEncode(trHash)),
            [=](mist::Peer& peer, mist::h2::ClientRequest request)
        {
            getJsonResponse(request,
                //[=](std::unique_ptr<JSON::basic_json_value> value)
                [=](boost::optional<const JSON::Value&> value)
            {
                // If a transaction parent transaction does not exist, add it to transactions to download and transactionParentsToDownload
                // If a transaction parent transaction does not exist, add it to transactionParentsToDownload
                if (value) {
                    auto tranHash = getMetadataHash(*value);
                    bool tranExists = true;
                    if (tranHash) {
                        try {
                            currentDatabase->getTransactionMeta(*tranHash);
                        } catch (std::runtime_error&) {
                            // Transaction does not exist
                            transactionsToDownload[tranHash->toString()] = *value;
                            tranExists = false;
                        }
                    }
                    if (!tranExists) {
                        auto parentHashes = getMetadataParents(*value);
                        for (auto& parentHash : parentHashes) {
                            try {
                                currentDatabase->getTransactionMeta(parentHash);
                            } catch (std::runtime_error&) {
                                // Parent does not exist
                                transactionParentsToDownload.insert(parentHash.toString());
                            }
                        }
                    }
                }
                queryTransactionsGetNextParent();
            });
            request.end();
        });
    } else {
        // Search transactionsToDownload for the oldest transaction
        std::string trHash;

        LOG(DBUG) << shortFinger() << "queryTransactionsGetNextParent download " << trHash;

        central.dbService.submitRequest(peer, "GET",
            "/transactions/" + mist::h2::urlEncode(currentDatabase->getManifest()->getHash().toString())
            + "/?from=[" + mist::h2::urlEncode(trHash) + "]",
            [=](mist::Peer& peer, mist::h2::ClientRequest request)
        {
            getJsonResponse(request,
                //[=](std::unique_ptr<JSON::basic_json_value> value)
                [=](boost::optional<const JSON::Value&> value)
            {
                // If transaction does not exist add it to transactionToDownloadInOrder
                if (value) {
                    auto tranHash = getMetadataHash(*value);
                    if (tranHash) {
                        try {
                            currentDatabase->getTransactionMeta(*tranHash);
                        } catch (std::runtime_error&) {
                            // Transaction does not exist
                            transactionToDownloadInOrder.push_back(tranHash->toString());
                        }
                    }
                }
                queryTransactionsGetNextParent();
            });
            request.end();
        });

    }
}

void
Mist::Central::PeerSyncState::queryTransactionsDownloadNextTransaction(std::vector<std::string>::iterator it)
{
    // Find if the peer has any transactions we are missing
    std::lock_guard<std::recursive_mutex> lock(mux);
    if (it == transactionToDownloadInOrder.end()) {
        databaseHashesIterator = std::next(databaseHashesIterator);
        queryTransactionsNext();
    } else {
        auto hash = *it;

        LOG(DBUG) << shortFinger() << "queryTransactionsDownloadNextTransaction " << hash;

        central.dbService.submitRequest(peer, "GET",
            "/transactions/" + mist::h2::urlEncode(currentDatabase->getManifest()->getHash().toString())
            + "/" + mist::h2::urlEncode(hash),
            [=](mist::Peer& peer, mist::h2::ClientRequest request)
        {
            // INSERT transaction into currentDatabase
            execInStream(central.ioCtx, request.stream().response(),
                [=](std::streambuf& is)
            {
                currentDatabase->writeToDatabase(is);
                queryTransactionsDownloadNextTransaction(std::next(it));
            });
            request.end();
        });
    }
}

void
Mist::Central::PeerSyncState::queryTransactionsDone()
{
    LOG(DBUG) << shortFinger() << "queryTransactionsDone";
    std::lock_guard<std::recursive_mutex> lock(mux);
    queryInvites();
}

void Mist::Central::addDatabaseInvite(Database::Manifest manifest,
    CryptoHelper::PublicKeyHash inviter)
{
    std::lock_guard<std::recursive_mutex> lock(sync.mux);
    auto it = pendingInvites.find(manifest.getHash());
    auto firstInvite = it == pendingInvites.end();
    if (firstInvite) {
        pendingInvites.insert({ manifest.getHash(),
            { manifest, {inviter} } }).first;
        sync.newDatabase(manifest);
    } else {
        it->second.second.push_back(inviter);
    }
}

void
Mist::Central::PeerSyncState::queryInvites()
{
    LOG(DBUG) << shortFinger() << "queryInvites";
    std::lock_guard<std::recursive_mutex> lock(mux);
    central.dbService.submitRequest(peer, "GET", "/databases",
        [=](mist::Peer& _peer, mist::h2::ClientRequest request) {
        request.setOnResponse(
            [=](mist::h2::ClientResponse response)
        {
            assert(*response.statusCode() == 200);
            innerGetJsonResponse(response,
                //[=](std::unique_ptr<JSON::basic_json_value> value)
                [=](boost::optional<const JSON::Value&> value)
            {
                assert(value);
                if (value->is_array()) {
                    for (auto const &v : value->array) {
                        using namespace std::placeholders;
                        Database::Manifest m = Database::Manifest::fromJSON( v, std::bind( &Central::verify, &central, _1, _2, _3 ) );

                        try {
                            central.getDatabaseManifest( m.getHash() );
                        } catch (std::runtime_error&) {
                            // Not found
                            central.addDatabaseInvite(m, keyHash);
                        }
                    }
                    // Add to transactionToDownloadInOrder
                } else {
                    throw std::runtime_error("Malformed JSON response");
                }
                queryInvitesDone();
            });
        });
    });
}

void
Mist::Central::PeerSyncState::queryInvitesDone()
{
    LOG(DBUG) << shortFinger() << "queryInvitesDone";
    std::lock_guard<std::recursive_mutex> lock(mux);
}

void
Mist::Central::PeerSyncState::queryAddressServers()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    state = State::QueryAddressServers;
    addresses.clear();
    addressServers = central.listAddressLookupServers();
    queryAddressServersNext(addressServers.begin());
}

void
Mist::Central::PeerSyncState::queryAddressServersNext(address_vector_t::iterator it)
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    if (it == addressServers.end()) {
        queryAddressServersDone();
    } else {
        auto addressServer = *it;
        auto fingerprint = pubKey.fingerprint();
        auto path = "/peer/" + mist::h2::urlEncode(fingerprint);
        LOG(DBUG) << shortFinger() << "Querying "
            << addressServer.first << ":" << addressServer.second
            << " for onion address";
        central.connCtx.submitRequest(
            mist::io::Address::fromAny(addressServer.first, addressServer.second),
            "GET", path, addressServer.first, {},
            [=](boost::optional<mist::h2::ClientRequest> request,
              boost::system::error_code ec) mutable
        {
            if (!ec) {
                LOG(DBUG) << shortFinger() << "Connected to "
                    << addressServer.first << ":" << addressServer.second;
                getJsonResponse(*request, [this, it, request, fingerprint]
                    (boost::optional<const JSON::Value&> value) mutable
                {
                    std::lock_guard<std::recursive_mutex> lock(mux);
                    if (value) {
                        if (value->is_array()) {
                            const auto& arr = value->array;
                            for (const auto& objVal : arr) {
                                if (objVal.is_object()) {
                                    const auto& type = objVal.at("type");
                                    const auto& address = objVal.at("address");
                                    const auto& port = objVal.at("port");
                                    if (!type.is_string() || !address.is_string() || !port.is_number()) {
                                        throw std::runtime_error("Malformed JSON response");
                                    }
                                    LOG(DBUG) << shortFinger() << "Got address"
                                        << ":" << address.get_string()
                                        << ":" << port.get_integer();
                                    addresses.emplace_back(address.get_string(), port.get_integer());
                                } else {
                                    LOG(DBUG) << shortFinger() << "JSON response invalid: not an object";
                                }
                            }
                        } else {
                            LOG(DBUG) << shortFinger() << "JSON response invalid: not an array";
                        }
                    } else {
                        LOG(DBUG) << shortFinger() << "JSON response invalid: not a valid JSON object";
                    }
                    queryAddressServersNext(std::next(it));
                });
                request->end();
            } else {
                LOG(DBUG) << shortFinger() << "Error while connecting to "
                    << addressServer.first << ":" << addressServer.second
                    << ": " << ec.message();
                queryAddressServersNext(std::next(it));
            };
        });
    }
}

void
Mist::Central::PeerSyncState::queryAddressServersDone()
{
    LOG(DBUG) << shortFinger() << "Done querying for onion address";
    std::lock_guard<std::recursive_mutex> lock(mux);
    connectTor();
}

void
Mist::Central::PeerSyncState::connectDirect()
{
    LOG(DBUG) << shortFinger() << "Direct connect";
    std::lock_guard<std::recursive_mutex> lock(mux);
    state = State::ConnectDirect;
    //central.connCtx.connectPeerDirect(peer, addr,
    //    [=](mist::Peer&, boost::system::error_code ec)
    //{
    //    if (!ec) {
    //        connectDirectDone();
    //    } else {
                //LOG(DBUG) << "Direct connect attempt failed for peer SHA3:" << pubKey.fingerprint()
                //    << ": " << ec.message();
    //        ...
    //    }
    //});
}

void
Mist::Central::PeerSyncState::connectDirectDone()
{
    LOG(DBUG) << shortFinger() << "Direct connect done";
    std::lock_guard<std::recursive_mutex> lock(mux);
    queryTransactions();
}

void
Mist::Central::PeerSyncState::connectTor()
{
    LOG(DBUG) << shortFinger() << "Tor connect";
    std::lock_guard<std::recursive_mutex> lock(mux);
    state = State::ConnectTor;
    connectTorNext(addresses.begin());
}

void
Mist::Central::PeerSyncState::connectTorNext(address_vector_t::iterator it)
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    if (it == addresses.end()) {
        LOG(DBUG) << shortFinger() << "Tor connect failed";
        state = State::Disconnected;
    } else {
        LOG(DBUG) << shortFinger() << "Trying tor address " << it->first << ":" << it->second;
        mist::PeerAddress addr{ it->first, it->second };
        central.connCtx.connectPeerTor(peer, addr,
            [=](mist::Peer&, boost::system::error_code ec)
        {
            if (!ec) {
                connectTorDone();
            } else {
                LOG(DBUG) << shortFinger() << "Tor connect attempt failed"
                    << ": " << ec.message();
                connectTorNext(std::next(it));
            }
        });
    }
}

void
Mist::Central::PeerSyncState::connectTorDone()
{
    LOG(DBUG) << shortFinger() << "Tor connect successful";
    std::lock_guard<std::recursive_mutex> lock(mux);
    queryTransactions();
}

void
Mist::Central::PeerSyncState::onDisconnect()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
}

void
Mist::Central::PeerSyncState::onPeerConnectionStatus(mist::Peer::ConnectionStatus status)
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    if (status == mist::Peer::ConnectionStatus::Connected) {
        /*if (state == State::ConnectTor)
            connectTorDone();
        else if (state == State::ConnectDirect)
            connectDirectDone();*/
    } else {
        onDisconnect();
    }
}

void
Mist::Central::PeerSyncState::listServices(
    Mist::Central::peer_service_list_callback callback) {
    std::lock_guard<std::recursive_mutex> lock(mux);

    central.dbService.submitRequest(peer, "GET", "/services",
        [=](mist::Peer& peer, mist::h2::ClientRequest request)
    {
        getJsonResponse(request,
            [=](boost::optional<const JSON::Value&> value)
        {
            std::vector<std::string> services;
            if (value->is_array()) {
                //JSON::Array& arr = static_cast<JSON::Array&>(*value);
                const auto& arr = value->array;
                for (const auto& service : arr) {
                    if (service.is_string()) {
                        //JSON::String& str = static_cast<JSON::String&>(*service);
                        //services.push_back(str.string);
                        services.push_back(service.get_string());
                    } else {
                        throw std::runtime_error("Malformed JSON response");
                    }
                }
            } else {
                throw std::runtime_error("Malformed JSON response");
            }
            callback(keyHash, services);
        });
        request.end();
    });
}

/*
 * Global sync state
 */
Mist::Central::PeerSyncState&
Mist::Central::getPeerSyncState( const CryptoHelper::PublicKeyHash& keyHash )
{
    std::lock_guard<std::recursive_mutex> lock(sync.mux);
    auto it = sync.peerState.find(keyHash);
    if (it == sync.peerState.end()) {
        auto peerSyncState(std::unique_ptr<PeerSyncState>(
            new PeerSyncState(*this, getPeer(keyHash).key)));
        it = sync.peerState.insert(
            std::make_pair(keyHash, std::move(peerSyncState))).first;
    }
    return *it->second;
}

void
Mist::Central::listServices(const Mist::CryptoHelper::PublicKeyHash& keyHash,
    Mist::Central::peer_service_list_callback callback)
{
    return getPeerSyncState(keyHash).listServices(callback);
}

void Mist::Central::registerService( const std::string& service, service_connection_callback cb ) {
    std::lock_guard<std::recursive_mutex> lock(serv.mux);

    auto& svc = connCtx.newService(service);
    auto it = serv.services.insert(std::make_pair(service, &svc));
    // Assert that the name was not already taken
    assert(it.second);
    svc.setOnWebSocket([cb]( mist::Peer& peer, std::string path,
            std::shared_ptr<mist::io::Socket> socket ) {
        cb(socket);
    });
}

void Mist::Central::onPeerConnectionStatus( mist::Peer& peer, mist::Peer::ConnectionStatus status ) {
    std::lock_guard<std::recursive_mutex> lock(sync.mux);

    auto keyHash(CryptoHelper::PublicKey::fromDer(peer.derPublicKey()).hash());
    getPeerSyncState(keyHash).onPeerConnectionStatus(status);
}

void Mist::Central::registerHttpService( const std::string& service,
        http_service_connection_callback cb ) {
    std::lock_guard<std::recursive_mutex> lock(serv.mux);

    auto& svc = connCtx.newService(service);
    auto it = serv.services.insert(std::make_pair(service, &svc));
    // Assert that the name was not already taken
    assert(it.second);
    svc.setOnPeerRequest([cb]( mist::Peer& peer,
            mist::h2::ServerRequest request, std::string path ) {
        cb(request);
    });
}

void Mist::Central::peerRestRequest( mist::Peer& peer,
    mist::h2::ServerRequest request, std::string path ) {

    ioCtx.queueJob([this, &peer, request, path]() {
        RestRequest::serve(*this, peer, request, path);
    });
}

/* RestRequest */

Mist::Central::RestRequest::RestRequest( Central& central, mist::Peer& peer,
        mist::h2::ServerRequest request )
        : central(central),
          peer(peer),
          pubKey(CryptoHelper::PublicKey::fromDer(peer.derPublicKey())),
          keyHash(pubKey.hash()),
          request(request) {
}

void Mist::Central::RestRequest::serve( Mist::Central& central,
        mist::Peer& peer, mist::h2::ServerRequest request, const std::string& path ) {
    std::make_shared<RestRequest>(central, peer, request)->entry(path);
}

std::string
Mist::Central::RestRequest::shortFinger() const
{
    return pubKey.fingerprint().substr(0, 6) + ": ";
}

void Mist::Central::RestRequest::entry( const std::string& path ) {
    LOG(DBUG) << shortFinger() << "Serving REST request for " << path;
    std::vector<std::string> elts;
    boost::split(elts, path, boost::is_any_of("/"));
    auto eltCount(elts.size());

    for (auto& elt : elts) {
        elt = mist::h2::urlDecode(elt);
    }

    // Expect a slash prefix and a non-zero path
    if (eltCount < 2 || elts[0] != "" || !request.method()) {
        replyBadMethod();
        return;
    }

    elts.erase(elts.begin());

    if (elts[0] == "transactions") {
        transactions(*request.method(), elts);
    } else if (elts[0] == "databases") {
        databases(elts);
    } else if (elts[0] == "users") {
        users(elts);
    } else if (elts[0] == "services") {
        services(elts);
    } else {
        replyNotFound();
    }
}

void Mist::Central::RestRequest::transactions(
        const std::string &method,
        const std::vector<std::string>& elts ) {
    auto eltCount = elts.size();

    if (eltCount <= 1) {
        replyBadRequest();
        return;
    }
    CryptoHelper::SHA3 dbHash( elts[1] );
    if (method == "HEAD") {
        if (eltCount == 3) {
            CryptoHelper::SHA3 trHash( elts[2] );
            transactionMetadata( dbHash, trHash );
        } else {
            replyBadRequest();
        }
    } else if (method == "GET") {
        if (eltCount == 2) {
            transactionsAll( dbHash );
        } else if (elts[2] == "latest") {
            transactionsLatest( dbHash );
        } else if (boost::starts_with( elts[2], "?from=" )) {
            std::string from = elts[2].substr( 6 );
            int last = 0, pos = 0;
            std::vector<CryptoHelper::SHA3> res;

            pos = from.find( ",", last );
            while (pos != std::string::npos) {
                res.push_back( CryptoHelper::SHA3( from.substr( last, pos-last ) ) );
                last = pos+1;
                pos = from.find( ",", last );
            }
            res.push_back( CryptoHelper::SHA3( from.substr( last, from.length()-last ) ) );
            transactionsFrom( dbHash, res );
        } else {
            CryptoHelper::SHA3 trHash( elts[2] );
            transaction( dbHash, trHash );
        }
    } else if (method == "POST") {
        if (eltCount == 3) {
            if (elts[2] == "new") {
                // TODO Read transaction metadata from JSON body
                transactionsNew( dbHash );
            }
        } else {
            replyBadRequest();
        }
    } else {
        replyBadMethod();
    }
}

void Mist::Central::RestRequest::transaction(
        const CryptoHelper::SHA3& dbHash,
        const CryptoHelper::SHA3& trHash ) {
    if (central.hasDatabasePermission(keyHash, dbHash)) {
        auto anchor(shared_from_this());
        auto db(central.getDatabase(dbHash));

        if (db == nullptr) {
            replyNotFound();
            return;
        }
        request.stream().submitResponse(200, {});
        execOutStream(central.ioCtx, request.stream().response(), [this, anchor, db, trHash](std::streambuf& os) {
            db->readTransaction(os, trHash.toString());
        });
    } else {
        replyNotAuthorized();
    }
}

void Mist::Central::RestRequest::transactionMetadata(
        const CryptoHelper::SHA3& dbHash,
        const CryptoHelper::SHA3& trHash ) {
    if (central.hasDatabasePermission(keyHash, dbHash)) {
        auto anchor(shared_from_this());
        auto db(central.getDatabase(dbHash));

        if (db == nullptr) {
            replyNotFound();
            return;
        }
        request.stream().submitResponse(200, {});
        execOutStream(central.ioCtx, request.stream().response(), [this, anchor, db, trHash](std::streambuf& os) {
            db->readTransactionMetadata(os, trHash.toString());
        });
    } else {
        replyNotAuthorized();
    }
}

void Mist::Central::RestRequest::transactionsAll(
        const CryptoHelper::SHA3& dbHash ) {
    if (central.hasDatabasePermission(keyHash, dbHash)) {
        auto anchor(shared_from_this());
        auto db(central.getDatabase(dbHash));

        if (db == nullptr) {
            replyNotFound();
            return;
        }
        request.stream().submitResponse(200, {});
        execOutStream(central.ioCtx, request.stream().response(), [this, anchor, db](std::streambuf& os) {
            db->readTransactionList(os);
        });
    } else {
        replyNotAuthorized();
    }
}

void Mist::Central::RestRequest::transactionsLatest(
        const CryptoHelper::SHA3& dbHash ) {
    if (central.hasDatabasePermission(keyHash, dbHash)) {
		auto anchor(shared_from_this());
        auto db(central.getDatabase(dbHash));

        if (db == nullptr) {
            replyNotFound();
            return;
        }
	    request.stream().submitResponse(200, {});
	    execOutStream(central.ioCtx, request.stream().response(), [this, anchor, db](std::streambuf& os) {
	        db->readTransactionMetadataLastest(os);
        });
    } else {
	    replyNotAuthorized();
    }
}

void Mist::Central::RestRequest::transactionsFrom(
        const CryptoHelper::SHA3& dbHash,
        const std::vector<CryptoHelper::SHA3>& fromTrHashes ) {

    auto db(central.getDatabase(dbHash));
    if (db == nullptr) {
        replyNotFound();
        return;
    }

    auto user(db->getUser(keyHash.toString()));
    if (!user) {
        replyNotAuthorized();
        return;
    }

    auto perm(user->getPermission());
    if (perm != Permission::P::admin && perm != Permission::P::read) {
        replyNotAuthorized();
        return;
    }

    auto anchor(shared_from_this());
    request.stream().submitResponse(200, {});
    execOutStream(central.ioCtx, request.stream().response(),
            [this, anchor, db, fromTrHashes](std::streambuf& os) {
        if (fromTrHashes.empty()) {
            db->readTransactionList(os);
        } else {
            std::vector<std::string> from;

            for (CryptoHelper::SHA3 trHash : fromTrHashes) {
                from.push_back(trHash.toString());
            }
            db->readTransactionMetadataFrom(os, from);
        }
    });
}

void Mist::Central::RestRequest::transactionsNew(
        const CryptoHelper::SHA3& dbHash ) {
    // TODO Start syncing the transaction if we need the transaction
}

void Mist::Central::RestRequest::databases( const std::vector<std::string>& elts ) {
    auto eltCount(elts.size());
    auto method(*request.method());

    if (eltCount == 1) {
        if (method == "GET") {
            // GET /databases
            databasesAll();
        } else {
            replyBadMethod();
        }
    } else if (eltCount == 3 && elts[2] == "invite") {
        if (method == "POST") {
            // POST /databases/[SHA3]/invite
            // Invite a user to a new database
            auto anchor(shared_from_this());
            getAllData(request, [this, anchor](std::string data)
            {
                using namespace std::placeholders;
                auto manifest{ Database::Manifest::fromString(data,
                    std::bind(&Central::verify, &central, _1, _2, _3)) };
                central.addDatabaseInvite(manifest, keyHash);
            });
        } else {
            replyBadMethod();
        }
    } else {
        replyNotFound();
    }
}

void Mist::Central::RestRequest::databasesAll() {
    std::vector<CryptoHelper::SHA3> dbs = central.listDatabasePermissions( keyHash );

    request.stream().submitResponse(200, {});
    execOutStream( central.ioCtx, request.stream().response(), [this, dbs](std::streambuf& sb) {
        std::ostream os( &sb );
        bool first = true;

        os << '[';
        for(CryptoHelper::SHA3 dbHash : central.listDatabasePermissions( keyHash )) {
            auto manifest = central.getDatabaseManifest( dbHash );
            os << manifest.toString();
            if (!first)
                os << ',';
            first = false;
        }
        os << ']';
    });
}

void Mist::Central::RestRequest::users( const std::vector<std::string>& elts ) {
    auto eltCount(elts.size());
    auto method(*request.method());

    if (eltCount == 2) {
        if (method == "GET") {
            // GET /users/[SHA3]
            // Fetch all users for a database
            auto dbHash{ CryptoHelper::SHA3(elts[1]) };
            usersAll(dbHash);
        } else {
            replyBadMethod();
        }
    } else if (eltCount == 3) {
        if (method == "GET") {
            auto dbHash{ CryptoHelper::SHA3(elts[1]) };
            auto q(elts[2]);
            auto qSize(q.size());
            if (qSize == 0) {
                replyBadRequest();
            } else if (boost::starts_with(q, "?from=")) {
                // GET /users/[SHA3]/?from=[SHA3, SHA3]
                // Fetch changed users from a specific state of the database
                std::vector<std::string> hashes;
                std::string params(q.substr(6));
                boost::split(hashes, params, boost::is_any_of(","));
                if (hashes.size() == 2) {
                    usersChanged(dbHash,
                        CryptoHelper::SHA3(hashes[0]),
                        CryptoHelper::SHA3(hashes[1]));
                } else {
                    replyBadRequest();
                }
                auto eltCount(elts.size());
            } else {
                // GET /users/[id (SHA3 public key)]
                // Fetch current information about a user
                userInfo(CryptoHelper::PublicKeyHash(q));
            }
        } else {
            replyBadMethod();
        }
    } else {
        replyNotFound();
    }
}

void Mist::Central::RestRequest::usersAll( const CryptoHelper::SHA3& dbHash ) {
    auto db(central.getDatabase(dbHash));

    if (db == nullptr) {
        replyNotFound();
        return;
    }

    // TODO:
    //db->mapUsers();
}

void Mist::Central::RestRequest::usersChanged( const CryptoHelper::SHA3& dbHash,
    const CryptoHelper::SHA3& fromTrHash,
    const CryptoHelper::SHA3& toTrHash ) {
    // TODO
}

void Mist::Central::RestRequest::userInfo( const CryptoHelper::PublicKeyHash& user ) {
    // TODO
}

void Mist::Central::RestRequest::services(
        const std::vector<std::string>& elts ) {
    auto eltCount = elts.size();

    if (eltCount > 1) {
        replyNotFound();
    } else {
        request.stream().submitResponse(200, {});
        execOutStream( central.ioCtx, request.stream().response(), [this](std::streambuf& sb) {
            std::ostream os( &sb );
            bool first = true;

            os << '[';
            for(std::string service : central.listServicePermissions( keyHash )) {
                os << service;
                if (!first)
                    os << ',';
                first = false;
            }
            os << ']';
        });
    }
}

void Mist::Central::RestRequest::replyBadRequest() {
    // 400 Bad request
    request.stream().submitResponse(400, {});
    request.stream().response().end();
}

void Mist::Central::RestRequest::replyBadMethod() {
    // 405 Method not allowed
    request.stream().submitResponse(405, {});
    request.stream().response().end();
}

void Mist::Central::RestRequest::replyNotFound() {
    // 404 Not found
    request.stream().submitResponse(404, {});
    request.stream().response().end();
}

void Mist::Central::RestRequest::replyNotAuthorized() {
    // 403 Forbidden
    request.stream().submitResponse(403, {});
    request.stream().response().end();
}
