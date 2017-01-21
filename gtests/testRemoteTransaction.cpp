/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <chrono>
#include <string>
#include <thread>

#include <gtest/gtest.h> // Google test framework

#include "crypto/key.hpp"

#include "io/io_context.hpp"
#include "io/ssl_context.hpp"

#include "CryptoHelper.h"
#include "Database.h"
#include "Exception.h"
#include "Helper.h"
#include "RemoteTransaction.h"

namespace {

namespace M = Mist;
namespace H = M::Helper;
namespace FS = H::filesystem;
namespace C = M::CryptoHelper;

using namespace std::placeholders;

const std::string dir{ "remoteTransactionTest" };
const std::string db_name{ "remoteTransactionTest" };
const std::string db_file{ db_name + ".db" };

// Manifest
C::SHA3 dbHash{ C::SHA3::fromString( "HLlZ+hhZIx0OppsWjOCiyjjD3Sj18UrRe9bAjbYntLE=" ) };
H::Date dbCreated( "2017-01-21 15:43:24.267" );
std::string dbCreatorPem = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1SEWOOAm6wFSHI6ixs0jBGMyfriXfpFFaDL15ihHdfezFNR8mc6AO30UeArNfCp/IgQff7lk735fni8O5GrbFd8LBYtFH7AT55MDa+qjK1VkwFlEPq9Qezhg+Rbnrole1XU2zc/NiRuCQtrygblhCbdK87kTirpOAnFT7lGonBNx4Os6tYJwcBl15xQpeRD0wO1PZQg2BLGyymrtdlnEBd+kg58ffmnlgJuAwx+FZ5L2Zzz4hrfFCQQVQvXJJlamIWYuZAE8+HXtQQ1iVay0ZuhCVziEfESrBTZrwVXYCcgM54cN1/jCqI0tb6Vl5Ny0CX/Yw1Gzo3fi/Ub20ZDHlQIDAQAB
-----END PUBLIC KEY-----)";
C::PublicKey dbCreator{};
std::string dbName{ "db2" };
//C::Signature dbSignature{ C::Signature::fromString( "kEW6ZNK18Ji90atFYtEIiEed/7xTU5DJPeA5y0yELzZPngCd+N8scUBQ6guuqDLZqQrZAx0lDwiRadxc1w2ldWX+Y7r3pCRQ8Ozgpb7kC/H6mDGDnPqdwgfaDA/b+U6ohb8oZw+eyvd34IRks9UXhL6OslNRAdkcg2dVR9pFzJ2CihgH41aeCbX/rtGMT5UQ79tv/zz31RIKo9tI5U7Wq4NRTfr4imug1fQislY7Z2XPWQhtQMLX7M5quE6hTsOG37xfNlVRxvJR6vcoieEIjWfOC/4U0l3sFdRl59Bwmg8GAEXhUIyQkJl1AZdNjGMrbCrKZUgQnwOewPbYmC47Jg==") };
std::string privKeyEncoded{ "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"};

/*
C::Signature fakeSign( const C::SHA3& hash ) {
    return C::Signature::fromString( hash.toString() );
}

bool fakeVerify( const C::PublicKey&, const C::SHA3&, const C::Signature& ) {
    return true;
}

C::Signature sign( mist::io::SSLContext& sslCtx, const C::SHA3& hash ) {
    auto buf( sslCtx.sign( hash.data(), hash.size() ) );
    return C::Signature::fromBuffer( buf );
}

bool verify( mist::io::SSLContext& sslCtx,
        const C::PublicKey& key,
        const C::SHA3& hash,
        const C::Signature& sig) {
    return sslCtx.verify(key.toDer(), hash.data(), hash.size(),
        sig.data(), sig.size());
}
//*/



class OpenDatabase : public M::Database {
public:
    OpenDatabase( M::Central* c, const std::string& path ) :
        M::Database( c, path ) /*, ioCtx(), sslCtx( ioCtx, dir ) */ {

        this->signer = std::bind( &OpenDatabase::sign, this, _1 );
        this->verifier = std::bind( &OpenDatabase::verify, this, _1, _2, _3 );

        /*
        auto privKeyVec( mist::crypto::base64Decode( privKeyEncoded ) );
        std::string privKey{ reinterpret_cast<const char*>( privKeyVec.data() ), privKeyVec.size() };
        sslCtx.loadPKCS12( privKey , "");

        // Start io event loop
        ioCtx.queueJob([=]() { ioCtx.exec(); });
        std::this_thread::sleep_for( std::chrono::microseconds( 500 ) );

        userKey = C::PublicKey::fromDer( sslCtx.derPublicKey() );
        userHash = userKey.hash();
        //*/
        dbCreator = C::PublicKey::fromPem( dbCreatorPem );
    }

    virtual ~OpenDatabase() = default;

    bool verify( const C::PublicKey& key,
            const C::SHA3& hash,
            const C::Signature& sig ) const {
        /*
        return sslCtx.verify(key.toDer(), hash.data(), hash.size(),
            sig.data(), sig.size());
        /*/
        return true;
        //*/
    }

    C::Signature sign( const C::SHA3& hash ) const {
        /*
        auto buf( sslCtx.sign( hash.data(), hash.size() ) );
        return C::Signature::fromBuffer( buf );
        /*/
        return C::Signature::fromString( hash.toString() );
        //*/
    }

    C::PublicKey getPublicKey() const {
        //return C::PublicKey::fromDer(sslCtx.derPublicKey());
        //return dbCreator;
        return C::PublicKey();
    }

    std::unique_ptr<M::Database::Manifest> createManifest() {
        std::unique_ptr<M::Database::Manifest> m( new M::Database::Manifest(
                std::bind( &OpenDatabase::sign, this, _1 ),
                std::bind( &OpenDatabase::verify, this, _1, _2, _3),
                dbName,
                dbCreated,
                getPublicKey() ) );
        m->sign();
        return std::move( m );
    }

    using M::Database::beginTransaction;
    using M::Database::beginRemoteTransaction;

    //mist::io::IOContext ioCtx;
    //mist::io::SSLContext sslCtx;
};

using D = OpenDatabase;
using ORef = D::ObjectRef;
using AD = D::AccessDomain;
using V = D::Value;
using VT = D::Value::Type;

using RT = M::RemoteTransaction;
using URT = std::unique_ptr<RT>;

void removeTestDb( const FS::path &p ) {
    LOG ( INFO ) << "Removing db: " << p;
    if ( FS::exists( p ) ) {
        ASSERT_TRUE( FS::is_regular( p ) );
        FS::remove( p );
    }
    FS::path wal( p.string() + "-wal" );
    if ( FS::exists( wal ) ) {
        FS::remove( wal );
    }
    FS::path shm( p.string() + "-shm" );
    if ( FS::exists( shm ) ) {
        FS::remove( shm );
    }
}

TEST( OpenDatabaseTest, Constructor ) {
    FS::path p { db_file };
    LOG ( INFO ) << "Constructor db: " << p.string();
    removeTestDb( p );
    dbCreator = C::PublicKey::fromPem( dbCreatorPem );
    D db( nullptr, p.make_preferred().string() );
}

TEST( OpenDatabaseTest, CreateManifest ) {
    FS::path p { db_file };
    LOG ( INFO ) << "Manifest for db: " << p.string();
    removeTestDb( p );
    D db( nullptr, p.string() );

    db.createManifest();
}

TEST( OpenDatabaseTest, CreateDb ) {
    FS::path p { db_file };
    LOG ( INFO ) << "Creating db: " << p.string();
    removeTestDb( p );
    D db( nullptr, p.make_preferred().string() );

    db.create( 0, std::move( db.createManifest() ) );
    db.close();
}

TEST( OpenDatabaseTest, InitDb ) {
    FS::path p { db_file };
    LOG ( INFO ) << "Init db: " << p.string();
    D db( nullptr, p.make_preferred().string() );

    db.init( std::move( db.createManifest() ) );
    db.close();
}

TEST(InitRemoteTransactionTest, CreateDb) {
    FS::path p { db_file };
    LOG ( INFO ) << "Creating db: " << p.string();
    removeTestDb( p );
    D db( nullptr, p.make_preferred().string() );

    db.create( 0, std::move( db.createManifest() ) );
    db.close();
}

C::SHA3Hasher hasher{};
C::SHA3 r() {
    hasher.reset();
    hasher.update( std::to_string( C::cryptoRandom() ) );
    return hasher.finalize();
}

TEST( TestRemoteTransaction, Constructor ) {
    LOG ( INFO ) << "Testing constructor";
    FS::path p{ db_file };
    D db( nullptr, p.make_preferred().string() );
    EXPECT_NO_THROW( db.init( std::move( db.createManifest() ) ) );

    std::vector<D::Transaction> parents{};

    C::SHA3 transHash{ r() };
    std::unique_ptr<RT> rt{ std::move( db.beginRemoteTransaction(
            AD::Normal,
            parents,
            H::Date( H::Date::now() ),
            C::SHA3(),
            transHash,
            db.sign( transHash )
    )) };
    rt.reset();
    EXPECT_NO_THROW( db.close() );
}

TEST( TestRemoteTransaction, Init ) {
    LOG ( INFO ) << "Testing init";
    FS::path p{ db_file };
    std::unique_ptr<D> db{ new D( nullptr, p.make_preferred().string() ) };
    EXPECT_NO_THROW( db->init() );
    std::vector<D::Transaction> parents{};
    std::unique_ptr<RT> rt{ new RT(
        db.get(),
        AD::Normal,
        1,
        parents,
        H::Date( H::Date::now() ),
        C::SHA3(),
        C::SHA3(),
        C::Signature()
    ) };
    EXPECT_NO_THROW( rt->init() );
    rt.reset();
    EXPECT_NO_THROW( db->close() );
    db.reset();
}

TEST( TestRemoteTransaction, CommitEmptyId10and20 ) {
    LOG ( INFO ) << "Testing two commits";
    FS::path p{ db_file };
    std::unique_ptr<D> db{ new D( nullptr, p.make_preferred().string() ) };
    db->init();
    std::vector<D::Transaction> parents{};

    // 10
    std::unique_ptr<RT> rt{ new RT(
        db.get(),
        AD::Normal,
        1,
        parents,
        H::Date( H::Date::now() ),
        C::SHA3(),
        C::SHA3(),
        C::Signature()
    ) };
    EXPECT_NO_THROW( rt->init() );
    EXPECT_NO_THROW( rt->commit() );
    rt.reset();

    // 20
    rt.reset( new RT(
        db.get(),
        AD::Normal,
        2,
        parents,
        H::Date( H::Date::now() ),
        C::SHA3(),
        C::SHA3(),
        C::Signature()
    ) );
    EXPECT_NO_THROW( rt->init() );
    EXPECT_NO_THROW( rt->commit() );

    db->close();
    db.reset();
}

class RemoteTransactionTest: public ::testing::Test {
public:
    URT beginRemoteWithTimeDiff( int timeDiff ) {
        return db.beginRemoteTransaction(
                AD::Normal,
                parents,
                H::Date( H::Date::now( timeDiff ) ),
                C::SHA3( r() ),
                C::SHA3( r() ),
                C::Signature() );
    }

    RemoteTransactionTest() :
            p( db_file ), db( nullptr, p.make_preferred().string() ), rt{}, parents{} {
        db.init();
        transactionParent = D::Transaction{
            AD::Normal,
            1,
            H::Date( "1970-01-01 00:00:00" ),
            C::SHA3(),
            C::SHA3(),
            C::Signature()
        };
        //parents.push_back( transactionParent );
        rt = std::move(
                db.beginRemoteTransaction(
                                AD::Normal,
                                parents,
                                H::Date( H::Date::now() ),
                                C::SHA3(),
                                C::SHA3(),
                                C::Signature()
        ) );
        rt->init();
    }

    ~RemoteTransactionTest() {
        rt.reset();
        db.close();
    }

    FS::path p;
    D db;
    URT rt;
    ORef rootParent{ AD::Normal, 0UL };

private:
    D::Transaction transactionParent;
    std::vector<D::Transaction> parents;
};

TEST_F( RemoteTransactionTest, Empty ) {
    LOG (INFO ) << "Test empty transaction";
}

TEST_F( RemoteTransactionTest, Rollback ) {
    LOG (INFO ) << "Test rollback";
    rt->rollback();
}

TEST_F( RemoteTransactionTest, CommitEmpty ) {
    LOG (INFO ) << "Test commit empty";
    EXPECT_NO_THROW( rt->commit() );
}

TEST_F( RemoteTransactionTest, NewEmptyObject ) {
    LOG( INFO ) << "Testing to create new object";

    std::map<std::string, D::Value> attr{};
    rt->newObject( C::cryptoRandom(), rootParent, attr );
    EXPECT_NO_THROW( rt->commit() );
}

unsigned long oid{}; // used in "update" test

TEST_F( RemoteTransactionTest, NewObject ) {
    LOG( INFO ) << "Testing to create new object";

    std::map<std::string, D::Value> attr{};
    attr.emplace( "test", D::Value( true ) );
    oid = C::cryptoRandom(); // used in "update" test
    rt->newObject( oid, rootParent, attr );
    EXPECT_NO_THROW( rt->commit() );
}

ORef latest{}, old{}, older{}; // used in the next test

TEST_F( RemoteTransactionTest, NewObjects1000 ) {
    LOG( INFO ) << "Testing to create 1000 new objects";

    std::map<std::string, D::Value> attr{};
    attr.emplace( "test", D::Value( 1 ) );
    ORef parent{rootParent };
    unsigned long id{ static_cast<unsigned long>(C::cryptoRandom()) } ;
    for ( int i{0}; i < 1000; ++i ) {
        attr["test"] = i;
        rt->newObject( id, parent, attr );
        older = old;
        old = parent;
        parent.id = id;
        id = C::cryptoRandom();
    }
    EXPECT_NO_THROW( rt->commit() );
    latest = parent;
}

TEST_F( RemoteTransactionTest, MoveObject ) {
    LOG( INFO ) << "Testing to move object";

    rt->moveObject( latest.id, older );
    EXPECT_NO_THROW( rt->commit() );
}

TEST_F( RemoteTransactionTest, UpdateObject ) {
    LOG( INFO ) << "Testing to update object";

    std::map<std::string, D::Value> attr{};
    attr.emplace( "updated", D::Value( 1 ) );
    rt->updateObject( oid, attr );
    EXPECT_NO_THROW( rt->commit() );
}

TEST_F( RemoteTransactionTest, DeleteObject ) {
    LOG( INFO ) << "Testing to delete object";

    rt->deleteObject( older.id );
    EXPECT_NO_THROW( rt->commit() );
}

TEST_F( RemoteTransactionTest, ReorderOldIncommingTransactions ) {
    // TODO: Verify that this test is valid with the current test data.
    LOG( INFO ) << "Testing reorder of old incomming transactions.";
    std::map<std::string, D::Value> attr{};

    URT r1 = std::move( beginRemoteWithTimeDiff( 0 ) );
    r1->init();
    attr.clear();
    attr.emplace( "o1", D::Value( true ) );
    r1->newObject( C::cryptoRandom(), rootParent, attr );
    r1->commit();
    r1.reset();

    URT r2 = std::move( beginRemoteWithTimeDiff( -2 ) );
    r2->init();
    attr.clear();
    attr.emplace( "o2", D::Value( true) );
    r2->newObject( C::cryptoRandom(), rootParent, attr );
    r2->commit();
    r2.reset();
}

} /* namespace */

