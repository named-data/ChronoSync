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

#include "state.hpp"

#include "tests/boost-test.hpp"

namespace chronosync {
namespace test {

BOOST_AUTO_TEST_SUITE(StateTests)

BOOST_AUTO_TEST_CASE(Basic)
{
  BOOST_CHECK_NO_THROW(State());
  State state;
  BOOST_CHECK_EQUAL(state.getLeaves().size(), 0);

  Name info("/test/name");
  info.appendNumber(0);

  BOOST_CHECK_NO_THROW(state.update(info, 12));

  BOOST_CHECK_NO_THROW(state.reset());
  BOOST_CHECK_EQUAL(state.getLeaves().size(), 0);

  std::tuple<bool, bool, SeqNo> result;
  result = state.update(info, 12);
  BOOST_CHECK_EQUAL(std::get<0>(result), true);
  BOOST_CHECK_EQUAL(std::get<1>(result), false);
  BOOST_CHECK_EQUAL(std::get<2>(result), 0);

  BOOST_CHECK_NO_THROW(state.update(info, 12));
  result = state.update(info, 12);
  BOOST_CHECK_EQUAL(std::get<0>(result), false);
  BOOST_CHECK_EQUAL(std::get<1>(result), false);
  BOOST_CHECK_EQUAL(std::get<2>(result), 0);

  BOOST_CHECK_NO_THROW(state.update(info, 11));
  result = state.update(info, 11);
  BOOST_CHECK_EQUAL(std::get<0>(result), false);
  BOOST_CHECK_EQUAL(std::get<1>(result), false);
  BOOST_CHECK_EQUAL(std::get<2>(result), 0);

  BOOST_CHECK_EQUAL(state.getLeaves().size(), 1);
  BOOST_CHECK_EQUAL((*state.getLeaves().begin())->getSeq(), 12);

  BOOST_CHECK_NO_THROW(state.update(info, 13));
  BOOST_CHECK_EQUAL(state.getLeaves().size(), 1);
  BOOST_CHECK_EQUAL((*state.getLeaves().begin())->getSeq(), 13);

  result = state.update(info, 14);
  BOOST_CHECK_EQUAL(std::get<0>(result), false);
  BOOST_CHECK_EQUAL(std::get<1>(result), true);
  BOOST_CHECK_EQUAL(std::get<2>(result), 13);

  BOOST_CHECK_EQUAL(state.getLeaves().size(), 1);
  BOOST_CHECK_EQUAL((*state.getLeaves().begin())->getSeq(), 14);

  Name info2("/test/name");
  info2.appendNumber(1);
  BOOST_CHECK_NO_THROW(state.update(info2, 3));
  BOOST_CHECK_EQUAL(state.getLeaves().size(), 2);
}

BOOST_AUTO_TEST_CASE(StateDigest)
{
  State state;
  BOOST_CHECK_EQUAL(state.getLeaves().size(), 0);

  Name info1("/test/name");
  info1.appendNumber(0);

  Name info2("/test/name");
  info2.appendNumber(1);

  Name info3("/test/mane");
  info3.appendNumber(0);

  state.update(info1, 10);
  ndn::ConstBufferPtr digest1 = state.getRootDigest();

  state.update(info2, 12);
  ndn::ConstBufferPtr digest2 = state.getRootDigest();

  state.update(info3, 8);
  ndn::ConstBufferPtr digest3 = state.getRootDigest();

  BOOST_CHECK(*digest1 != *digest2);
  BOOST_CHECK(*digest2 != *digest3);
  BOOST_CHECK(*digest1 != *digest3);

  state.reset();

  state.update(info1, 10);
  ndn::ConstBufferPtr digest4 = state.getRootDigest();

  state.update(info3, 8);
  ndn::ConstBufferPtr digest5 = state.getRootDigest();

  state.update(info2, 12);
  ndn::ConstBufferPtr digest6 = state.getRootDigest();

  BOOST_CHECK(*digest4 == *digest1);
  BOOST_CHECK(*digest5 != *digest2);
  BOOST_CHECK(*digest6 == *digest3);
}

BOOST_AUTO_TEST_CASE(DecodeEncode)
{
  const uint8_t wire[] = {
    0x80, 0x2c, // SyncReply
      0x81, 0x14, // StateLeaf
        0x07, 0x0f, // Name: /test/name/[0]
          0x08, 0x04,
            0x74, 0x65, 0x73, 0x74,
          0x08, 0x04,
            0x6e, 0x61, 0x6d, 0x65,
          0x08, 0x01,
            0x00,
        0x82, 0x1, // SeqNo: 14
          0x0e,
      0x81, 0x14, // StateLeaf
        0x07, 0x0f, // Name: /test/name/[1]
          0x08, 0x04,
            0x74, 0x65, 0x73, 0x74,
          0x08, 0x04,
            0x6e, 0x61, 0x6d, 0x65,
          0x08, 0x01,
            0x01,
        0x82, 0x1, // SeqNo: 4
          0x04
  };

  Block block(wire, sizeof(wire));
  State state;
  BOOST_REQUIRE_NO_THROW(state.wireDecode(block));

  BOOST_CHECK_EQUAL(state.getLeaves().size(), 2);
  LeafContainer::index<ordered>::type::iterator it = state.getLeaves().get<ordered>().begin();
  BOOST_CHECK_EQUAL((*it)->getSeq(), 14);
  it++;
  BOOST_CHECK_EQUAL((*it)->getSeq(), 4);


  State state2;

  Name info1("/test/name");
  info1.appendNumber(0);
  state2.update(info1, 14);

  Name info2("/test/name");
  info2.appendNumber(1);
  state2.update(info2, 4);

  BOOST_REQUIRE_NO_THROW(state2.wireEncode());
  Block block2 = state2.wireEncode();

  BOOST_CHECK_EQUAL_COLLECTIONS(block.wire(),
                                block.wire() + block.size(),
                                block2.wire(),
                                block2.wire() + block2.size());

  BOOST_CHECK(*state.getRootDigest() == *state2.getRootDigest());
}

BOOST_AUTO_TEST_CASE(Combine)
{
  State state1;
  State state2;

  Name info1("/test/name");
  info1.appendNumber(0);

  Name info2("/test/name");
  info2.appendNumber(1);

  Name info3("/test/name");
  info3.appendNumber(2);

  state1.update(info1, 4);
  state1.update(info2, 14);

  state2.update(info2, 15);
  state2.update(info3, 25);

  BOOST_CHECK_EQUAL(state1.getLeaves().size(), 2);
  BOOST_CHECK_EQUAL(state2.getLeaves().size(), 2);

  BOOST_REQUIRE_NO_THROW(state2 += state1);

  BOOST_CHECK_EQUAL(state1.getLeaves().size(), 2);
  BOOST_CHECK_EQUAL(state2.getLeaves().size(), 3);

  LeafContainer::index<ordered>::type::iterator it = state2.getLeaves().get<ordered>().begin();
  BOOST_CHECK_EQUAL((*it)->getSeq(), 4);
  it++;
  BOOST_CHECK_EQUAL((*it)->getSeq(), 15);
  it++;
  BOOST_CHECK_EQUAL((*it)->getSeq(), 25);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test
} // namespace chronosync
