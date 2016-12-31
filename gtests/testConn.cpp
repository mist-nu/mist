/*
 * (c) 2016 VISIARC AB
 * 
 * Free software licensed under GPLv3.
 */

#include <exception>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <fstream>

// Logger
/*
#include <memory>
#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>
#include <g3log/std2_make_unique.hpp>
#include "Helper.h"
//*/

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <gtest/gtest.h> // Google test framework

#include "Central.h"
#include "Exception.h"
#include "Database.h"
#include "Transaction.h"
#include "PrivateUserAccount.h"

#include "conn/peer.hpp"
#include "conn/conn.hpp"
#include "conn/service.hpp"

#include "crypto/hash.hpp"
#include "crypto/key.hpp"

#include "io/io_context.hpp"
#include "io/ssl_context.hpp"

namespace M = Mist;
namespace FS = M::Helper::filesystem;

using ORef = M::Database::ObjectRef;
using AD = M::Database::AccessDomain;
using V = M::Database::Value;
using VT = M::Database::Value::Type;

namespace { // Anonymous namespace

mist::io::IOContext ioCtx;

std::unique_ptr<mist::io::SSLContext> sslCtx1;
std::unique_ptr<mist::io::SSLContext> sslCtx2;

std::unique_ptr<mist::ConnectContext> connCtx1;
std::unique_ptr<mist::ConnectContext> connCtx2;

boost::optional<mist::Peer&> other1;
boost::optional<mist::Peer&> other2;

boost::optional<uint16_t> directPort1;
boost::optional<uint16_t> directPort2;

boost::optional<uint16_t> torPort1;
boost::optional<uint16_t> torPort2;

boost::optional<mist::Service&> service1;
boost::optional<mist::Service&> service2;

void writeToFile(std::string filename, const char* data, std::size_t length)
{
  std::ofstream os;
  os.open(filename, std::ios::binary | std::ios::trunc);
  os.write(data, length);
  os.close();
}

TEST(InitConnTest, CreateDb) {
  auto workDir = "workDir";

  sslCtx1 = std::unique_ptr<mist::io::SSLContext>(
    new mist::io::SSLContext(ioCtx, workDir));

  sslCtx2 = std::unique_ptr<mist::io::SSLContext>(
    new mist::io::SSLContext(ioCtx));

  std::string pkcs12_1 = mist::crypto::createP12TestRsaPrivateKeyAndCert(*sslCtx1,
    "", "CN=testone", 2048, mist::crypto::ValidityTimeUnit::Min, 10);

  //writeToFile("pkcs12_1.p12", pkcs12_1.data(), pkcs12_1.length());

  std::string pkcs12_2 = mist::crypto::createP12TestRsaPrivateKeyAndCert(*sslCtx2,
    "", "CN=testtwo", 2048, mist::crypto::ValidityTimeUnit::Min, 10);

  //writeToFile("pkcs12_2.p12", pkcs12_1.data(), pkcs12_1.length());

  sslCtx1->loadPKCS12(pkcs12_1, "");

  sslCtx2->loadPKCS12(pkcs12_2, "");

  connCtx1 = std::unique_ptr<mist::ConnectContext>(
    new mist::ConnectContext(*sslCtx1,
      [](mist::Peer& peer)
  {
    peer.setAuthenticated(true);
  }));

  connCtx2 = std::unique_ptr<mist::ConnectContext>(
    new mist::ConnectContext(*sslCtx2,
      [](mist::Peer& peer)
  {
    peer.setAuthenticated(true);
  }));

  other1 = connCtx1->addAuthenticatedPeer(sslCtx2->derPublicKey());

  other2 = connCtx2->addAuthenticatedPeer(sslCtx1->derPublicKey());
}

class AsyncTest : public std::enable_shared_from_this<AsyncTest>
{
public:

  AsyncTest()
    : _succeeded(false), _failed(false)
  {
  }

  bool hasResult()
  {
    std::unique_lock<std::mutex> lock(_mux);
    return _succeeded || _failed;
  }

  void setResult(bool succeeded)
  {
    std::lock_guard<std::mutex> lock(_mux);
    if (_succeeded || _failed)
      return;
    _succeeded = succeeded;
    _cond.notify_all();
  }

  bool wait()
  {
    std::unique_lock<std::mutex> lock(_mux);
    while (!_succeeded && !_failed) {
      _cond.wait(lock);
    }
    return _succeeded;
  }

  void exec(unsigned int timeout, bool timeoutResult = false)
  {
    auto anchor = shared_from_this();
    ioCtx.queueJob([this, anchor, timeout, timeoutResult]()
    {
      auto start = boost::posix_time::microsec_clock::local_time() \
        .time_of_day().total_milliseconds();
      while (1) {
        if (hasResult())
          return;
        auto now = boost::posix_time::microsec_clock::local_time() \
          .time_of_day().total_milliseconds();
        if (now > start + timeout)
          setResult(timeoutResult);
        ioCtx.ioStep(100);
      }
    });
  }

private:
  std::mutex _mux;
  std::condition_variable _cond;

  bool _succeeded;
  bool _failed;
};

class ConnTest : public ::testing::Test {
public:

  ConnTest()
    : _asyncTest(std::make_shared<AsyncTest>())
  {
  }

  void exec(unsigned int timeout)
  {
    _asyncTest->exec(timeout);
  }

  bool wait()
  {
    return _asyncTest->wait();
  }

  std::shared_ptr<AsyncTest> asyncTest()
  {
    return _asyncTest;
  }

private:

  std::shared_ptr<AsyncTest> _asyncTest;

};

namespace
{
std::uint8_t fromHexNibble(char c)
{
  if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  if (c >= '0' && c <= '9')
    return (c - '0');
  assert(false && "Not a hex digit");
  throw;
}
std::vector<std::uint8_t> fromHexString(const std::string& s)
{
  assert(!(s.length() & 1));
  std::size_t count = s.length() >> 1;
  std::vector<std::uint8_t> out(count);
  for (unsigned n = 0; n != count; ++n) {
    out[n] = (fromHexNibble(s[n << 1]) << 4) + fromHexNibble(s[(n << 1) + 1]);
  }
  return out;
}
}

TEST_F(ConnTest, Hash) {
  LOG(INFO) << "Test sign";

  std::string inData = "abc";

  ASSERT_EQ(mist::crypto::hash_sha3_256()->finalize(
    reinterpret_cast<const std::uint8_t*>(inData.data()),
    reinterpret_cast<const std::uint8_t*>(inData.data() + inData.size())),
    fromHexString("3a985da74fe225b2045c172d6bd390bd"
      "855f086e3e9d525b46bfe24511431532"));

  ASSERT_EQ(mist::crypto::hash_sha3_384()->finalize(
    reinterpret_cast<const std::uint8_t*>(inData.data()),
    reinterpret_cast<const std::uint8_t*>(inData.data() + inData.size())),
    fromHexString("ec01498288516fc926459f58e2c6ad8d"
      "f9b473cb0fc08c2596da7cf0e49be4b2"
      "98d88cea927ac7f539f1edf228376d25"));

  ASSERT_EQ(mist::crypto::hash_sha3_512()->finalize(
    reinterpret_cast<const std::uint8_t*>(inData.data()),
    reinterpret_cast<const std::uint8_t*>(inData.data() + inData.size())),
    fromHexString("b751850b1a57168a5693cd924b6b096e"
      "08f621827444f70d884f5d0240d2712e"
      "10e116e9192af3c91a7ec57647e39340"
      "57340b4cf408d5a56592f8274eec53f0"));

  inData = "";

  ASSERT_EQ(mist::crypto::hash_sha3_256()->finalize(
    reinterpret_cast<const std::uint8_t*>(inData.data()),
    reinterpret_cast<const std::uint8_t*>(inData.data() + inData.size())),
    fromHexString("a7ffc6f8bf1ed76651c14756a061d662"
      "f580ff4de43b49fa82d80a4b80f8434a"));

  ASSERT_EQ(mist::crypto::hash_sha3_384()->finalize(
    reinterpret_cast<const std::uint8_t*>(inData.data()),
    reinterpret_cast<const std::uint8_t*>(inData.data() + inData.size())),
    fromHexString("0c63a75b845e4f7d01107d852e4c2485"
      "c51a50aaaa94fc61995e71bbee983a2a"
      "c3713831264adb47fb6bd1e058d5f004"));

  ASSERT_EQ(mist::crypto::hash_sha3_512()->finalize(
    reinterpret_cast<const std::uint8_t*>(inData.data()),
    reinterpret_cast<const std::uint8_t*>(inData.data() + inData.size())),
    fromHexString("a69f73cca23a9ac5c8b567dc185a756e"
      "97c982164fe25859e0d1dcc1475c80a6"
      "15b2123af1f5f94c11e3e9402c3ac558"
      "f500199d95b6d3e301758586281dcd26"));

  inData = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

  ASSERT_EQ(mist::crypto::hash_sha3_256()->finalize(
    reinterpret_cast<const std::uint8_t*>(inData.data()),
    reinterpret_cast<const std::uint8_t*>(inData.data() + inData.size())),
    fromHexString("41c0dba2a9d6240849100376a8235e2c"
      "82e1b9998a999e21db32dd97496d3376"));

  ASSERT_EQ(mist::crypto::hash_sha3_384()->finalize(
    reinterpret_cast<const std::uint8_t*>(inData.data()),
    reinterpret_cast<const std::uint8_t*>(inData.data() + inData.size())),
    fromHexString("991c665755eb3a4b6bbdfb75c78a492e"
      "8c56a22c5c4d7e429bfdbc32b9d4ad5a"
      "a04a1f076e62fea19eef51acd0657c22"));

  ASSERT_EQ(mist::crypto::hash_sha3_512()->finalize(
    reinterpret_cast<const std::uint8_t*>(inData.data()),
    reinterpret_cast<const std::uint8_t*>(inData.data() + inData.size())),
    fromHexString("04a371e84ecfb5b8b77cb48610fca818"
      "2dd457ce6f326a0fd3d7ec2f1e91636d"
      "ee691fbe0c985302ba1b0d8dc78c0863"
      "46b533b49c030d99a27daf1139d6e75e"));
}

void testHash(const std::string& hex)
{
  auto hash = fromHexString(hex);

  auto sign1 = sslCtx1->sign(hash.data(), hash.size());
  ASSERT_TRUE(other2->verify(hash.data(), hash.size(), sign1.data(), sign1.size()));

  auto sign2 = sslCtx2->sign(hash.data(), hash.size());
  ASSERT_TRUE(other1->verify(hash.data(), hash.size(), sign2.data(), sign2.size()));

  ASSERT_FALSE(other1->verify(hash.data(), hash.size(), sign1.data(), sign1.size()));
  ASSERT_FALSE(other2->verify(hash.data(), hash.size(), sign2.data(), sign2.size()));
}

TEST_F(ConnTest, Sign) {
  LOG(INFO) << "Test sign";

  testHash("90bd855f");
  testHash("");
  testHash("ffffffff");
  testHash("00");
}

TEST_F(ConnTest, Serve) {
  LOG(INFO) << "Test serve";

  auto test = asyncTest();

  directPort1 = connCtx1->serveDirect({ { 9000, 9010 } });
  directPort2 = connCtx2->serveDirect({ { 9000, 9010 } });

  ASSERT_GE(*directPort1, 9000);
  ASSERT_LE(*directPort1, 9010);

  ASSERT_GE(*directPort2, 9000);
  ASSERT_LE(*directPort2, 9010);

  ASSERT_NE(*directPort1, *directPort2);
}

//TEST_F(ConnTest, Service) {
//  LOG(INFO) << "Test service";
//
//  auto test = asyncTest();
//
//  /* Service1 */
//  bool firstConnect1 = false;
//
//  service1 = connCtx1->newService("chat");
//
//  service1->setOnPeerConnectionStatus(
//    [test, firstConnect1](mist::Peer& peer,
//      mist::Peer::ConnectionStatus status) mutable
//  {
//    if (!firstConnect1)
//      return;
//    firstConnect1 = true;
//    LOG(INFO) << "Service1 got connection";
//    if (status != mist::Peer::ConnectionStatus::Connected) {
//      test->setResult(false);
//      return;
//    }
//
//    service1->submitRequest(*other1, "get", "hoj",
//      [test](mist::Peer& peer, mist::h2::ClientRequest request)
//    {
//      LOG(INFO) << "Service1 submitted request";
//      if (&peer != &*other1) {
//        test->setResult(false);
//        return;
//      }
//    });
//  });
//
//  service1->setOnPeerRequest(
//    [test](mist::Peer& peer, mist::h2::ServerRequest request, std::string path)
//  {
//    LOG(INFO) << "Service1 got peer request";
//    if (&peer != &*other1) {
//      test->setResult(false);
//      return;
//    }
//    if (path != "hej") {
//      test->setResult(false);
//      return;
//    }
//  });
//
//  /* Service2 */
//  bool firstConnect2 = false;
//
//  service2 = connCtx2->newService("chat");
//
//  service2->setOnPeerConnectionStatus(
//    [test, firstConnect2](mist::Peer& peer,
//      mist::Peer::ConnectionStatus status) mutable
//  {
//    if (!firstConnect2)
//      return;
//    firstConnect2 = true;
//    LOG(INFO) << "Service2 got connection";
//    if (status != mist::Peer::ConnectionStatus::Connected) {
//      test->setResult(false);
//      return;
//    }
//
//    service2->submitRequest(*other2, "get", "hej",
//      [test](mist::Peer& peer, mist::h2::ClientRequest request)
//    {
//      LOG(INFO) << "Service2 submitted request";
//      if (&peer != &*other2) {
//        test->setResult(false);
//        return;
//      }
//    });
//  });
//
//  service2->setOnPeerRequest(
//    [test](mist::Peer& peer, mist::h2::ServerRequest request, std::string path)
//  {
//    LOG(INFO) << "Service2 got peer request";
//    if (&peer != &*other2) {
//      test->setResult(false);
//      return;
//    }
//    if (path != "hoj") {
//      test->setResult(false);
//      return;
//    }
//  });
//
//  directPort1 = connCtx1->serveDirect({ { 9000, 9010 } });
//  directPort2 = connCtx2->serveDirect({ { 9000, 9010 } });
//
//  ASSERT_GE(*directPort1, 9000);
//  ASSERT_LE(*directPort1, 9010);
//
//  ASSERT_GE(*directPort2, 9000);
//  ASSERT_LE(*directPort2, 9010);
//
//  ASSERT_NE(*directPort1, *directPort2);
//
//  connCtx1->connectPeerDirect(*other1, "127.0.0.1", *directPort1);
//  connCtx2->connectPeerDirect(*other2, "127.0.0.1", *directPort2);
//}

}
