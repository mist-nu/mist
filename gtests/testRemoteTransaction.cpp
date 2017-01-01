/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <gtest/gtest.h> // Google test framework

#include "CryptoHelper.h"
#include "Database.h"
#include "Exception.h"
#include "Helper.h"
#include "RemoteTransaction.h"

namespace {

const std::string db_file{ "remoteTransactionTest.db" };

namespace M = Mist;
namespace H = M::Helper;
namespace FS = H::filesystem;
namespace C = M::CryptoHelper;

class OpenDatabase : public M::Database {
public:
    OpenDatabase( M::Central* c, const std::string& path ) : M::Database( c, path ) {}
    virtual ~OpenDatabase() = default;

    using M::Database::beginTransaction;
    using M::Database::beginRemoteTransaction;
};

using D = OpenDatabase;
using ORef = D::ObjectRef;
using AD = D::AccessDomain;
using V = D::Value;
using VT = D::Value::T;

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

TEST(InitRemoteTransactionTest, CreateDb) {
    FS::path p { db_file };
    removeTestDb( p );
    LOG ( INFO ) << "Creating db: " << p;
    D db( nullptr, p.make_preferred().string() );
    db.create( 0, nullptr );
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
    db.init();
    std::vector<D::Transaction> parents{};
    std::unique_ptr<RT> rt{ new RT(
        &db,
        AD::Normal,
        1,
        parents,
        H::Date( H::Date::now() ),
        C::SHA3(),
        C::SHA3(),
        C::Signature()
    ) };
    rt.reset();
    db.close();
}

TEST( TestRemoteTransaction, Init ) {
    LOG ( INFO ) << "Testing init";
    FS::path p{ db_file };
    std::unique_ptr<D> db{ new D( nullptr, p.make_preferred().string() ) };
    db->init();
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
    db->close();
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

