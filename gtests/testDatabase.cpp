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
    using V = M::Database::Value;
    using T = V::Type;

    V dv { };
    EXPECT_EQ( T::Boolean, dv.type );
    EXPECT_FALSE( dv.value.boolean );
}

TEST(DatabaseValue, TestBool) {
	LOG( INFO ) << "Test Database::Value::Boolean";
    using V = M::Database::Value;
    using T = V::Type;

    {
        V dv( T::Boolean, true );
        EXPECT_EQ( T::Boolean, dv.type );
        EXPECT_TRUE( dv.value.boolean );
    }

    {
        V dv( T::Boolean, false );
        EXPECT_EQ( T::Boolean, dv.type );
        EXPECT_FALSE( dv.value.boolean );
    }
}

TEST(DatabaseValue, TestNumber) {
	LOG( INFO ) << "Test Database::Value::Number";
    using V = M::Database::Value;
    using T = V::Type;

    {
        V dv( T::Number, 0 );
        EXPECT_EQ( T::Number, dv.type );
        EXPECT_NEAR( 0.0, dv.value.number, 0.000001 );
    }

    {
        V dv( T::Number, 17 );
        EXPECT_EQ( T::Number, dv.type );
        EXPECT_NEAR( 17.0, dv.value.number, 0.000001 );
    }

    {
        V dv( T::Number, float( 1.0 ) );
        EXPECT_EQ( T::Number, dv.type );
        EXPECT_NEAR( 1.0, dv.value.number, 0.000001 );
    }

    {
        V dv( T::Number, float( 2.0 ) );
        EXPECT_EQ( T::Number, dv.type );
        EXPECT_NEAR( 2.0, dv.value.number, 0.000001 );
    }
}

TEST(DatabaseValue, TestString) {
	LOG( INFO ) << "Test Database::Value::String";
    using V = M::Database::Value;
    using T = V::Type;

    {
        std::string* a = new std::string( "A" );
        V dv( T::String, a );
        delete a;
        EXPECT_EQ( T::String, dv.type );
        EXPECT_EQ( 0, dv.value.string->compare( "A" ) );
    }

    {
        V dv( T::String, "B" );
        EXPECT_EQ( T::String, dv.type );
        EXPECT_NE( T::JSON, dv.type );
        EXPECT_EQ( 0, dv.value.string->compare( "B" ) );
    }

    {
        V dv( T::String, std::string( "C" ) );
        EXPECT_EQ( T::String, dv.type );
        EXPECT_EQ( 0, dv.value.string->compare( "C" ) );
    }
}

TEST(DatabaseValue, TestJson) {
	LOG( INFO ) << "Test Database::Value::JSON";
    using V = M::Database::Value;
    using T = V::Type;

    {
        std::string* json = new std::string( "JSON" );
        V dv( T::JSON, json );
        delete json;
        EXPECT_EQ( T::JSON, dv.type );
        EXPECT_NE( T::String, dv.type );
        EXPECT_EQ( 0, dv.value.string->compare( "JSON" ) );
    }

    {
        V dv( T::JSON, "JSON2" );
        EXPECT_EQ( T::JSON, dv.type );
        EXPECT_EQ( 0, dv.value.string->compare( "JSON2" ) );
    }

    {
        V dv( T::JSON, std::string( "JSON3" ) );
        EXPECT_EQ( T::JSON, dv.type );
        EXPECT_EQ( 0, dv.value.string->compare( "JSON3" ) );
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
