/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <map>
#include <string>
#include <sstream>

#include <gtest/gtest.h> // Google test framework

#include "Database.h"
#include "ExchangeFormat.h"
#include "Exception.h"
#include "Helper.h"

namespace {

const std::string db_file{ "exhangeFormatTest.db" };

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

    using M::Database::writeToDatabase;

    using M::Database::readTransaction;
    using M::Database::readTransactionList;
    using M::Database::readTransactionMetadata;
    using M::Database::readTransactionMetadataLastest;
    using M::Database::readTransactionMetadataFrom;

    using M::Database::readUser;
    using M::Database::readUsers;
    using M::Database::readUsersFrom;
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

TEST(InitExhangeFormatTest, CreateDb) {
    FS::path p { db_file };
    removeTestDb( p );
    LOG ( INFO ) << "Creating db: " << p;
    D db( nullptr, p.make_preferred().string() );
    db.create( 0, nullptr );
    db.close();
}

std::string pubKey0{
    "-----BEGIN PUBLIC KEY-----\\n"
    "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCqGKukO1De7zhZj6+H0qtjTkVxwTCpvKe4eCZ0\\n"
    "FPqri0cb2JZfXJ/DgYSF6vUpwmJG8wVQZKjeGcjDOL5UlsuusFncCzWBQ7RKNUSesmQRMSGkVb1/\\n"
    "3j+skZ6UtW+5u09lHNsj6tQ51s1SPrCBkedbNf0Tp0GbMJDyR4e9T04ZZwIDAQAB\\n"
    "-----END PUBLIC KEY-----\\n"
};

std::string pubKey1{
    "-----BEGIN PUBLIC KEY-----\\n"
    "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDMYfnvWtC8Id5bPKae5yXSxQTt\\n"
    "+Zpul6AnnZWfI2TtIarvjHBFUtXRo96y7hoL4VWOPKGCsRqMFDkrbeUjRrx8iL91\\n"
    "4/srnyf6sh9c8Zk04xEOpK1ypvBz+Ks4uZObtjnnitf0NBGdjMKxveTq+VE7BWUI\\n"
    "yQjtQ8mbDOsiLLvh7wIDAQAB\\n"
    "-----END PUBLIC KEY-----\\n"
};

std::map<int,std::string> t{};

TEST(InitExchangeFormatTest, Strings ) {
    LOG( INFO ) << "Initiating exchange format test.";
    t.emplace( 10, std::string{
        R"({)"
            R"("id":"10",)"
            R"("signature":"11",)"
            R"("transaction":{)"
                R"("metadata":{)"
                    R"("accessDomain":2,)"
                    R"("timestamp":"2000-01-01 00:00",)"
                    R"("user":"12",)"
                    R"("parents":{},)"
                    R"("version":"1.0")"
                R"(},)"
                R"("objects":{)"
                    R"("changed":{},)"
                    R"("deleted":{},)"
                    R"("moved":{},)"
                    R"("new":{})"
                R"(})"
            R"(})"
        R"(})"
    } );

    t.emplace( 20, std::string{
        R"({)"
            R"("id":"20",)"
            R"("signature":"21",)"
            R"("transaction":{)"
                R"("metadata":{)"
                    R"("accessDomain":2,)"
                    R"("timestamp":"2000-01-01 00:00",)"
                    R"("user":"22",)"
                    R"("parents":{"MTA="},)"
                    R"("version":"1.0")"
                R"(},)"
                R"("objects":{)"
                    R"("changed":{},)"
                    R"("deleted":{},)"
                    R"("moved":{},)"
                    R"("new":{)"
                        R"("23":{"parent":0,"attributes":{"example_name":"text","two":2}})"
                    R"(})"
                R"(})"
            R"(})"
        R"(})"
    } );

    t.emplace( 30, std::string{
        R"({)"
            R"("id":"30",)"
            R"("signature":"31",)"
            R"("transaction":{)"
                R"("metadata":{)"
                    R"("accessDomain":2,)"
                    R"("timestamp":"2000-01-01 00:00",)"
                    R"("user":"32",)"
                    R"("parents":{"20":true},)"
                    R"("version":"1.0")"
                R"(},)"
                R"("objects":{)"
                    R"("changed":{},)"
                    R"("deleted":{},)"
                    R"("moved":{},)"
                    R"("new":{)"
                        R"("33":{"parent":23,"attributes":{"true":1}},)"
                        R"("34":{"parent":23,"attributes":{"1":1}},)"
                        R"("35":{"parent":23,"attributes":{"null":0}},)"
                        R"("36":{"parent":23,"attributes":{"text":"four"}})"
                    R"(})"
                R"(})"
            R"(})"
        R"(})"
    } );

    t.emplace( 40, std::string{
        R"({)"
            R"("id":"40",)"
            R"("signature":"41",)"
            R"("transaction":{)"
                R"("metadata":{)"
                    R"("accessDomain":2,)"
                    R"("timestamp":"2000-01-01 00:00",)"
                    R"("user":"42",)"
                    R"("parents":{"30":true},)"
                    R"("version":"1.0")"
                R"(},)"
                R"("objects":{)"
                    R"("changed":{)"
                        R"("33":{"attributes":{"changed":4}})"
                    R"(},)"
                    R"("deleted":{},)"
                    R"("moved":{},)"
                    R"("new":{})"
                R"(})"
            R"(})"
        R"(})"
    } );

    t.emplace( 50, std::string{
        R"({)"
            R"("id":"50",)"
            R"("signature":"51",)"
            R"("transaction":{)"
                R"("metadata":{)"
                    R"("accessDomain":2,)"
                    R"("timestamp":"2000-01-01 00:00",)"
                    R"("user":"52",)"
                    R"("parents":{"40":true},)"
                    R"("version":"1.0")"
                R"(},)"
                R"("objects":{)"
                    R"("changed":{)"
                        R"("33":{"attributes":{"changed":5.0}},)"
                        R"("34":{"attributes":{"changed":5.0}})"
                    R"(},)"
                    R"("deleted":{},)"
                    R"("moved":{},)"
                    R"("new":{})"
                R"(})"
            R"(})"
        R"(})"
    } );

    t.emplace( 60, std::string{
        R"({)"
            R"("id":"60",)"
            R"("signature":"61",)"
            R"("transaction":{)"
                R"("metadata":{)"
                    R"("accessDomain":2,)"
                    R"("timestamp":"2000-01-01 00:00",)"
                    R"("user":"62",)"
                    R"("parents":{"50":true},)"
                    R"("version":"1.0")"
                R"(},)"
                R"("objects":{)"
                    R"("changed":{},)"
                    R"("deleted":{)"
                        R"("36":true)"
                    R"(},)"
                    R"("moved":{},)"
                    R"("new":{})"
                R"(})"
            R"(})"
        R"(})"
    } );

    t.emplace( 70, std::string{
        R"({)"
            R"("id":"70",)"
            R"("signature":"71",)"
            R"("transaction":{)"
                R"("metadata":{)"
                    R"("accessDomain":2,)"
                    R"("timestamp":"2000-01-01 00:00",)"
                    R"("user":"72",)"
                    R"("parents":{"60":true},)"
                    R"("version":"1.0")"
                R"(},)"
                R"("objects":{)"
                    R"("changed":{},)"
                    R"("deleted":{)"
                        R"("35":true)"
                    R"(},)"
                    R"("moved":{},)"
                    R"("new":{})"
                R"(})"
            R"(})"
        R"(})"
    } );

    t.emplace( 80, std::string{
        R"({)"
            R"("id":"80",)"
            R"("signature":"81",)"
            R"("transaction":{)"
                R"("metadata":{)"
                    R"("accessDomain":2,)"
                    R"("timestamp":"2000-01-01 00:00",)"
                    R"("user":"82",)"
                    R"("parents":{"70":true},)"
                    R"("version":"1.0")"
                R"(},)"
                R"("objects":{)"
                    R"("changed":{},)"
                    R"("deleted":{},)"
                    R"("moved":{)"
                        R"("33":36)"
                    R"(},)"
                    R"("new":{})"
                R"(})"
            R"(})"
        R"(})"
    } );

    t.emplace( 90, std::string{
        R"({)"
            R"("id":"90",)"
            R"("signature":"91",)"
            R"("transaction":{)"
                R"("metadata":{)"
                    R"("accessDomain":2,)"
                    R"("timestamp":"2000-01-01 00:00",)"
                    R"("user":"92",)"
                    R"("parents":{"80":true},)"
                    R"("version":"1.0")"
                R"(},)"
                R"("objects":{)"
                    R"("changed":{},)"
                    R"("deleted":{},)"
                    R"("moved":{)"
                        R"("33":0,)"
                        R"("36":33)"
                    R"(},)"
                    R"("new":{})"
                R"(})"
            R"(})"
        R"(})"
    } );

    std::string user100Start{
        R"({)"
            R"("id":"100",)"
            R"("signature":"101",)"
            R"("transaction":{)"
                R"("metadata":{)"
                    R"("accessDomain":1,)"
                    R"("timestamp":"2000-01-01 00:00",)"
                    R"("user":"102",)"
                    R"("parents":{},)"
                    R"("version":"1.0")"
                R"(},)"
                R"("objects":{)"
                    R"("changed":{},)"
                    R"("deleted":{},)"
                    R"("moved":{},)"
                    R"("new":{)"
                        R"("1024":{"parent":1,"attributes":{)"
                            R"("id":"1000","name":"userName1","permission":"admin","publicKey":")"
    };

    std::string user100End{
                        R"("}})"
                    R"(})"
                R"(})"
            R"(})"
        R"(})"
    };

    t.emplace( 100, std::string{ user100Start + pubKey0 + user100End } );

    std::string user110Start{
        R"({)"
            R"("id":"110",)"
            R"("signature":"111",)"
            R"("transaction":{)"
                R"("metadata":{)"
                    R"("accessDomain":1,)"
                    R"("timestamp":"2000-01-01 00:00",)"
                    R"("user":"112",)"
                    R"("parents":{},)"
                    R"("version":"1.0")"
                R"(},)"
                R"("objects":{)"
                    R"("changed":{},)"
                    R"("deleted":{},)"
                    R"("moved":{},)"
                    R"("new":{)"
                        R"("1025":{"parent":1,"attributes":{)"
                            R"("id":"1001","name":"userName2","permission":"admin","publicKey":")"
    };

    std::string user110End{
                        R"("}})"
                    R"(})"
                R"(})"
            R"(})"
        R"(})"
    };

    t.emplace( 110, std::string{ user110Start + pubKey1 + user110End } );
    LOG( INFO ) << "Exchange format initiation done.";
}

class ExchangeFormatTest : public ::testing::Test {
public:
    ExchangeFormatTest() {
        db.init();
    }
    virtual ~ExchangeFormatTest() {}

    FS::path p{ db_file };
    D db{ nullptr, p.make_preferred().string() };
};


std::string getMeta( std::string str ) {
    str.erase( str.find( ",\"objects\":{" ), str.length() );
    str += "}}";
    return str;
}

TEST_F( ExchangeFormatTest, writeMinimalTransaction ) {
    LOG( DBUG ) << "Testing minimal transaction.";
    std::stringstream ss{ t[10] };
    db.writeToDatabase( *ss.rdbuf() );
}

TEST_F( ExchangeFormatTest, newObject ) {
    LOG( DBUG ) << "New Object";
    std::stringstream ss{ t[20] };
    db.writeToDatabase( *ss.rdbuf() );
}

TEST_F( ExchangeFormatTest, fourNewObjects ) {
    LOG( DBUG ) << "Four new objects";
    std::stringstream ss{ t[30] };
    db.writeToDatabase( *ss.rdbuf() );
}

TEST_F( ExchangeFormatTest, objectChanged ) {
    LOG( DBUG ) << "Object changed";
    std::stringstream ss{ t[40] };
    db.writeToDatabase( *ss.rdbuf() );
}

TEST_F( ExchangeFormatTest, twoObjectsChanged ) {
    LOG( DBUG ) << "Two objects changed";
    std::stringstream ss{ t[50] };
    db.writeToDatabase( *ss.rdbuf() );
}

TEST_F( ExchangeFormatTest, objectDeleted ) {
    LOG( DBUG ) << "object deleted.";
    std::stringstream ss{ t[60] };
    db.writeToDatabase( *ss.rdbuf() );
}

TEST_F( ExchangeFormatTest, oneMoreObjectsDeleted ) {
    LOG( DBUG ) << "One more object deleted";
    std::stringstream ss{ t[70] };
    db.writeToDatabase( *ss.rdbuf() );
}

TEST_F( ExchangeFormatTest, objectMoved ) {
    LOG( DBUG ) << "Object moved";
    std::stringstream ss{ t[80] };
    db.writeToDatabase( *ss.rdbuf() );
}

TEST_F( ExchangeFormatTest, twoObjectsMoved ) {
    LOG( DBUG ) << "Two objects moved.";
    std::stringstream ss{ t[90] };
    db.writeToDatabase( *ss.rdbuf() );
}

TEST_F( ExchangeFormatTest, addUsers ) {
    LOG( DBUG ) << "Add user";
    std::stringstream ss1{ t[100] };
    db.writeToDatabase( *ss1.rdbuf() );
    std::stringstream ss2{ t[110] };
    db.writeToDatabase( *ss2.rdbuf() );
}

TEST_F( ExchangeFormatTest, readTransactionMetadata ) {
    LOG( DBUG ) << "Read transaction meta data";
    std::stringstream ss{};
    db.readTransactionMetadata( *ss.rdbuf(), "20" );
    EXPECT_EQ( getMeta( t[20] ), ss.str() );
}


TEST_F( ExchangeFormatTest, readTransactionLatest ) {
    LOG( DBUG ) << "Read latest transaction meta data";
    std::stringstream ss{};
    ASSERT_TRUE( ss.str().empty() );
    db.readTransactionMetadataLastest( *ss.rdbuf() );
    EXPECT_EQ( getMeta( t[90] ), ss.str() ); // hash sorted as string therefore "90" > "110"
}

TEST_F( ExchangeFormatTest, readTransactionList ) {
    LOG( DBUG ) << "Read transaction list";
    std::string should_work_when_all_other_transactions_work{ "[" };

    for( auto trans: t ) {
        should_work_when_all_other_transactions_work += getMeta( trans.second ) + ",";
    }
    should_work_when_all_other_transactions_work.pop_back();
    should_work_when_all_other_transactions_work += "]";

    std::stringstream ss{};
    ASSERT_TRUE( ss.str().empty() );
    db.readTransactionList( *ss.rdbuf() );
    EXPECT_EQ( should_work_when_all_other_transactions_work, ss.str() );
}

TEST_F( ExchangeFormatTest, readTransaction ) {
    LOG( DBUG ) << "Read transaction";
    std::stringstream ss{};
    ASSERT_TRUE( ss.str().empty() );
    db.readTransaction( *ss.rdbuf(), "90" );
    EXPECT_EQ( t[90], ss.str() );
}

TEST_F( ExchangeFormatTest, readUser ) {
    LOG( DBUG ) << "Read user";
    std::string serializedUser{ R"({"id":"1000","name":"userName1","permission":"admin","publicKey":")" + pubKey0 + R"("})" };
    std::stringstream ss{};
    ASSERT_TRUE( ss.str().empty() );
    db.readUser( *ss.rdbuf(), "1000" );
    EXPECT_EQ( serializedUser, ss.str() );
}

TEST_F( ExchangeFormatTest, readUsers ) {
    LOG( DBUG ) << "Read user list";
    std::string serializedUserList{ "["
        R"({"id":"1000","name":"userName1","permission":"admin","publicKey":")" + pubKey0 + R"("},)"
        R"({"id":"1001","name":"userName2","permission":"admin","publicKey":")" + pubKey1 + R"("})"
        "]"
    };
    std::stringstream ss{};
    db.readUsers( *ss.rdbuf() );
    EXPECT_EQ( serializedUserList, ss.str() );
}

TEST_F( ExchangeFormatTest, readUsersFromBoth ) {
    LOG( DBUG ) << "Read specified users";
    std::string serializedUserList{ "["
        R"({"id":"1000","name":"userName1","permission":"admin","publicKey":")" + pubKey0 + R"("},)"
        R"({"id":"1001","name":"userName2","permission":"admin","publicKey":")" + pubKey1 + R"("})"
        "]"
    };
    std::stringstream ss{};
    db.readUsersFrom( *ss.rdbuf(), std::vector<std::string>{ "1000", "1001" } );
    EXPECT_EQ( serializedUserList, ss.str() );
}

TEST_F( ExchangeFormatTest, readUsersFromOne ) {
    LOG( DBUG ) << "Read some users";
    std::string serializedUserList{ "["
        R"({"id":"1000","name":"userName1","permission":"admin","publicKey":")" + pubKey0 + R"("})"
        "]"
    };
    std::stringstream ss{};
    db.readUsersFrom( *ss.rdbuf(), std::vector<std::string>{ "1000" } );
    EXPECT_EQ( serializedUserList, ss.str() );
}

}
