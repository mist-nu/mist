/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <vector>

#include "Helper.h"
#include "Database.h"
#include "gtest/gtest.h"

namespace M = Mist;
namespace FS = M::Helper::filesystem;

using V = M::Database::Value;
using T = V::T;

TEST(ManifestTest, TODO_TestAllMethods) {
    LOG( INFO ) << "TODO: Test all methods";
    // TODO: Do this.
}

TEST(ObjectRefTest, TODO_TestSort) {
    LOG( INFO ) << "TODO: Test sorting object references";
    // TODO: Do this.
}

TEST(DatabaseValue, TestEmpty) {
    LOG( INFO ) << "Test Database::Value empty";

    V dv { };
    EXPECT_EQ( T::NoType, dv.t );
    EXPECT_FALSE( dv.b );
}

TEST(DatabaseValue, TestBool) {
    LOG( INFO ) << "Test Database::Value::Boolean";

    {
        V dv( true );
        EXPECT_EQ( T::Boolean, dv.t );
        EXPECT_TRUE( dv.b );
    }

    {
        V dv( false );
        EXPECT_EQ( T::Boolean, dv.t );
        EXPECT_FALSE( dv.b );
    }
}

TEST(DatabaseValue, TestNumber) {
    LOG( INFO ) << "Test Database::Value::Number";

    {
        V dv( 0 );
        EXPECT_EQ( T::Number, dv.t );
        EXPECT_NEAR( 0.0, dv.n, 0.000001 );
    }

    {
        V dv( 17 );
        EXPECT_EQ( T::Number, dv.t );
        EXPECT_NEAR( 17.0, dv.n, 0.000001 );
    }

    {
        V dv( 1.0 );
        EXPECT_EQ( T::Number, dv.t );
        EXPECT_NEAR( 1.0, dv.n, 0.000001 );
    }

    {
        V dv( 2.0 );
        EXPECT_EQ( T::Number, dv.t );
        EXPECT_NEAR( 2.0, dv.n, 0.000001 );
    }
}

TEST(DatabaseValue, TestString) {
    LOG( INFO ) << "Test Database::Value::String";

    {
        V dv( "A" );
        EXPECT_EQ( T::String, dv.t );
        EXPECT_EQ( 0, dv.v.compare( "A" ) );
    }

    {
        V dv( "B" );
        EXPECT_EQ( T::String, dv.t );
        EXPECT_NE( T::Json, dv.t );
        EXPECT_EQ( 0, dv.v.compare( "B" ) );
    }

    {
        V dv( std::string( "C" ) );
        EXPECT_EQ( T::String, dv.t );
        EXPECT_EQ( 0, dv.v.compare( "C" ) );
    }
}

TEST(DatabaseValue, TestJson) {
    LOG( INFO ) << "Test Database::Value::JSON";

    {
        std::string json = "JSON";
        V dv( json, true );
        EXPECT_EQ( T::Json, dv.t );
        EXPECT_NE( T::String, dv.t );
        EXPECT_EQ( 0, dv.v.compare( "JSON" ) );
    }

    {
        V dv( "JSON2", true );
        EXPECT_EQ( T::Json, dv.t );
        EXPECT_EQ( 0, dv.v.compare( "JSON2" ) );
    }

    {
        V dv( "JSON3", true );
        EXPECT_EQ( T::Json, dv.t );
        EXPECT_EQ( 0, dv.v.compare( "JSON3" ) );
    }
}

class DatabaseTest: public ::testing::Test {
public:
    DatabaseTest() :
            p( "test.db" ), db( nullptr, p.make_preferred().string() ), db2( nullptr, p.make_preferred().string() ) {
        if ( FS::exists( p ) && FS::is_regular( p ) ) {
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

    FS::path p;
    M::Database db, db2;
};

TEST(DatabaseTestTrivial, Trivial) {
    LOG( INFO ) << "Test Database constructor";
    M::Database db { nullptr, "test.db" };
}

TEST_F(DatabaseTest, Create) {
    LOG( INFO ) << "Test Database create";
    ASSERT_NO_THROW( db.create( 0, nullptr ) );

    EXPECT_ANY_THROW( db2.create( 0, nullptr ) ); // TODO: Create better exception in the class
    //EXPECT_FALSE( db2.isOK() );
}

TEST_F(DatabaseTest, Init) {
    LOG( INFO ) << "Test Database init";
    db.create( 0, nullptr );
    ASSERT_TRUE( db.isOK() );

    db.init();
    EXPECT_TRUE( db.isOK() );

    db2.init();
    EXPECT_TRUE( db2.isOK() );
}

TEST_F(DatabaseTest, Close) {
    LOG( INFO ) << "Test Database close";
    db.create( 0, nullptr );
    ASSERT_TRUE( db.isOK() );

    db.init();
    ASSERT_TRUE( db.isOK() );

    db.close();
    ASSERT_TRUE( db.isOK() );
}
