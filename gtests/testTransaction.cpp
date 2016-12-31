/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <exception>

// Logger
/*
#include <memory>
#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>
#include <g3log/std2_make_unique.hpp>
#include "Helper.h"
//*/

#include <gtest/gtest.h> // Google test framework

#include "Exception.h"
#include "Database.h"
#include "Transaction.h"

namespace { // Anonymous namespace

const std::string db_file{ "transactionTest.db" };

namespace M = Mist;
namespace FS = M::Helper::filesystem;

using ORef = M::Database::ObjectRef;
using AD = M::Database::AccessDomain;
using V = M::Database::Value;
using VT = M::Database::Value::Type;


/*
std::unique_ptr<g3::LogWorker> logWorker;
std::unique_ptr<g3::SinkHandle<g3::FileSink>> logHandle;

TEST(InitTransactionTest, InitLogger) {
	const std::string dir = "./log/";
	const std::string file = "testTransactions";
	logWorker = g3::LogWorker::createLogWorker();
	logHandle = logWorker->addDefaultLogger(file, dir);
	g3::initializeLogging(logWorker.get());
}
//*/

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
        { "name", V( VT::String, "A" ) },
        { "test1", V( VT::Number, 17 ) },
        { "test2", V( VT::String, "test" ) }
    };
    ASSERT_NO_THROW( id_A = t->newObject( o_A, m_A ) );

    ORef o_B { AD::Normal, id_A };
    // TODO: fix the JSON here when the "JSON Normalizer" is done.
    std::map<std::string, V> m_B {
        { "name", V( VT::String, "B" ) },
        { "test3", V( VT::JSON, "{'test':17}" ) },
        { "test4", V( VT::Boolean, true ) }
    };
    ASSERT_NO_THROW( id_B = t->newObject( o_B, m_B ) );

    ORef o_C { AD::Normal, id_B };
    std::map<std::string, V> m_C { { "name", V( VT::String, "C" ) } };
    ASSERT_NO_THROW( id_C = t->newObject( o_C, m_C ) );

    try {
        t->commit();
    } catch ( const M::Exception& e ) {
        LOG( WARNING ) << "Mist exception: " << e.what();
        FAIL() << "Mist ex: " << e.what();
    } catch ( const SQLite::Exception& e ) {
        LOG( WARNING ) << "SQLite exception: " << e.what();
        FAIL() << "SQL ex: " << e.what();
    } catch ( const std::runtime_error& e ) {
        LOG( WARNING ) << "Runtime error: " << e.what();
        FAIL() << "Runtime error: " << e.what();
    }
}

TEST_F( TransactionTest, Create1000NewObjects) {
    LOG( INFO ) << "Create 1000 new object for to see time consumption";

    std::unique_ptr<M::Transaction> t{ nullptr };
    t = std::move( db.beginTransaction( AD::Normal ) );

    unsigned long prevId = 0;
    for(int i = 0; i < 1000; ++i) {
        ORef o { AD::Normal, prevId };
        std::map<std::string, V> m {
            { std::string( "name" + std::to_string( i ) ), V( VT::String, std::string( "A" + std::to_string( i ) ) ) },
            { std::string( "test" + std::to_string( i ) ), V( VT::Number, i ) }
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

}

