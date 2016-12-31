/*
 * Copyright (c) 2016 VISIARC AB
 */

#ifndef SRC_CENTRAL_H_
#define SRC_CENTRAL_H_

// STL
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "CryptoHelper.h"
#include "Database.h"
#include "Helper.h"
#include "PrivateUserAccount.h"
#include "JSONstream.h"

#include "conn/conn.hpp"
#include "conn/peer.hpp"
#include "conn/service.hpp"

#include "io/io_context.hpp"
#include "io/ssl_context.hpp"
#include "io/socket.hpp"

#include "h2/server_request.hpp"

namespace Mist {

enum PeerStatus { Direct, DirectAnonymous, Indirect, IndirectAnonymous, Blocked };

struct Peer {
    CryptoHelper::PublicKeyHash id;
    CryptoHelper::PublicKey key;
    std::string name;
    PeerStatus status;
    bool anonymous;
};

/**
 * Main class of the Mist library. Instanciate this class to use the Mist library. It needs a path to
 * a directory where it will store its data. Only one Mist instance can use the same directory
 * at the same time.
 *
 * Call create to initiate necessary databases the first time a new directory is used.
 *
 * Call init to initiate the Mist library.
 *
 * To start networking call the startSync method. Mist will create a background thread to handle the network
 * connections.
 *
 * Before staring networking the first time, at least one lookup server must be added with the addAddressLookupServer
 * method.
 */
class Central {
public:
    /**
     * Path is the path to the directory where it stores its SQLite databases. If it is the first time the
     * directory is used, create must be called. Init must be called before using any other methods.
     */
    Central( std::string path );

    /**
     * Start Mist from an existing directory with databases that has already been initaited. Usually
     * the RSA key pair is stored in one of the SQLite databases. However it can also be sent as an
     * argument to the init and create methods. Must always be the same key pair.
     *
     * Note that the connection library, with network connections, does not start until the startSync
     * method is called.
     */
    virtual void init( boost::optional<std::string> privKey = boost::none );
    /**
     * Initiate a directory for use with the Mist library. Mist will create all necessary SQLite databases
     * and usually also generate a RSA key pair. However the key pair can also be supplied as an argument.
     * Then Mist library will not store the private key. The same key pair must be used to later calls to init.
     */
    virtual void create( boost::optional<std::string> privKey = boost::none );
    /**
     * Close Mist. Closes all open databases and disconnects any network sockets.
     */
    virtual void close();

    virtual void startEventLoop();

    /**
     * Get the public key of this peer.
     */
    virtual CryptoHelper::PublicKey getPublicKey() const;

    virtual CryptoHelper::PrivateKey getPrivateKey() const;

    virtual CryptoHelper::Signature sign(const CryptoHelper::SHA3& hash) const;

    /**
     * Verify a signature against a public key. Signatures are 224-bit SHA-3 hash, encrypted by a private RSA key.
     */
    virtual bool verify(const CryptoHelper::PublicKey& key, const CryptoHelper::SHA3& hash, const CryptoHelper::Signature& sig) const;

    /**
     * Get a Mist database, based on the SHA-3 hash of its manifest.
     */
    virtual Mist::Database* getDatabase( CryptoHelper::SHA3 hash );
    /**
     * Get a Mist database, based on its local id number. The local id number is just a counter incremented every time
     * we make or receive another database. Database id numbers are not the same for different peers.
     */
    virtual Mist::Database* getDatabase( unsigned localId );
    /**
     * Create a new Mist database. This peer will be the owner and have admin permission to the database.
     */
    virtual Mist::Database* createDatabase( std::string name );
    /**
     * Start downloading a database we have been given permission to.
     */
    virtual Mist::Database* receiveDatabase( const Mist::Database::Manifest &manifest );
    /**
     * List all Mist databases we have created, or replicated.
     */
    virtual std::vector<Mist::Database::Manifest> listDatabases();

    /**
     * Add a peer that we will accept connections from. Note that the peer must also add us, otherwise it will refuse
     * our connection attempts.
     *
     * Users from databases we replicate are automatically added as peers.
     */
    virtual void addPeer( const CryptoHelper::PublicKey& key, const std::string& name, Mist::PeerStatus status, bool anonymous );
    /**
     * Change the settings of a peer.
     */
    virtual void changePeer( const CryptoHelper::PublicKeyHash& keyHash, const std::string& name, Mist::PeerStatus status, bool anonymous );
    /**
     * Remove a peer.
     *
     * Note that users from databases we replicate are automatically added as peers. It is not possible to remove them,
     * but it is possible to change their status to Blocked.
     */
    virtual void removePeer( const CryptoHelper::PublicKeyHash& keyHash );
    /**
     * List all our peers.
     */
    virtual std::vector<Mist::Peer> listPeers() const;
    /**
     * Get information about a peer.
     */
    virtual Peer getPeer( const CryptoHelper::PublicKeyHash& keyHash ) const;

    /**
     * The address lookup server is used to store our TOR hidden service address, and to find the
     * TOR hidden service address of peers.
     *
     * This method adds a lookup server that we will use. The default public lookup server is
     * lookup.mist.nu port 40443.
     *
     * Address lookup servers are stored in a SQLite database.
     */
    virtual void addAddressLookupServer( const std::string& address, std::uint16_t port );
    /**
     * Remove an address lookup server. We will no longer use it.
     */
    virtual void removeAddressLookupServer( const std::string& address, std::uint16_t port );
    /**
     * List all address lookup server that we use.
     */
    virtual std::vector<std::pair<std::string, std::uint16_t>> listAddressLookupServers() const;

    virtual void addDatabasePermission( const CryptoHelper::PublicKeyHash& keyHash, const CryptoHelper::SHA3& dbHash );
    virtual void removeDatabasePermission( const CryptoHelper::PublicKeyHash& keyHash, const CryptoHelper::SHA3& dbHash );

    /**
     * List all databases a peer has permission to.
     */
    virtual std::vector<CryptoHelper::SHA3> listDatabasePermissions( const CryptoHelper::PublicKeyHash& keyHash );
    /**
     * Check is a peer has permission to a database.
     */
    bool hasDatabasePermission( const CryptoHelper::PublicKeyHash& keyHash, const CryptoHelper::SHA3& dbHash );

    /**
     * Add permission for a peer to connect to a service.
     */
    virtual void addServicePermission( const CryptoHelper::PublicKeyHash& keyHash, const std::string& service, const std::string& path );
    /**
     * Remove permission for a peer to connect to a service.
     */
    virtual void removeServicePermission( const CryptoHelper::PublicKeyHash& keyHash, const std::string& service, const std::string& path );
    /**
     * List all services a peer has permission to.
     */
    virtual std::map<std::string, std::vector<std::string>> listServicePermissions( const CryptoHelper::PublicKeyHash& keyHash );

    //
    // Suggestions for a connection api
    //
    virtual void startServeTor(std::string torPath,
        mist::io::port_range_list torIncomingPort,
        mist::io::port_range_list torOutgoingPort,
        mist::io::port_range_list controlPort);
    virtual void startServeDirect(
        mist::io::port_range_list directIncomingPort);

    using new_database_callback = std::function<void( Mist::Database::Manifest )>;
    /**
     * Start the Mist connection library and begin replicating databases.
     *
     * The newDatabase callback is called if we get permission to a new database. To replicate that database, call
     * receiveDatabase.
     *
     * We can force all connections to be anonymous for this session, by setting forceAnonymous to true. This is useful
     * if we normally allow some peers to make direct connections, but do want to override that for this session. Perhaps
     * because the user is using a public wifi.
     */
    virtual void startSync( new_database_callback newDatabase, bool forceAnonymous=false );
    /**
     * Stop the Mist connection library, and close any open network connections.
     */
    virtual void stopSync();

    using peer_service_list_callback = std::function<void( CryptoHelper::PublicKeyHash, std::vector<std::string> )>;
    /**
     * Ask a peer for a list of services we have permission to use.
     */
    virtual void listServices( const CryptoHelper::PublicKeyHash& peer, peer_service_list_callback callback );

    using service_connection_callback = std::function<void(std::shared_ptr<mist::io::Socket>)>;
    /**
     * Register a service, that will use a virtual socket to communications.
     *
     * We will get a callback if another peer connects to the service. Note that a peer must
     * first have been granted permission to use the service. Such permission is granted using the addServicePermission
     * method.
     */
    virtual void registerService( const std::string& service, service_connection_callback cb );

    using http_service_connection_callback = std::function<void(mist::h2::ServerRequest)>;
    /**
     * Register a service, that will use a http/2 to communicate.
     *
     * We will get a callback if another peer connects to the service. Note that a peer must
     * first have been granted permission to use the service. Such permission is granted using the addServicePermission
     * method.
     */
    virtual void registerHttpService( const std::string& service, http_service_connection_callback cb );

    /**
     * Open a connection to a service provided by one of our peers. The connection will use a virtual socket to
     * communicate.
     */
    using service_open_socket_callback
        = std::function<void(const CryptoHelper::PublicKeyHash&,
            std::shared_ptr<mist::io::Socket>)>;
    virtual void openServiceSocket(
        const CryptoHelper::PublicKeyHash keyHash, std::string service,
        std::string path, service_open_socket_callback cb);

    /**
     * Open a connection to a service provided by one of our peers. The connection will use a http/2 to
     * communicate.
     */
    using service_open_request_callback
        = std::function<void(const CryptoHelper::PublicKeyHash&,
            mist::h2::ClientRequest)>;
    virtual void openServiceRequest(
        const CryptoHelper::PublicKeyHash keyHash, std::string service,
        std::string method, std::string path, service_open_request_callback cb);

    virtual ~Central();

private:
    static bool dbExists( std::string filename );

    std::string path;
    std::unique_ptr<Helper::Database::Database> contentDatabase;
    std::unique_ptr<Helper::Database::Database> settingsDatabase;
    std::map<unsigned, Mist::Database*> databases;

    mist::io::IOContext ioCtx;
    mist::io::SSLContext sslCtx;
    mist::ConnectContext connCtx;

    /* Mist database service */
    mist::Service& dbService;

    CryptoHelper::PrivateKey privKey;

    /* Other services */
    struct {
        std::recursive_mutex mux;
        std::map<std::string, mist::Service*> services;
    } serv;
    mist::Service& getService(const std::string& service);

    /* Per-peer sync state */
    class PeerSyncState {
    public:

        PeerSyncState( Central& central,
            const CryptoHelper::PublicKey& pubKey );

        void startSync();
        void stopSync();
        void onPeerConnectionStatus(mist::Peer::ConnectionStatus status);

        void listServices(Central::peer_service_list_callback callback);

/*        const CryptoHelper::PublicKey& getPublicKey() const;
        const CryptoHelper::PublicKeyHash& getPublicKeyHash() const;
        mist::Peer& getPeer();*/

        mist::Peer& getConnPeer() {
            return peer;
        }
    private:

        using address_t = std::pair<std::string, std::uint16_t>;
        using address_vector_t = std::vector<address_t>;
        using manifest_vector_t = std::vector<Database::Manifest>;
        using sha3_vector_t = std::vector<CryptoHelper::SHA3>;

        void onDisconnect();

        void queryAddressServers();
        void queryAddressServersNext(address_vector_t::iterator it);
        void queryAddressServersDone();
        void connectTor();
        void connectTorDone();
        void connectDirect();
        void connectDirectDone();
//        void queryDatabases();
//        void queryDatabasesDone();
        void queryTransactions();
        void queryTransactionsNext();
        void queryTransactionsGetNextParent();
        void queryTransactionsDownloadNextTransaction(std::vector<std::string>::iterator it);
        void queryTransactionsDone();

        enum class State {
            Reset,
            Disconnected,
            QueryAddressServers,
            ConnectDirect,
            ConnectTor,
//            QueryDatabases,
            QueryTransactions,
        } state;
        Mist::Central& central;

        CryptoHelper::PublicKey pubKey;
        CryptoHelper::PublicKeyHash keyHash;
        mist::Peer& peer;
        bool anonymous;
        Mist::Database *currentDatabase;

        std::recursive_mutex mux;
        std::vector<std::pair<std::string, std::uint16_t>> addressServers;
        std::vector<std::pair<std::string, std::uint16_t>> addresses;
        std::vector<Database::Manifest> databases;
        sha3_vector_t databaseHashes;
        sha3_vector_t::iterator databaseHashesIterator;
        std::set<std::string> transactionParentsToDownload;
        std::map<std::string,JSON::Value> transactionsToDownload;
        std::vector<std::string> transactionToDownloadInOrder;
    };
    friend class PeerSyncState;

    /* Global sync state */
    struct {
        std::recursive_mutex mux;
        bool started;
        bool forceAnonymous;
        std::map<CryptoHelper::PublicKeyHash, std::unique_ptr<PeerSyncState>> peerState;
    } sync;

    void syncStep();
    PeerSyncState& getPeerSyncState( const CryptoHelper::PublicKeyHash& keyHash );

    /* connCtx callbacks */
    void authenticatePeer( mist::Peer& peer );
    void onPeerConnectionStatus( mist::Peer& peer,
        mist::Peer::ConnectionStatus status );

    /* REST interface */
    void peerRestRequest( mist::Peer& peer, mist::h2::ServerRequest request,
        std::string path );
    class RestRequest : public std::enable_shared_from_this<RestRequest>
    {
    public:
        static void serve( Central& central, mist::Peer& peer,
            mist::h2::ServerRequest request, const std::string& path );

    private:
        RestRequest( Central& central, mist::Peer& peer,
            mist::h2::ServerRequest request );

        Central& central;
        mist::Peer& peer;
        CryptoHelper::PublicKeyHash keyHash;
        mist::h2::ServerRequest request;
        std::map<CryptoHelper::SHA3,Mist::Database*> databaseCache;

        void entry( const std::string& path );

        void transactions( const std::vector<std::string>& elts );
        void transaction( const CryptoHelper::SHA3& dbHash,
            const CryptoHelper::SHA3& trHash );
        void transactionMetadata( const CryptoHelper::SHA3& dbHash,
            const CryptoHelper::SHA3& trHash );
        void transactionsAll( const CryptoHelper::SHA3& dbHash );
        void transactionsLatest( const CryptoHelper::SHA3& dbHash );
        void transactionsRange( const CryptoHelper::SHA3& dbHash,
            const CryptoHelper::SHA3& fromTrHash,
            const CryptoHelper::SHA3& toTrHash );
        void transactionsNew( const CryptoHelper::SHA3& dbHash );
        void transactionsHaveNew( const CryptoHelper::SHA3& dbHash );

        void databases( const std::vector<std::string>& elts );
        void databasesAll();
        void databaseInvite( const Mist::Database::Manifest &manifest );

        void users( const std::vector<std::string>& elts );
        void usersAll();
        void usersChanged( const CryptoHelper::SHA3& dbHash,
            const CryptoHelper::SHA3& fromTrHash,
            const CryptoHelper::SHA3& toTrHash );
        void userInfo( const CryptoHelper::PublicKeyHash& user );

        void replyBadRequest();
        void replyBadMethod();
        void replyNotFound();
        void replyNotAuthorized();
    };
    friend class RestRequest;

};

} /* namespace Mist */

#endif /* SRC_CENTRAL_H_ */
