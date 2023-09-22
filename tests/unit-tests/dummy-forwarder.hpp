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

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/lp/nack.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

#ifndef NDN_CHRONOSYNC_UNIT_TESTS_DUMMY_FORWARDER_HPP
#define NDN_CHRONOSYNC_UNIT_TESTS_DUMMY_FORWARDER_HPP

namespace ndn {
namespace chronosync {

/**
 * @brief Very basic implementation of the dummy forwarder
 *
 * Interests expressed by any added face, will be forwarded to all other faces.
 * Similarly, any pushed data, will be pushed to all other faces.
 */
class DummyForwarder
{
public:
  DummyForwarder(boost::asio::io_context& io, KeyChain& keyChain);

  Face&
  addFace();

  Face&
  getFace(size_t nFace)
  {
    return *m_faces.at(nFace);
  }

  void
  removeFaces();

private:
  boost::asio::io_context& m_io;
  KeyChain& m_keyChain;
  std::vector<std::shared_ptr<DummyClientFace>> m_faces;
};

} // namespace chronosync
} // namespace ndn

#endif // NDN_CHRONOSYNC_UNIT_TESTS_DUMMY_FORWARDER_HPP
