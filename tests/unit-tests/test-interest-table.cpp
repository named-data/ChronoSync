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

#include "interest-table.hpp"

#include "tests/boost-test.hpp"
#include "tests/unit-test-time-fixture.hpp"

#include <unistd.h>

namespace chronosync {
namespace test {

class InterestTableFixture : public ndn::tests::UnitTestTimeFixture
{
public:
  InterestTableFixture()
  {
    const uint8_t origin[] = {0x01, 0x02, 0x03, 0x04};
    Name prefix("/test/prefix");

    Name interestName1;
    digest1 = ndn::util::Sha256::computeDigest({origin, 1});
    interestName1.append(prefix).append(name::Component(digest1));

    interest1 = Interest(interestName1);
    interest1.setInterestLifetime(time::milliseconds(100));

    Name interestName2;
    digest2 = ndn::util::Sha256::computeDigest({origin, 2});
    interestName2.append(prefix).append(name::Component(digest2));
    interest2 = Interest(interestName2);
    interest2.setInterestLifetime(time::milliseconds(100));

    Name interestName3;
    digest3 = ndn::util::Sha256::computeDigest({origin, 3});
    interestName3.append(prefix).append(name::Component(digest3));
    interest3 = Interest(interestName3);
    interest3.setInterestLifetime(time::milliseconds(100));
  }

  void
  insert(InterestTable& table, const Interest& interest, ndn::ConstBufferPtr digest)
  {
    table.insert(interest, digest);
  }

  Interest interest1;
  ndn::ConstBufferPtr digest1;

  Interest interest2;
  ndn::ConstBufferPtr digest2;

  Interest interest3;
  ndn::ConstBufferPtr digest3;
};

BOOST_FIXTURE_TEST_SUITE(InterestTableTest, InterestTableFixture)

BOOST_AUTO_TEST_CASE(Container)
{
  InterestContainer container;

  container.insert(make_shared<UnsatisfiedInterest>(interest1, digest1));
  container.insert(make_shared<UnsatisfiedInterest>(interest2, digest2));
  container.insert(make_shared<UnsatisfiedInterest>(interest3, digest3));

  BOOST_CHECK_EQUAL(container.size(), 3);
  BOOST_CHECK(container.find(digest3) != container.end());
  BOOST_CHECK(container.find(digest2) != container.end());
  BOOST_CHECK(container.find(digest1) != container.end());
}

BOOST_AUTO_TEST_CASE(Basic)
{
  InterestTable table(io);

  table.insert(interest1, digest1);
  table.insert(interest2, digest2);
  table.insert(interest3, digest3);

  BOOST_CHECK_EQUAL(table.size(), 3);
  InterestTable::const_iterator it = table.begin();
  BOOST_CHECK(it != table.end());
  it++;
  BOOST_CHECK(it != table.end());
  it++;
  BOOST_CHECK(it != table.end());
  it++;
  BOOST_CHECK(it == table.end());

  BOOST_CHECK_EQUAL(table.size(), 3);
  table.erase(digest1);
  BOOST_CHECK_EQUAL(table.size(), 2);
  table.erase(digest2);
  BOOST_CHECK_EQUAL(table.size(), 1);
  ConstUnsatisfiedInterestPtr pendingInterest = *table.begin();
  table.clear();
  BOOST_CHECK_EQUAL(table.size(), 0);
  BOOST_CHECK(*pendingInterest->digest == *digest3);
}

BOOST_AUTO_TEST_CASE(Expire)
{
  InterestTable table(io);

  insert(table, interest1, digest1);

  advanceClocks(ndn::time::milliseconds(10), 10);

  insert(table, interest2, digest2);
  insert(table, interest3, digest3);

  advanceClocks(ndn::time::milliseconds(10), 5);

  insert(table, interest2, digest2);

  advanceClocks(ndn::time::milliseconds(10), 2);
  BOOST_CHECK_EQUAL(table.size(), 2);

  advanceClocks(ndn::time::milliseconds(10), 5);
  BOOST_CHECK_EQUAL(table.size(), 1);

  advanceClocks(ndn::time::milliseconds(10), 15);
  BOOST_CHECK_EQUAL(table.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test
} // namespace chronosync
