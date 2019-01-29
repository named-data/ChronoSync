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

#include "diff-state.hpp"
#include "diff-state-container.hpp"

#include "tests/boost-test.hpp"

namespace chronosync {
namespace test {

BOOST_AUTO_TEST_SUITE(DiffStateTests)

BOOST_AUTO_TEST_CASE(Diff)
{
  State state;

  Name info1("/test/name");
  info1.appendNumber(0);

  Name info2("/test/name");
  info2.appendNumber(1);

  Name info3("/test/name");
  info3.appendNumber(2);

  DiffStatePtr root = make_shared<DiffState>();
  DiffStatePtr action1 = make_shared<DiffState>();
  root->setNext(action1);

  state.update(info1, 1);
  action1->update(info1, 1);
  action1->setRootDigest(state.getRootDigest());

  DiffStatePtr action2 = make_shared<DiffState>();
  action1->setNext(action2);

  state.update(info2, 1);
  state.update(info3, 2);
  action2->update(info2, 1);
  action2->update(info3, 2);
  action2->setRootDigest(state.getRootDigest());

  LeafContainer::index<ordered>::type::iterator it;
  ConstStatePtr diff0 = root->diff();
  BOOST_CHECK_EQUAL(diff0->getLeaves().size(), 3);
  it = diff0->getLeaves().get<ordered>().begin();
  BOOST_CHECK_EQUAL((*it)->getSessionName(), info1);
  BOOST_CHECK_EQUAL((*it)->getSeq(), 1);
  it++;
  BOOST_CHECK_EQUAL((*it)->getSessionName(), info2);
  BOOST_CHECK_EQUAL((*it)->getSeq(), 1);
  it++;
  BOOST_CHECK_EQUAL((*it)->getSessionName(), info3);
  BOOST_CHECK_EQUAL((*it)->getSeq(), 2);

  ConstStatePtr diff1 = action1->diff();
  BOOST_CHECK_EQUAL(diff1->getLeaves().size(), 2);
  it = diff1->getLeaves().get<ordered>().begin();
  BOOST_CHECK_EQUAL((*it)->getSessionName(), info2);
  BOOST_CHECK_EQUAL((*it)->getSeq(), 1);
  it++;
  BOOST_CHECK_EQUAL((*it)->getSessionName(), info3);
  BOOST_CHECK_EQUAL((*it)->getSeq(), 2);
}

BOOST_AUTO_TEST_CASE(Container)
{
  DiffStateContainer container;

  State state;

  Name info1("/test/name");
  info1.appendNumber(0);

  Name info2("/test/name");
  info2.appendNumber(1);

  Name info3("/test/name");
  info3.appendNumber(2);

  DiffStatePtr root = make_shared<DiffState>();
  DiffStatePtr action1 = make_shared<DiffState>();
  root->setNext(action1);

  state.update(info1, 1);
  action1->update(info1, 1);
  ndn::ConstBufferPtr digest1 = state.getRootDigest();
  action1->setRootDigest(digest1);

  DiffStatePtr action2 = make_shared<DiffState>();
  action1->setNext(action2);

  state.update(info2, 1);
  state.update(info3, 2);
  action2->update(info2, 1);
  action2->update(info3, 2);
  ndn::ConstBufferPtr digest2 = state.getRootDigest();
  action2->setRootDigest(digest2);

  DiffStatePtr action3 = make_shared<DiffState>();
  state.update(info1, 3);
  state.update(info3, 4);
  action3->update(info1, 3);
  action3->update(info3, 4);
  ndn::ConstBufferPtr digest3 = state.getRootDigest();
  action3->setRootDigest(digest3);

  container.insert(action3);
  container.insert(action2);
  container.insert(action1);
  BOOST_CHECK_EQUAL(container.size(), 3);

  DiffStateContainer::index<sequenced>::type::iterator it = container.get<sequenced>().begin();
  BOOST_CHECK(*(*it)->getRootDigest() == *digest3);
  it++;
  BOOST_CHECK(*(*it)->getRootDigest() == *digest2);
  it++;
  BOOST_CHECK(*(*it)->getRootDigest() == *digest1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test
} // namespace chronosync
