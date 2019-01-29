/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2019 University of California, Los Angeles
 *
 * This file is part of ChronoSync, synchronization library for distributed realtime
 * applications for NDN.
 *
 * ChronoSync is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * ChronoSync is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ChronoSync, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "socket.hpp"

#include "tests/boost-test.hpp"
#include "tests/unit-test-time-fixture.hpp"

#include <ndn-cxx/util/dummy-client-face.hpp>

namespace chronosync {
namespace test {

using std::string;
using ndn::util::DummyClientFace;

/**
 * @brief Emulate an app that use the Socket class
 *
 * The app has two types of data set: one is simply string while the other is integer array.
 * For each type of data set, the app has a specific fetching strategy.
 */
class SocketTestApp : noncopyable
{
public:
  SocketTestApp(const Name& syncPrefix,
                const Name& userPrefix,
                DummyClientFace& face,
                bool isNum)
    : sum(0)
    , socket(syncPrefix,
             userPrefix,
             face,
             isNum ? bind(&SocketTestApp::fetchNumbers, this, _1) :
                          bind(&SocketTestApp::fetchAll, this, _1),
             Logic::DEFAULT_NAME,
             Logic::DEFAULT_VALIDATOR,
             Logic::DEFAULT_SYNC_INTEREST_LIFETIME,
             name::Component::fromEscapedString("override"))
  {
  }

  void
  set(const Data& dataPacket)
  {
    // std::cerr << "set Data" << std::endl;
    Name dataName(dataPacket.getName());
    string str2(reinterpret_cast<const char*>(dataPacket.getContent().value()),
                dataPacket.getContent().value_size());
    data.insert(make_pair(dataName, str2));
  }

  void
  set(Name name, const char* buf, int len)
  {
    string str2(buf, len);
    data.insert(make_pair(name, str2));
  }

  void
  setNum(const Data& dataPacket)
  {
    // std::cerr << "setNum Data" << std::endl;
    size_t n = dataPacket.getContent().value_size() / 4;
    const uint32_t* numbers = reinterpret_cast<const uint32_t*>(dataPacket.getContent().value());
    for (size_t i = 0; i < n; i++) {
      sum += numbers[i];
    }
  }

  void
  setNum(Name name, const uint8_t* buf, int len)
  {
    BOOST_ASSERT(len >= 4);

    int n = len / 4;
    const uint32_t* numbers = reinterpret_cast<const uint32_t*>(buf);
    for (int i = 0; i < n; i++) {
      sum += numbers[i];
    }
  }

  void
  fetchAll(const std::vector<MissingDataInfo>& v)
  {
    // std::cerr << "fetchAll" << std::endl;
    for (size_t i = 0; i < v.size(); i++) {
      for(SeqNo s = v[i].low; s <= v[i].high; ++s) {
        socket.fetchData(v[i].session, s, [this] (const Data& dataPacket) {
            this->set(dataPacket);
          });
      }
    }
  }

  void
  fetchNumbers(const std::vector<MissingDataInfo>& v)
  {
    // std::cerr << "fetchNumbers" << std::endl;
    for (size_t i = 0; i < v.size(); i++) {
      for(SeqNo s = v[i].low; s <= v[i].high; ++s) {
        socket.fetchData(v[i].session, s, [this] (const Data& dataPacket) {
            this->setNum(dataPacket);
          });
      }
    }
  }

  string
  toString()
  {
    string str = "\n";
    for (auto it = data.begin(); it != data.end(); ++it) {
      str += "<";
      str += it->first.toUri();
      str += "|";
      str += it->second;
      str += ">";
      str += "\n";
    }

    return str;
  }

public:
  std::map<ndn::Name, string> data;
  uint32_t sum;
  Socket socket;
};

class SocketFixture : public ndn::tests::UnitTestTimeFixture
{
public:
  SocketFixture()
    : syncPrefix("/ndn/broadcast/sync")
  {
    syncPrefix.appendVersion();
    userPrefix[0] = Name("/user0");
    userPrefix[1] = Name("/user1");
    userPrefix[2] = Name("/user2");

    faces[0].reset(new DummyClientFace(io, {true, true}));
    faces[1].reset(new DummyClientFace(io, {true, true}));
    faces[2].reset(new DummyClientFace(io, {true, true}));

    for (int i = 0; i < 3; i++) {
      readInterestOffset[i] = 0;
      readDataOffset[i] = 0;
    }
  }

  void
  passPacket()
  {
    for (int i = 0; i < 3; i++)
      checkFace(i);
  }

  void
  checkFace(int sender)
  {
    while (faces[sender]->sentInterests.size() > readInterestOffset[sender]) {
      for (int i = 0; i < 3; i++) {
        if (sender != i)
          faces[i]->receive(faces[sender]->sentInterests[readInterestOffset[sender]]);
      }
      readInterestOffset[sender]++;
    }
    while (faces[sender]->sentData.size() > readDataOffset[sender]) {
      for (int i = 0; i < 3; i++) {
        if (sender != i)
          faces[i]->receive(faces[sender]->sentData[readDataOffset[sender]]);
      }
      readDataOffset[sender]++;
    }
  }

  void
  createSocket(size_t idx, bool isNum)
  {
    app[idx] = make_shared<SocketTestApp>(syncPrefix, userPrefix[idx], ref(*faces[idx]), isNum);
    sessionName[idx] = app[idx]->socket.getLogic().getSessionName();
  }

  void
  publishAppData(size_t idx, const string& data)
  {
    app[idx]->socket.publishData(reinterpret_cast<const uint8_t*>(data.c_str()), data.size(),
                                 ndn::time::milliseconds(1000));
  }

  void
  publishAppData(size_t idx, const string& data, SeqNo seqNo)
  {
    app[idx]->socket.publishData(reinterpret_cast<const uint8_t*>(data.c_str()), data.size(),
                                 ndn::time::milliseconds(1000), seqNo);
  }

  void
  setAppData(size_t idx, SeqNo seqNo, const string& data)
  {
    Name dataName = sessionName[idx];
    dataName.appendNumber(seqNo);
    app[idx]->set(dataName, data.c_str(), data.size());
  }

  void
  publishAppNum(size_t idx, const uint8_t* buf, size_t size)
  {
    app[idx]->socket.publishData(buf, size, ndn::time::milliseconds(1000));
  }

  void
  setAppNum(size_t idx, SeqNo seqNo, const uint8_t* buf, size_t size)
  {
    Name dataName = sessionName[idx];
    dataName.appendNumber(seqNo);
    app[idx]->setNum(dataName, buf, size);
  }

public:
  Name syncPrefix;
  Name userPrefix[3];
  Name sessionName[3];

  std::unique_ptr<DummyClientFace> faces[3];
  shared_ptr<SocketTestApp> app[3];

  size_t readInterestOffset[3];
  size_t readDataOffset[3];
};

BOOST_FIXTURE_TEST_SUITE(SocketTests, SocketFixture)

BOOST_AUTO_TEST_CASE(BasicData)
{
  createSocket(0, false);
  advanceClocks(ndn::time::milliseconds(10), 5);
  createSocket(1, false);
  advanceClocks(ndn::time::milliseconds(10), 5);
  createSocket(2, false);
  advanceClocks(ndn::time::milliseconds(10), 5);

  string data0 = "Very funny Scotty, now beam down my clothes";
  publishAppData(0, data0);

  for (int i = 0; i < 50; i++) {
    advanceClocks(ndn::time::milliseconds(2), 10);
    passPacket();
  }
  setAppData(0, 1, data0);

  advanceClocks(ndn::time::milliseconds(10), 1);
  BOOST_CHECK_EQUAL(app[0]->toString(), app[1]->toString());
  BOOST_CHECK_EQUAL(app[0]->toString(), app[2]->toString());

  string data1 = "Yes, give me that ketchup";
  string data2 = "Don't look conspicuous, it draws fire";
  publishAppData(0, data1);
  advanceClocks(ndn::time::milliseconds(10), 1);
  publishAppData(0, data2);

  for (int i = 0; i < 50; i++) {
    advanceClocks(ndn::time::milliseconds(2), 10);
    passPacket();
  }
  setAppData(0, 2, data1);
  advanceClocks(ndn::time::milliseconds(10), 1);
  setAppData(0, 3, data2);

  advanceClocks(ndn::time::milliseconds(10), 1);
  BOOST_CHECK_EQUAL(app[0]->toString(), app[1]->toString());
  BOOST_CHECK_EQUAL(app[0]->toString(), app[2]->toString());

  string data3 = "You surf the Internet, I surf the real world";
  string data4 = "I got a fortune cookie once that said 'You like Chinese food'";
  string data5 = "Real men wear pink. Why? Because their wives make them";
  publishAppData(2, data3);
  advanceClocks(ndn::time::milliseconds(10), 2);
  publishAppData(1, data4);
  advanceClocks(ndn::time::milliseconds(10), 1);
  publishAppData(1, data5);

  for (int i = 0; i < 100; i++) {
    advanceClocks(ndn::time::milliseconds(2), 10);
    passPacket();
  }
  setAppData(2, 1, data3);
  advanceClocks(ndn::time::milliseconds(10), 1);
  setAppData(1, 1, data4);
  advanceClocks(ndn::time::milliseconds(10), 1);
  setAppData(1, 2, data5);

  advanceClocks(ndn::time::milliseconds(10), 7);
  BOOST_CHECK_EQUAL(app[0]->toString(), app[1]->toString());
  BOOST_CHECK_EQUAL(app[0]->toString(), app[2]->toString());

  string data6 = "Shakespeare says: 'Prose before hos.'";
  string data7 = "Pick good people, talent never wears out";
  publishAppData(0, data6);
  publishAppData(1, data7);

  for (int i = 0; i < 100; i++) {
    advanceClocks(ndn::time::milliseconds(2), 10);
    passPacket();
  }
  setAppData(0, 4, data6);
  setAppData(1, 3, data7);

  BOOST_CHECK_EQUAL(app[0]->toString(), app[1]->toString());
  BOOST_CHECK_EQUAL(app[0]->toString(), app[2]->toString());

  BOOST_CHECK_EQUAL(sessionName[0], Name("/user0/override"));
  BOOST_CHECK_EQUAL(sessionName[1], Name("/user1/override"));
}

BOOST_AUTO_TEST_CASE(BasicNumber)
{
  createSocket(0, true);
  advanceClocks(ndn::time::milliseconds(10), 5);
  createSocket(1, true);
  advanceClocks(ndn::time::milliseconds(10), 5);

  uint32_t num1[5] = {0, 1, 2, 3, 4};
  uint8_t* buf1 = reinterpret_cast<uint8_t*>(num1);
  size_t size1 = sizeof(num1);
  publishAppNum(0, buf1, size1);
  advanceClocks(ndn::time::milliseconds(10), 5);
  setAppNum(0, 0, buf1, size1);

  for (int i = 0; i < 100; i++) {
    advanceClocks(ndn::time::milliseconds(2), 10);
    passPacket();
  }
  BOOST_CHECK_EQUAL(app[0]->sum, app[1]->sum);
  BOOST_CHECK_EQUAL(app[1]->sum, 10);

  uint32_t num2[5] = {9, 7, 2, 1, 1};
  uint8_t* buf2 = reinterpret_cast<uint8_t*>(num2);
  size_t size2 = sizeof(num2);
  publishAppNum(1, buf2, size2);
  setAppNum(1, 0, buf2, size2);

  for (int i = 0; i < 50; i++) {
    advanceClocks(ndn::time::milliseconds(2), 10);
    passPacket();
  }
  BOOST_CHECK_EQUAL(app[0]->sum, app[1]->sum);
  BOOST_CHECK_EQUAL(app[1]->sum, 30);
}

BOOST_AUTO_TEST_CASE(BasicDataWithAppSeq)
{
  createSocket(0, false);
  advanceClocks(ndn::time::milliseconds(10), 5);
  createSocket(1, false);
  advanceClocks(ndn::time::milliseconds(10), 5);
  createSocket(2, false);
  advanceClocks(ndn::time::milliseconds(10), 5);

  string data0 = "Very funny Scotty, now beam down my clothes";
  publishAppData(0, data0, 100);

  for (int i = 0; i < 50; i++) {
    advanceClocks(ndn::time::milliseconds(2), 10);
    passPacket();
  }
  setAppData(0, 100, data0);

  advanceClocks(ndn::time::milliseconds(10), 1);
  BOOST_CHECK_EQUAL(app[0]->toString(), app[1]->toString());
  BOOST_CHECK_EQUAL(app[0]->toString(), app[2]->toString());

  string data1 = "Yes, give me that ketchup";
  string data2 = "Don't look conspicuous, it draws fire";
  publishAppData(0, data1, 200);
  advanceClocks(ndn::time::milliseconds(10), 1);
  publishAppData(0, data2, 300);

  for (int i = 0; i < 50; i++) {
    advanceClocks(ndn::time::milliseconds(2), 10);
    passPacket();
  }
  setAppData(0, 200, data1);
  advanceClocks(ndn::time::milliseconds(10), 1);
  setAppData(0, 300, data2);

  advanceClocks(ndn::time::milliseconds(10), 1);
  BOOST_CHECK_EQUAL(app[0]->toString(), app[1]->toString());
  BOOST_CHECK_EQUAL(app[0]->toString(), app[2]->toString());

  string data3 = "You surf the Internet, I surf the real world";
  string data4 = "I got a fortune cookie once that said 'You like Chinese food'";
  string data5 = "Real men wear pink. Why? Because their wives make them";
  publishAppData(2, data3, 100);
  advanceClocks(ndn::time::milliseconds(10), 2);
  publishAppData(1, data4, 100);
  advanceClocks(ndn::time::milliseconds(10), 1);
  publishAppData(1, data5, 200);

  for (int i = 0; i < 100; i++) {
    advanceClocks(ndn::time::milliseconds(2), 10);
    passPacket();
  }
  setAppData(2, 100, data3);
  advanceClocks(ndn::time::milliseconds(10), 1);
  setAppData(1, 100, data4);
  advanceClocks(ndn::time::milliseconds(10), 1);
  setAppData(1, 200, data5);

  advanceClocks(ndn::time::milliseconds(10), 7);
  BOOST_CHECK_EQUAL(app[0]->toString(), app[1]->toString());
  BOOST_CHECK_EQUAL(app[0]->toString(), app[2]->toString());

  string data6 = "Shakespeare says: 'Prose before hos.'";
  string data7 = "Pick good people, talent never wears out";
  publishAppData(0, data6, 500);
  publishAppData(1, data7, 300);

  for (int i = 0; i < 100; i++) {
    advanceClocks(ndn::time::milliseconds(2), 10);
    passPacket();
  }
  setAppData(0, 500, data6);
  setAppData(1, 300, data7);

  BOOST_CHECK_EQUAL(app[0]->toString(), app[1]->toString());
  BOOST_CHECK_EQUAL(app[0]->toString(), app[2]->toString());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test
} // namespace chronosync
