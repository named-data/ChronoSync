/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2023 University of California, Los Angeles
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

#include "logic.hpp"

#include "tests/boost-test.hpp"

namespace chronosync {
namespace test {

class Handler
{
public:
  Handler(ndn::Face& face, const Name& syncPrefix, const Name& userPrefix)
    : logic(face, syncPrefix, userPrefix, [] (auto&&...) {})
  {
  }

  void
  updateSeqNo(const SeqNo& seqNo)
  {
    logic.updateSeqNo(seqNo);
  }

  void
  addUserNode(const Name& prefix)
  {
    logic.addUserNode(prefix);
  }

  void
  removeUserNode(const Name& prefix)
  {
    logic.removeUserNode(prefix);
  }

  Logic logic;
  std::map<Name, SeqNo> map;
};

class MultiUserFixture
{
public:
  MultiUserFixture()
    : syncPrefix("/ndn/broadcast/sync")
    , scheduler(io)
  {
    syncPrefix.appendVersion();
    userPrefix[0] = Name("/user0");
    userPrefix[1] = Name("/user1");
    userPrefix[2] = Name("/user2");

    face = std::make_shared<ndn::Face>(io);
  }

  Name syncPrefix;
  Name userPrefix[3];

  boost::asio::io_context io;
  shared_ptr<ndn::Face> face;
  ndn::Scheduler scheduler;
  shared_ptr<Handler> handler;
};

BOOST_FIXTURE_TEST_SUITE(MultiUserTests, MultiUserFixture)

BOOST_AUTO_TEST_CASE(ThreeUserNode)
{
  handler = std::make_shared<Handler>(*face, syncPrefix, userPrefix[0]);
  handler->addUserNode(userPrefix[1]);
  handler->addUserNode(userPrefix[2]);
  handler->removeUserNode(userPrefix[0]);

  handler->logic.setDefaultUserPrefix(userPrefix[1]);
  handler->updateSeqNo(1);
  BOOST_CHECK_EQUAL(handler->logic.getSeqNo(userPrefix[1]), 1);

  handler->logic.updateSeqNo(2, userPrefix[2]);
  handler->logic.setDefaultUserPrefix(userPrefix[2]);

  BOOST_CHECK_EQUAL(handler->logic.getSeqNo(), 2);

  BOOST_CHECK_THROW(handler->logic.getSeqNo(userPrefix[0]), Logic::Error);
  BOOST_CHECK_THROW(handler->logic.getSessionName(userPrefix[0]), Logic::Error);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test
} // namespace chronosync
