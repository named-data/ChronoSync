/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2024 University of California, Los Angeles
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

#include "dummy-forwarder.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

namespace chronosync::tests {

using ndn::DummyClientFace;

DummyForwarder::DummyForwarder(boost::asio::io_context& io, ndn::KeyChain& keyChain)
  : m_io(io)
  , m_keyChain(keyChain)
{
}

ndn::Face&
DummyForwarder::addFace()
{
  auto face = std::make_shared<DummyClientFace>(m_io, m_keyChain, DummyClientFace::Options{true, true});
  DummyClientFace* self = &*face; // to prevent memory leak

  face->onSendInterest.connect([this, self] (const auto& interest) {
      ndn::Interest i(interest);
      for (auto& otherFace : m_faces) {
        if (self == &*otherFace) {
          continue;
        }
        boost::asio::post(m_io, [=] { otherFace->receive(i); });
      }
    });

  face->onSendData.connect([this, self] (const auto& data) {
      ndn::Data d(data);
      for (auto& otherFace : m_faces) {
        if (self == &*otherFace) {
          continue;
        }
        boost::asio::post(m_io, [=] { otherFace->receive(d); });
      }
    });

  face->onSendNack.connect([this, self] (const auto& nack) {
      ndn::lp::Nack n(nack);
      for (auto& otherFace : m_faces) {
        if (self == &*otherFace) {
          continue;
        }
        boost::asio::post(m_io, [=] { otherFace->receive(n); });
      }
    });

  m_faces.push_back(face);
  return *face;
}

void
DummyForwarder::removeFaces()
{
  m_faces.clear();
}

} // namespace chronosync::tests
