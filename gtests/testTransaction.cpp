/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <exception>

#include <gtest/gtest.h> // Google test framework

#include "Exception.h"
#include "Database.h"
#include "Transaction.h"

namespace { // Anonymous namespace

const std::string db_file{ "transactionTest.db" };

namespace M = Mist;
namespace FS = M::Helper::filesystem;

using O = M::Database::Object;
using ORef = M::Database::ObjectRef;
using AD = M::Database::AccessDomain;
using V = M::Database::Value;
using VT = M::Database::Value::Type;
using QR = M::Database::QueryResult;

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

TEST(InitTransactionTest, CreateDb) {
    FS::path p { db_file };
    removeTestDb( p );
    LOG ( INFO ) << "Creating db: " << p;
    M::Database db( nullptr, p.make_preferred().string() );
    db.create( 0, nullptr );
    db.close();
}

unsigned long id_A { 0 }, id_B { 0 }, id_C { 0 };

class OpenDatabase : public M::Database {
public:
    OpenDatabase( M::Central* c, const std::string& path ) : M::Database( c, path ) {}
    virtual ~OpenDatabase() = default;

    using M::Database::beginTransaction;
    using M::Database::beginRemoteTransaction;
};

class TransactionTest: public ::testing::Test {
public:
    TransactionTest() :
            p( db_file ), db( nullptr, p.make_preferred().string() ) {
        db.init();
    }

    ~TransactionTest() {
        db.close();
    }

    FS::path p;
    OpenDatabase db;
};

TEST_F(TransactionTest, NewObjects) {
    LOG( INFO ) << "Testing new objects";

    std::unique_ptr<M::Transaction> t{ nullptr };
    ASSERT_NO_THROW( t = std::move( db.beginTransaction( AD::Normal ) ) );

    ORef o_A { AD::Normal, 0 };
    std::map<std::string, V> m_A {
        { "name", V( "A" ) },
        { "test1", V( 17 ) },
        { "test2", V( "test" ) }
    };
    ASSERT_NO_THROW( id_A = t->newObject( o_A, m_A ) );

    ORef o_B { AD::Normal, id_A };
    // TODO: fix the JSON here when the "JSON Normalizer" is done.
    std::map<std::string, V> m_B {
        { "name", V( "B" ) },
        { "test3", V( "{'test':17}", true ) },
        { "test4", V(  true ) }
    };
    ASSERT_NO_THROW( id_B = t->newObject( o_B, m_B ) );

    ORef o_C { AD::Normal, id_B };
    std::map<std::string, V> m_C { { "name", V( "C" ) } };
    ASSERT_NO_THROW( id_C = t->newObject( o_C, m_C ) );

    try {
        t->commit();
    } catch ( const M::Exception& e ) {
        t.reset();
        LOG( WARNING ) << "Mist exception: " << e.what();
        FAIL() << "Mist ex: " << e.what();
    } catch ( const SQLite::Exception& e ) {
        t.reset();
        LOG( WARNING ) << "SQLite exception: " << e.what();
        FAIL() << "SQL ex: " << e.what();
    } catch ( const std::runtime_error& e ) {
        t.reset();
        LOG( WARNING ) << "Runtime error: " << e.what();
        FAIL() << "Runtime error: " << e.what();
    }
    t.reset();
}

TEST_F( TransactionTest, Create100NewObjects) {
    LOG( INFO ) << "Create 100 new object for to see time consumption";

    std::unique_ptr<M::Transaction> t{ nullptr };
    t = std::move( db.beginTransaction( AD::Normal ) );

    unsigned long prevId = 0;
    for(int i = 0; i < 100; ++i) {
        ORef o { AD::Normal, prevId };
        std::map<std::string, V> m {
            { std::string( "name" + std::to_string( i ) ), V( "A" + std::to_string( i ) ) },
            { std::string( "test" + std::to_string( i ) ), V( i ) }
        };
        prevId = t->newObject( o, m );
    }
    t->commit();
}

TEST_F( TransactionTest, TODO_UpdateObjects) {
    LOG( INFO ) << "Testing update objects";
    // TODO: The .ts file seems to only use transaction.moveObject,
    // and not transaction.updateObject.
    // Figure out these test cases later, when the rest have been done.
}

TEST_F( TransactionTest, MoveObjects ) {
    LOG( INFO ) << "Testing move objects";

    std::unique_ptr<M::Transaction> t{ nullptr };
    //try {
        ORef o_C { AD::Normal, id_C };
        t = std::move( db.beginTransaction( AD::Normal ) );
        t->moveObject( id_B, o_C );
        t->rollback();
        t.reset();
        //delete t;
        //t = nullptr;

        ORef o_A { AD::Normal, id_A };
        t = std::move( db.beginTransaction( AD::Normal ) );
        t->moveObject( id_C, o_A );
        t->commit();
        t.reset();
    /*
    } catch ( M::Exception const & e ) {
        FAIL() << e.getErrorCode();
    } catch ( const SQLite::Exception& e ) {
        FAIL() << e.what();
    } catch (...) {
        FAIL();
    }
    //*/
}

TEST_F( TransactionTest, DeleteObjects ) {
    LOG( INFO ) << "Testing delete objects";

    std::unique_ptr<M::Transaction> t{ nullptr };
    //try {
        t = std::move( db.beginTransaction( AD::Normal ) );
        EXPECT_ANY_THROW( t->deleteObject(id_A) ); // TODO: Should this throw?
        t.reset();

        t = std::move( db.beginTransaction( AD::Normal ) );
        t->deleteObject( id_B );
        t->commit();
    /*
    } catch ( M::Exception const & e ) {
        FAIL() << e.getErrorCode();
    } catch (...) {
        FAIL();
    }
    //*/
}

TEST_F( TransactionTest, QueryAndSubscribeToObject ) {
    std::map<std::string,V> args{};
    QR qr{ db.query( static_cast<int>( AD::Normal ), 0, "", "", "", args, 0, false ) }; // Get all objects
    EXPECT_FALSE( qr.isFunctionCall ); // Not function call?
    EXPECT_LT( 0u, qr.objects.size() ); // Got some objects?
    O& changeThis{ qr.objects.at( 50 ) }; // Save ref to object
    EXPECT_NE( 0u, changeThis.id ); // ref is not root?
    EXPECT_NE( 0u, changeThis.parent.id ); // Parent to ref is not root?
    O o{ qr.objects.at(0) }; // Temp copy first
    EXPECT_NE( o.id, changeThis.id ); // First != ref ?

    // Subscribe to last
    unsigned subId = db.subscribeObject(
            [&o](O obj) -> void {
                o = obj;
            },
            static_cast<int>( changeThis.accessDomain ),
            changeThis.id,
            false );

    std::unique_ptr<M::Transaction> t{ std::move( db.beginTransaction() ) };
    t->moveObject( changeThis.id, { AD::Normal, 0 } ); // Make last as leaf to root
    t->commit();
    EXPECT_EQ( o.id, changeThis.id ); // Expect this to have changed
    db.unsubscribe( subId );
}

TEST_F( TransactionTest, SubscribeToQuery ) {
    bool gotCb{ false };
    std::map<std::string,V> args{};

    // Sub to all objects
    unsigned subId = db.subscribeQuery(
            [&gotCb](QR) -> void {
                gotCb = true;
            },
            static_cast<int>( AD::Normal ), 0, "", "", "", args, 0, false
    );

    // Get all objects
    QR qr{ db.query( static_cast<int>( AD::Normal ), 0, "", "", "", args, 0, false ) }; // Get all objects
    EXPECT_FALSE( qr.isFunctionCall ); // Not function call?
    EXPECT_LT( 0u, qr.objects.size() ); // Got some objects?
    O& changeThis{ qr.objects.at( 25 ) }; // Save ref to object
    EXPECT_NE( 0u, changeThis.id ); // ref is not root?
    EXPECT_NE( 0u, changeThis.parent.id ); // Parent to ref is not root?
    O o{ qr.objects.at(0) }; // Temp copy first
    EXPECT_NE( o.id, changeThis.id ); // First != ref ?

    // Change ref
    std::unique_ptr<M::Transaction> t{ std::move( db.beginTransaction() ) };
    t->moveObject( changeThis.id, { AD::Normal, 0 } ); // Make last as leaf to root
    t->commit();

    EXPECT_TRUE( gotCb );

    db.unsubscribe( subId );
}

TEST_F( TransactionTest, DumpDb ) {
    db.dump( p.string() );
}

}
