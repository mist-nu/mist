/*
 * Central.cpp
 *
 *  Created on: 18 Sep 2016
 *      Author: mattias
 */

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

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

namespace
{

// class h2_ostream : public std::enable_shared_from_this<h2_istream>,
//                    public std::basic_streambuf<char>
// {
// private:

//     std::mutex mux;
//     std::condition_variable read_cond;
//     std::condition_variable write_cond;
//     bool eof;
//     bool running;
//     bool exited;

//     char* bufData;
//     std::size_t bufLength;

// public:

//     h2_istream() : eof(false), running(true), exited(false),
// 	           bufData(nullptr), bufLength(0) {}

//     class guard
//     {
//     public:
// 	guard(std::shared_ptr<h2_istream> strm) : strm(strm) {}
// 	~guard() { strm->exit(); }
//     private:
// 	std::shared_ptr<h2_istream> strm;
//     };
//     friend class guard;

//     guard make_guard() { return guard(shared_from_this()); }

// protected:

//     void exit() {
// 	std::unique_lock<std::mutex> lock(mux);
// 	running = false;
// 	exited = true;
// 	write_cond.notify_all();
//     }

//     virtual int_type underflow() override {
// 	std::unique_lock<std::mutex> lock(mux);
// 	/* In case of a re-read after EOF, check again here
// 	   TODO: Find out if we can skip this... */
// 	if (eof)
// 	    return traits_type::eof();

// 	running = false;
// 	write_cond.notify_all();
//         while (!running) {
// 	    read_cond.wait(lock);
//         }

// 	if (eof) {
// 	    return traits_type::eof();
// 	} else {
// 	    setg(bufData, bufData, bufData + bufLength);
// 	    return traits_type::to_int_type(bufData[0]);
// 	}
//     }

//     std::ptrdiff_t onData(std::uint8_t* data, std::size_t length, std::uint32_t* flags) {
// 	std::unique_lock<std::mutex> lock(mux);
// 	/* In case of an unconsumed stream, we get data
// 	   after the reader has finished */
// 	if (exited)
// 	    return;

// 	bufData = const_cast<char*>
// 	    (reinterpret_cast<const char*>(data));
// 	bufLength = length;
// 	if (!data) {
// 	    eof = true;
// 	}

// 	running = true;
// 	read_cond.notify_all();
// 	while (running) {
// 	    write_cond.wait(lock);
// 	}
//     }
// };

// class h2_istream : public std::enable_shared_from_this<h2_istream>,
//                    public std::basic_streambuf<char>
// {
// private:

//     std::mutex mux;
//     std::condition_variable read_cond;
//     std::condition_variable write_cond;
//     bool eof;
//     bool running;
//     bool exited;

//     char* bufData;
//     std::size_t bufLength;

// public:

//     h2_istream() : eof(false), running(true), exited(false),
// 	           bufData(nullptr), bufLength(0) {}

//     class guard
//     {
//     public:
// 	guard(std::shared_ptr<h2_istream> strm) : strm(strm) {}
// 	~guard() { strm->exit(); }
//     private:
// 	std::shared_ptr<h2_istream> strm;
//     };
//     friend class guard;

//     guard make_guard() { return guard(shared_from_this()); }

// protected:

//     void exit() {
// 	std::unique_lock<std::mutex> lock(mux);
// 	running = false;
// 	exited = true;
// 	write_cond.notify_all();
//     }

//     virtual int_type underflow() override {
// 	std::unique_lock<std::mutex> lock(mux);
// 	/* In case of a re-read after EOF, check again here
// 	   TODO: Find out if we can skip this... */
// 	if (eof)
// 	    return traits_type::eof();

// 	running = false;
// 	write_cond.notify_all();
//         while (!running) {
// 	    read_cond.wait(lock);
//         }

// 	if (eof) {
// 	    return traits_type::eof();
// 	} else {
// 	    setg(bufData, bufData, bufData + bufLength);
// 	    return traits_type::to_int_type(bufData[0]);
// 	}
//     }

//     void onRead(const std::uint8_t* data, std::size_t length) {
// 	std::unique_lock<std::mutex> lock(mux);
// 	/* In case of an unconsumed stream, we get data
// 	   after the reader has finished */
// 	if (exited)
// 	    return;

// 	bufData = const_cast<char*>
// 	    (reinterpret_cast<const char*>(data));
// 	bufLength = length;
// 	if (!data) {
// 	    eof = true;
// 	}

// 	running = true;
// 	read_cond.notify_all();
// 	while (running) {
// 	    write_cond.wait(lock);
// 	}
//     }
// };

// class client_response_stream : public h2_istream {
// public:
//     mist::h2::ClientResponse res;
//     client_response_stream(mist::h2::ClientResponse res) : res(res) {
// 	using namespace std::placeholders;
// 	res.setOnData(std::bind(&client_response_stream::onRead, this, _1, _2));
//     }
// };

// class server_request_stream : public h2_istream {
// public:
//     mist::h2::ServerRequest req;
//     server_request_stream(mist::h2::ServerRequest req) : req(req) {
// 	using namespace std::placeholders;
// 	req.setOnData(std::bind(&server_request_stream::onRead, this, _1, _2));
//     }
// };

void putAllData(mist::h2::ServerResponse response,
    std::string buffer) {
    std::size_t i = 0;
    response.setOnRead([response, buffer, i](std::uint8_t* data,
        std::size_t length,
        std::uint32_t* flags) mutable -> std::ptrdiff_t
    {
        std::size_t n = std::min(length, buffer.length() - i);
        if (n == 0) {
            response.end();
            return 0;
        } else {
            const std::uint8_t* begin = reinterpret_cast<const std::uint8_t*>(buffer.data());
            const std::uint8_t* end = begin + n;
            std::copy(begin, end, data);
            return static_cast<std::ptrdiff_t>(n);
        }
    });
}

void putAllData(mist::h2::ClientRequest request,
    std::string buffer) {
    std::size_t i = 0;
    request.setOnRead([request, buffer, i](std::uint8_t* data,
        std::size_t length,
        std::uint32_t* flags) mutable -> std::ptrdiff_t
    {
        std::size_t n = std::min(length, buffer.length() - i);
        if (n == 0) {
            request.end();
            return 0;
        } else {
            const std::uint8_t* begin = reinterpret_cast<const std::uint8_t*>(buffer.data());
            const std::uint8_t* end = begin + n;
            std::copy(begin, end, data);
            return static_cast<std::ptrdiff_t>(n);
        }
    });
}

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
    putAllData(req, os.str());
}

void execOutStream(mist::io::IOContext& ioCtx,
    mist::h2::ServerResponse res,
    std::function<void(std::streambuf&)> fn) {
    std::ostringstream os;
    fn(*os.rdbuf());
    putAllData(res, os.str());
}

} // namespace

Mist::Central::Central( std::string path ) :
        path( path ), contentDatabase( nullptr ), settingsDatabase( nullptr ),
        databases { },
        ioCtx(), sslCtx( ioCtx, path ),
        connCtx( sslCtx, std::bind(&Mist::Central::authenticatePeer, this, std::placeholders::_1) ),
        dbService( connCtx.newService("mistDb") ),
        privKey( *this ) {

    using namespace std::placeholders;

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

void Mist::Central::init( boost::optional<std::string> privKey ) {
    if ( !dbExists( path + "/settings.db" ) || !dbExists( path + "/content.db" ) ) {
        // TODO: does not exist
      throw;
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
        settingsDatabase->exec( "CREATE TABLE UserServicePermission (userKeyHash TEXT, service TEXT, path TEXT)" );
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
    mist::io::port_range_list controlPort)
{
    connCtx.startServeTor(torIncomingPort,
        torOutgoingPort, controlPort,
        torPath, path);
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
        Mist::Database* db = new Mist::Database(this, path + "/" + std::to_string(localId) + ".db");
        db->init();
        databases.insert(std::make_pair(localId, db));
        return db;
    }
}

Mist::Database* Mist::Central::createDatabase( std::string name ) {
    // TODO: verify that we have initiated this object first.
    Mist::Database::Manifest* manifest = new Mist::Database::Manifest( this, name, Helper::Date::now(), getPublicKey() ); // TODO: figure out where this should get deleted
    manifest->sign();
    //manifest->sign( new Mist::CryptoHelper::PrivateKey() ); // TODO: fix this! ( memory leak )
    Helper::Database::Transaction transaction( *settingsDatabase );
    Helper::Database::Statement query( *settingsDatabase, "SELECT IFNULL(MAX(localId),0)+1 AS localId FROM Database" );
    try {
        if ( query.executeStep() ) {
            unsigned localId = query.getColumn( "localId" );
            // TODO: check if the file '${localID}.db' exists.
            // TODO: fix this memory leak
            Mist::Database* db = new Mist::Database( this, path + "/" + std::to_string( localId ) + ".db" );
            db->create( localId, manifest );
            Helper::Database::Statement query( *settingsDatabase, "INSERT INTO Database (hash, localId, creator, name, manifest) VALUES (?, ?, ?, ?, ?)" );
            query.bind( 1, manifest->getHash().toString() );
            query.bind( 2, localId );
            query.bind( 3, manifest->getCreator().hash().toString() );
            query.bind( 4, name );
            query.bind( 5, manifest->toString() );
            query.exec();
            transaction.commit();
            //*
            // TODO: The following lines can be discarded in favor for something else
            db->close();
            delete db;
            db = new Mist::Database( this, path + "/" + std::to_string( localId ) + ".db" );
            db->init();
            // TODO: add db to this->databases map
            //*/
            return db;
        } else {
            // TODO: handle this
        }
    } catch ( Helper::Database::Exception &e ) {
        // TODO: handle this.
    }
    return nullptr; // TODO: make sure this does not happen?
}

Mist::Database* Mist::Central::receiveDatabase( const Mist::Database::Manifest& manifest ) {
    return nullptr;
}

std::vector<Mist::Database::Manifest> Mist::Central::listDatabases() {

    std::vector<Mist::Database::Manifest> manifests;

    Helper::Database::Statement query(*settingsDatabase,
        "SELECT hash, localId, creator, name, manifest FROM Database");

    while (query.executeStep()) {
        try {
            /* name */
            std::string name(query.getColumn("name").getString());

            /* manifest */
            Helper::Date created;
            CryptoHelper::PublicKey creator;
            CryptoHelper::Signature signature;
            CryptoHelper::SHA3 hash;
            {
                //Mist::NormalizedJSON json;
                //json.parse(query.getColumn("manifest").getString());
                /* TODO: Make use of the manifest */
            }

            manifests.emplace_back( this, name, created, creator, signature, hash);
        } catch (Helper::Database::Exception& e) {
            // TODO: column error on manifest
            continue;
        }
    }

    return manifests;
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
                throw;
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
            throw;
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
        throw;
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
            throw;
    }
    peer.anonymous = static_cast<bool>(query.getColumn("anonymous").getInt() != 0);
    return peer;
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
Mist::Central::addServicePermission( const CryptoHelper::PublicKeyHash& keyHash, const std::string& service, const std::string& path ) {
    Helper::Database::Transaction transaction(*settingsDatabase);
    Helper::Database::Statement query(*settingsDatabase,
        "INSERT INTO UserServicePermission (userKeyHash, service, path) VALUES (?,?,?)");
    query.bind(1, keyHash.toString());
    query.bind(2, service);
    query.bind(3, path);
    query.exec();
    transaction.commit();
}

void
Mist::Central::removeServicePermission( const CryptoHelper::PublicKeyHash& keyHash, const std::string& service, const std::string& path ) {
    Helper::Database::Transaction transaction(*settingsDatabase);
    Helper::Database::Statement query(*settingsDatabase,
        "DELETE FROM UserServicePermission"
        " WHERE userKeyHash=? AND service=? AND path=?");
    query.bind(1, keyHash.toString());
    query.bind(2, service);
    query.bind(3, path);
    query.exec();
    transaction.commit();
}

std::map<std::string, std::vector<std::string>>
Mist::Central::listServicePermissions( const CryptoHelper::PublicKeyHash& keyHash ) {
    Helper::Database::Statement query(*settingsDatabase,
        "SELECT service, path"
        " FROM UserServicePermission WHERE userKeyHash=?");
    query.bind(1, keyHash.toString());
    std::map<std::string, std::vector<std::string>> permissions;
    while (query.executeStep()) {
        auto service = query.getColumn("service").getString();
        auto path = query.getColumn("path").getString();
        auto it = permissions.find(service);
        if (it == permissions.end())
            it = permissions.emplace(std::make_pair(service, std::vector<std::string>())).first;
        auto& paths = it->second;
        paths.push_back(path);
    }
    return permissions;
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
    Helper::Database::Statement query(*settingsDatabase, "INSERT INTO AddressLookupServer (address, port) VALUES (?, ?)");
    query.bind(1, address);
    query.bind(2, port);
    query.exec();
    transaction.commit();

    connCtx.onionAddress([=](const std::string& onionAddress)
    {
        auto addr(mist::io::Address::fromAny(address, port));
        connCtx.submitRequest(addr, "POST", "/peer",
            [=](boost::optional<mist::h2::ClientRequest> request)
        {
            if (request) {
                std::string sss(
                    R"("[{"address":"")" + onionAddress + "\""
                    + R"(","port":443)"
                    + R"(,"type":"onion"")"
                    + R"("}])");
                putAllData(*request, sss);
            }
            request->setOnResponse([](mist::h2::ClientResponse response) {
                // TODO: Figure out error states here
            });
        });
    });
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
    std::lock_guard<std::recursive_mutex> lock(sync.mux);
    sync.started = true;
    sync.forceAnonymous = forceAnonymous;
}

void Mist::Central::syncStep() {
    std::lock_guard<std::recursive_mutex> lock(sync.mux);
    ioCtx.setTimeout(1000, std::bind(&Central::syncStep, this));
    if (!sync.started)
        return;

    for (auto peer : listPeers()) {
        getPeerSyncState(peer.id).startSync();
    }
}

void Mist::Central::stopSync() {
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
        queryAddressServers();
    }
}

void
Mist::Central::PeerSyncState::stopSync()
{
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

void
Mist::Central::PeerSyncState::queryTransactions()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    state = State::QueryTransactions;

    databaseHashes = central.listDatabasePermissions( keyHash );
    databaseHashesIterator = databaseHashes.begin();
    queryTransactionsNext();
}

void
Mist::Central::PeerSyncState::queryTransactionsNext()
{
    // Find if the peer has any transactions we are missing
    std::lock_guard<std::recursive_mutex> lock(mux);
    if (databaseHashesIterator == databaseHashes.end()) {
        queryTransactionsDone();
    } else {
        auto hash= *databaseHashesIterator;

        currentDatabase = central.getDatabase( hash );
        if (!currentDatabase) {
            // TODO Log error
            databaseHashesIterator = std::next(databaseHashesIterator);
            queryTransactionsNext();
        }

        transactionsToDownload.clear();
        transactionParentsToDownload.clear();
        transactionToDownloadInOrder.clear();

        std::basic_stringbuf<char> sb;
        currentDatabase->readTransactionMetadataLastest( sb );
        central.dbService.submitRequest(peer, "GET", "/transactions/" + hash.toString() + "/?from=" + sb.str(),
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
                    central.dbService.submitRequest(peer, "GET", "/transactions/" + hash.toString() + "/latest",
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
                                }
                                if (!transactionParentsToDownload.empty() || !transactionsToDownload.empty()) {
                                    queryTransactionsGetNextParent();
                                } else {
                                    // Nothing to do
                                    databaseHashesIterator = std::next(databaseHashesIterator);
                                    queryTransactionsNext();
                                }
                            } else {
                                throw;
                            }
                        });
                    });
                } else {
                    innerGetJsonResponse(response,
                        //[=](std::unique_ptr<JSON::basic_json_value> value)
                        [=](boost::optional<const JSON::Value&> value)
                    {
                        assert(value);
                        if (value->is_array()) {
                            // Add to transactionToDownloadInOrder
                        } else {
                            throw;
                        }
                        if (transactionToDownloadInOrder.empty()) {
                            databaseHashesIterator = std::next(databaseHashesIterator);
                            queryTransactionsNext();
                        } else {
                            queryTransactionsDownloadNextTransaction(transactionToDownloadInOrder.begin());
                        }
                    });
                }
            });
        });
    }
}

void
Mist::Central::PeerSyncState::queryTransactionsGetNextParent()
{
    if (transactionParentsToDownload.empty()) {
        auto trHash = *(transactionParentsToDownload.begin());

        transactionParentsToDownload.erase( trHash );
        central.dbService.submitRequest(peer, "HEAD", "/transactions/" + currentDatabase->getManifest()->getHash().toString() + "/" + trHash,
            [=](mist::Peer& peer, mist::h2::ClientRequest request)
        {
            getJsonResponse(request,
                //[=](std::unique_ptr<JSON::basic_json_value> value)
                [=](boost::optional<const JSON::Value&> value)
            {
                // If a transaction parent transaction does not exist, add it to transactions to download and transactionParentsToDownload
                // If a transaction parent transaction does not exist, add it to transactionParentsToDownload
                queryTransactionsGetNextParent();
            });
        });
    } else {
        // Search transactionsToDownload for the oldest transaction
        std::string trHash;
        central.dbService.submitRequest(peer, "GET", "/transactions/" + currentDatabase->getManifest()->getHash().toString() + "/?from=[" + trHash + "]",
            [=](mist::Peer& peer, mist::h2::ClientRequest request)
        {
            getJsonResponse(request,
                //[=](std::unique_ptr<JSON::basic_json_value> value)
                [=](boost::optional<const JSON::Value&> value)
            {
                // If transaction does not exist add it to transactionToDownloadInOrder

                queryTransactionsGetNextParent();
            });
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
        auto hash= *it;

        central.dbService.submitRequest(peer, "GET", "/transactions/" + currentDatabase->getManifest()->getHash().toString() + "/" + hash,
            [=](mist::Peer& peer, mist::h2::ClientRequest request)
        {
            // INSERT transaction into currentDatabase
            queryTransactionsDownloadNextTransaction(std::next(it));
        });
    }
}

void
Mist::Central::PeerSyncState::queryTransactionsDone()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
}

void
Mist::Central::PeerSyncState::queryAddressServers()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    state = State::QueryAddressServers;
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
        auto path = "/peer/" + pubKey.md5Fingerprint();
        std::cerr << addressServer.first << " " << addressServer.second << std::endl;
        central.connCtx.submitRequest(
            mist::io::Address::fromAny(addressServer.first, addressServer.second),
            "GET", path, [=](boost::optional<mist::h2::ClientRequest> request) mutable
        {
            if (request) {
                getJsonResponse(*request, [this, it, request]
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
                                        throw;
                                    }
                                    // TODO: if (type == "???")
                                    addresses.emplace_back(address.get_string(), port.get_integer());
                                }
                            }
                        }
                    }
                    queryAddressServersNext(std::next(it));
                });
                request->end();
            } else {
                queryAddressServersNext(std::next(it));
            };
        });

    }
}

void
Mist::Central::PeerSyncState::queryAddressServersDone()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    connectTor();
}

void
Mist::Central::PeerSyncState::connectDirect()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    state = State::ConnectDirect;
}

void
Mist::Central::PeerSyncState::connectDirectDone()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    queryTransactions();
}

void
Mist::Central::PeerSyncState::connectTor()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    state = State::ConnectTor;
    for (auto addr : addresses)
        peer.addAddress(mist::PeerAddress{ addr.first, addr.second });
    central.connCtx.connectPeerTor(peer);
}

void
Mist::Central::PeerSyncState::connectTorDone()
{
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
        if (state == State::ConnectTor)
            connectTorDone();
        else if (state == State::ConnectDirect)
            connectDirectDone();
    } else {
        onDisconnect();
    }
}

void
Mist::Central::PeerSyncState::listServices(
    Mist::Central::peer_service_list_callback callback) {
    std::lock_guard<std::recursive_mutex> lock(mux);

    central.dbService.submitRequest(peer, "GET", "/services/",
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
                        throw;
                    }
                }
            } else {
                throw;
            }
            callback(keyHash, services);
        });
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
          keyHash(CryptoHelper::PublicKey::fromDer(peer.derPublicKey()).hash()),
          request(request) {
}

void Mist::Central::RestRequest::serve( Mist::Central& central,
        mist::Peer& peer, mist::h2::ServerRequest request, const std::string& path ) {
    RestRequest(central, peer, request).entry(path);
}

void Mist::Central::RestRequest::entry( const std::string& path ) {
    std::vector<std::string> elts;
    boost::split(elts, path, boost::is_any_of("/"));
    auto eltCount(elts.size());

    if (!request.method()) {
        replyBadMethod();
    } else if (eltCount == 0) {
        replyBadRequest();
    } else if (elts[0] == "transactions") {
        transactions(*request.method(), elts);
    } else if (elts[0] == "databases") {
        databases(elts);
    } else if (elts[0] == "users") {
        users(elts);
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
            }
            res.push_back( CryptoHelper::SHA3( from.substr( last, from.length() ) ) );
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
    if (central.hasDatabasePermission(keyHash, dbHash)) {
		auto anchor(shared_from_this());
        auto db(central.getDatabase(dbHash));

        if (db == nullptr) {
            replyNotFound();
            return;
        }
	    request.stream().submitResponse(200, {});
	    execOutStream(central.ioCtx, request.stream().response(), [this, anchor, db, fromTrHashes](std::streambuf& os) {
            std::vector<std::string> from;

            for (CryptoHelper::SHA3 trHash : fromTrHashes) {
                from.push_back( trHash.toString() );
            }
	        db->readTransactionMetadataFrom(os, from);
        });
    } else {
	    replyNotAuthorized();
    }
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
//            databaseInvite(CryptoHelper::SHA3(elts[1]));
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
        JSON::Serialize serializer;
        std::ostream os( &sb );

        serializer.set_ostream( os );
        serializer.start_array();
        for(CryptoHelper::SHA3 dbHash : central.listDatabasePermissions( keyHash )) {
            serializer.put( dbHash.toString() );
        }
        serializer.close_array();
    });
}

void Mist::Central::RestRequest::databaseInvite( const Mist::Database::Manifest &manifesth ) {
    // TODO
    // Call callback set in startSync
}

void Mist::Central::RestRequest::users( const std::vector<std::string>& elts ) {
    auto eltCount(elts.size());
    auto method(*request.method());

    if (eltCount == 1) {
        if (method == "GET") {
            // GET /users
            // Fetch all users
            usersAll();
        } else {
            replyBadMethod();
        }
    } else if (eltCount == 2) {
        if (method == "GET") {
            auto q(elts[1]);
            auto qSize(q.size());
            if (qSize == 0) {
                replyBadRequest();
            } else if (boost::starts_with(q, "?with=")) {
                // GET /users/?from=[SHA3, SHA3, SHA3]
                // Fetch changed users from a specific state of the database
                std::vector<std::string> hashes;
                std::string params(q.substr(6));
                boost::split(hashes, params, boost::is_any_of(","));
                if (hashes.size() == 3) {
                    usersChanged(CryptoHelper::SHA3(hashes[0]),
                        CryptoHelper::SHA3(hashes[1]),
                        CryptoHelper::SHA3(hashes[2]));
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

void Mist::Central::RestRequest::usersAll() {
    // TODO
}

void Mist::Central::RestRequest::usersChanged( const CryptoHelper::SHA3& dbHash,
    const CryptoHelper::SHA3& fromTrHash,
    const CryptoHelper::SHA3& toTrHash ) {
    // TODO
}

void Mist::Central::RestRequest::userInfo( const CryptoHelper::PublicKeyHash& user ) {
    // TODO
}

void Mist::Central::RestRequest::replyBadRequest() {
    // 400 Bad request
    request.stream().submitResponse(400, {});
}

void Mist::Central::RestRequest::replyBadMethod() {
    // 403 Bad method
    request.stream().submitResponse(403, {});
}

void Mist::Central::RestRequest::replyNotFound() {
    // 404 Not found
    request.stream().submitResponse(404, {});
}

void Mist::Central::RestRequest::replyNotAuthorized() {
    // ??? Not authorized
    // TODO:
    request.stream().submitResponse(400, {});
}
