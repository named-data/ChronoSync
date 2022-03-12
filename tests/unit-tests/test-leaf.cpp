/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2022 University of California, Los Angeles
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

#include "leaf.hpp"
#include "leaf-container.hpp"

#include "tests/boost-test.hpp"

#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/util/string-helper.hpp>

namespace chronosync {
namespace test {

BOOST_AUTO_TEST_SUITE(LeafTests)

BOOST_AUTO_TEST_CASE(LeafBasic)
{
  Name userPrefix("/test/name");
  BOOST_CHECK_NO_THROW(Leaf leaf(userPrefix, 1, 10));

  Leaf leaf(userPrefix, 1, 10);
  Name sessionName = userPrefix;
  sessionName.appendNumber(1);
  BOOST_CHECK_EQUAL(leaf.getSessionName(), sessionName);
  BOOST_CHECK_EQUAL(leaf.getSeq(), 10);

  leaf.setSeq(9);
  BOOST_CHECK_EQUAL(leaf.getSeq(), 10);
  leaf.setSeq(11);
  BOOST_CHECK_EQUAL(leaf.getSeq(), 11);
}

BOOST_AUTO_TEST_CASE(LeafDigest)
{
  std::string result = "05fe7f728d3341e9eff82526277b02171044124d0a52e8c4610982261c20de2b";

  Name userPrefix("/test/name");
  Leaf leaf(userPrefix, 1, 10);

  BOOST_CHECK_EQUAL(result, ndn::toHex(*leaf.getDigest(), false));
}

BOOST_AUTO_TEST_CASE(Container)
{
  LeafPtr leaf1 = make_shared<Leaf>(Name("/test/name"), 1, 10);
  LeafPtr leaf2 = make_shared<Leaf>(Name("/test/name"), 2, 10);

  LeafContainer container;

  container.insert(leaf1);
  container.insert(leaf2);

  Name idx1("/test/name");
  idx1.appendNumber(1);

  Name idx2("/test/name");
  idx2.appendNumber(2);

  Name idx3("/test/name");
  idx3.appendNumber(3);

  Name idx4("/test/mane");
  idx4.appendNumber(4);

  LeafContainer::index<hashed>::type& hashedIndex = container.get<hashed>();

  BOOST_CHECK_EQUAL(container.get<ordered>().size(), 2);
  BOOST_CHECK_EQUAL(container.get<hashed>().size(), 2);
  BOOST_CHECK(container.find(idx1) != container.end());
  BOOST_CHECK(container.find(idx2) != container.end());
  BOOST_CHECK(container.find(idx3) == container.end());

  BOOST_CHECK(hashedIndex.find(idx1) != hashedIndex.end());
  BOOST_CHECK(hashedIndex.find(idx2) != hashedIndex.end());
  BOOST_CHECK(hashedIndex.find(idx3) == hashedIndex.end());

  LeafPtr leaf3 = make_shared<Leaf>(Name("/test/mane"), 3, 10);
  container.insert(leaf3);

  Name idx0("/test/mane");
  idx0.appendNumber(3);

  LeafContainer::index<ordered>::type::iterator it = container.get<ordered>().begin();
  BOOST_CHECK_EQUAL((*it)->getSessionName(), idx0);
  it++;
  BOOST_REQUIRE(it != container.get<ordered>().end());
  BOOST_CHECK_EQUAL((*it)->getSessionName(), idx1);
  it++;
  BOOST_REQUIRE(it != container.get<ordered>().end());
  BOOST_CHECK_EQUAL((*it)->getSessionName(), idx2);


  BOOST_CHECK(hashedIndex.find(idx0) != hashedIndex.end());
  BOOST_CHECK(hashedIndex.find(idx1) != hashedIndex.end());
  BOOST_CHECK(hashedIndex.find(idx2) != hashedIndex.end());
  BOOST_CHECK(hashedIndex.find(idx4) == hashedIndex.end());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test
} // namespace chronosync
