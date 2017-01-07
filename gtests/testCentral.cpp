/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <map>
#include <string>
#include <vector>

#include "Helper.h"
#include "Database.h"
#include "Central.h"
#include "gtest/gtest.h"

namespace M = Mist;
namespace FS = M::Helper::filesystem;

namespace
{

void tryRemoveFile(std::string p) {
    FS::path filename(p);
    if (FS::exists(filename)) {
        FS::remove(filename);
    }
}

void tryRemoveDb(std::string p) {
    tryRemoveFile(p);
    tryRemoveFile(p + "-wal");
    tryRemoveFile(p + "-shm");
}

void removeCentral(std::string p) {
    LOG(INFO) << "Removing central: " << p;
    tryRemoveDb(p + "/settings.db");
    tryRemoveDb(p + "/content.db");
    tryRemoveDb(p + ".tmp/settings.db");
    tryRemoveDb(p + ".tmp/content.db");
}

void createTestCentral(FS::path p) {
    removeCentral(p.string());

    M::Central central(p.string(), true);
    M::Database* db;

    central.create();
    central.init();
    central.close();
}

} // namespace

TEST(InitRestTest, CreateCentral) {
    createTestCentral(FS::path("central"));
}

class CentralTest: public ::testing::Test {
public:
    CentralTest() : central("central", true) {
        central.init();
    }

    M::Central central;
};

TEST_F(CentralTest, Peer) {
    LOG( INFO ) << "Test Central Peer";
    std::string pemPubKey1("-----BEGIN PUBLIC KEY-----\n"
        "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCqGKukO1De7zhZj6+H0qtjTkVxwTCpvKe4eCZ0\n"
        "FPqri0cb2JZfXJ/DgYSF6vUpwmJG8wVQZKjeGcjDOL5UlsuusFncCzWBQ7RKNUSesmQRMSGkVb1/\n"
        "3j+skZ6UtW+5u09lHNsj6tQ51s1SPrCBkedbNf0Tp0GbMJDyR4e9T04ZZwIDAQAB\n"
        "-----END PUBLIC KEY-----\n");
    std::string pemPubKey2("-----BEGIN PUBLIC KEY-----\n"
        "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDMYfnvWtC8Id5bPKae5yXSxQTt\n"
        "+Zpul6AnnZWfI2TtIarvjHBFUtXRo96y7hoL4VWOPKGCsRqMFDkrbeUjRrx8iL91\n"
        "4/srnyf6sh9c8Zk04xEOpK1ypvBz+Ks4uZObtjnnitf0NBGdjMKxveTq+VE7BWUI\n"
        "yQjtQ8mbDOsiLLvh7wIDAQAB\n"
        "-----END PUBLIC KEY-----\n");
    auto pubKey1(Mist::CryptoHelper::PublicKey::fromPem(pemPubKey1));
    auto pubKey2(Mist::CryptoHelper::PublicKey::fromPem(pemPubKey2));
    central.addPeer(pubKey1, "testPeer1", Mist::PeerStatus::DirectAnonymous, false);

    auto peer1(central.getPeer(pubKey1.hash()));
    ASSERT_EQ(peer1.id, pubKey1.hash());
    ASSERT_EQ(peer1.key, pubKey1);
    ASSERT_EQ(peer1.name, "testPeer1");
    ASSERT_EQ(peer1.status, Mist::PeerStatus::DirectAnonymous);
    ASSERT_EQ(peer1.anonymous, false);

    auto peers(central.listPeers());
    ASSERT_EQ(peers.size(), 1u);
    ASSERT_EQ(peers[0].id, pubKey1.hash());
    ASSERT_EQ(peers[0].key, pubKey1);
    ASSERT_EQ(peers[0].name, "testPeer1");
    ASSERT_EQ(peers[0].status, Mist::PeerStatus::DirectAnonymous);
    ASSERT_EQ(peers[0].anonymous, false);

    central.addPeer(pubKey2, "testPeer2", Mist::PeerStatus::Direct, true);

    auto peer2(central.getPeer(pubKey2.hash()));
    ASSERT_EQ(peer2.id, pubKey2.hash());
    ASSERT_EQ(peer2.key, pubKey2);
    ASSERT_EQ(peer2.name, "testPeer2");
    ASSERT_EQ(peer2.status, Mist::PeerStatus::Direct);
    ASSERT_EQ(peer2.anonymous, true);

    ASSERT_EQ(central.listPeers().size(), 2u);

    central.removePeer(pubKey1.hash());

    peers = central.listPeers();
    ASSERT_EQ(peers.size(), 1u);
    ASSERT_EQ(peers[0].id, pubKey2.hash());
    ASSERT_EQ(peers[0].key, pubKey2);
    ASSERT_EQ(peers[0].name, "testPeer2");
    ASSERT_EQ(peers[0].status, Mist::PeerStatus::Direct);
    ASSERT_EQ(peers[0].anonymous, true);

    central.removePeer(pubKey2.hash());
    ASSERT_EQ(central.listPeers().size(), 0u);
}

TEST_F(CentralTest, ServicePermission) {
    LOG(INFO) << "Test Central AddressLookupServer";
    std::string pemPubKey1("-----BEGIN PUBLIC KEY-----\n"
        "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCqGKukO1De7zhZj6+H0qtjTkVxwTCpvKe4eCZ0\n"
        "FPqri0cb2JZfXJ/DgYSF6vUpwmJG8wVQZKjeGcjDOL5UlsuusFncCzWBQ7RKNUSesmQRMSGkVb1/\n"
        "3j+skZ6UtW+5u09lHNsj6tQ51s1SPrCBkedbNf0Tp0GbMJDyR4e9T04ZZwIDAQAB\n"
        "-----END PUBLIC KEY-----\n");
    std::string pemPubKey2("-----BEGIN PUBLIC KEY-----\n"
        "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDMYfnvWtC8Id5bPKae5yXSxQTt\n"
        "+Zpul6AnnZWfI2TtIarvjHBFUtXRo96y7hoL4VWOPKGCsRqMFDkrbeUjRrx8iL91\n"
        "4/srnyf6sh9c8Zk04xEOpK1ypvBz+Ks4uZObtjnnitf0NBGdjMKxveTq+VE7BWUI\n"
        "yQjtQ8mbDOsiLLvh7wIDAQAB\n"
        "-----END PUBLIC KEY-----\n");
    auto pubKey1(Mist::CryptoHelper::PublicKey::fromPem(pemPubKey1));
    auto pubKey2(Mist::CryptoHelper::PublicKey::fromPem(pemPubKey2));

    central.addServicePermission(pubKey1.hash(), "myChat", "instance0");
    central.addServicePermission(pubKey1.hash(), "myChat", "instance1");
    central.addServicePermission(pubKey2.hash(), "myChat", "instance2");

    std::map<std::string, std::vector<std::string>> expected{
        { "myChat", { "instance0", "instance1" } }
    };
    ASSERT_EQ(central.listServicePermissions(pubKey1.hash()),
        expected);

    expected = {
        { "myChat", { "instance2" } }
    };
    ASSERT_EQ(central.listServicePermissions(pubKey2.hash()),
        expected);

    central.removeServicePermission(pubKey1.hash(), "myChat", "instance0");

    expected = {
        { "myChat", { "instance1" } }
    };
    ASSERT_EQ(central.listServicePermissions(pubKey1.hash()),
        expected);

    central.removeServicePermission(pubKey1.hash(), "myChat", "instance1");
    ASSERT_EQ(central.listServicePermissions(pubKey1.hash()).size(), 0u);

    central.removeServicePermission(pubKey2.hash(), "myChat", "instance2");
    ASSERT_EQ(central.listServicePermissions(pubKey2.hash()).size(), 0u);
}

// TEST_F(CentralTest, AddressLookupServers) {
//     LOG(INFO) << "Test Central AddressLookupServers";
//     central.addAddressLookupServer("www.hej.se", 8080);
//     std::vector<std::pair<std::string, std::uint16_t>> expected{ { "www.hej.se", 8080} };
//     ASSERT_EQ(central.listAddressLookupServers(), expected);
//     central.removeAddressLookupServer("www.hej.se", 8080);
//     ASSERT_EQ(central.listAddressLookupServers().size(), 0u);
// }
