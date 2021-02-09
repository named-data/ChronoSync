/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2021 University of California, Los Angeles
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
 *
 * @author Zhenkai Zhu <http://irl.cs.ucla.edu/~zhenkai/>
 * @author Chaoyi Bian <bcy@pku.edu.cn>
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "state.hpp"
#include "detail/tlv.hpp"

#include <boost/range/adaptor/reversed.hpp>
#include <ndn-cxx/util/exception.hpp>

namespace chronosync {

State::~State() = default;

std::tuple<bool, bool, SeqNo>
State::update(const Name& info, const SeqNo& seq)
{
  m_wire.reset();

  auto leaf = m_leaves.find(info);
  if (leaf == m_leaves.end()) {
    m_leaves.insert(make_shared<Leaf>(info, seq));
    return std::make_tuple(true, false, 0);
  }
  else {
    if ((*leaf)->getSeq() == seq || seq < (*leaf)->getSeq()) {
      return std::make_tuple(false, false, 0);
    }

    SeqNo old = (*leaf)->getSeq();
    m_leaves.modify(leaf, [=] (LeafPtr& leaf) { leaf->setSeq(seq); } );
    return std::make_tuple(false, true, old);
  }
}

ConstBufferPtr
State::getRootDigest() const
{
  m_digest.reset();

  for (const auto& leaf : m_leaves.get<ordered>()) {
    BOOST_ASSERT(leaf != nullptr);
    m_digest.update(leaf->getDigest()->data(), leaf->getDigest()->size());
  }

  return m_digest.computeDigest();
}

void
State::reset()
{
  m_leaves.clear();
}

State&
State::operator+=(const State& state)
{
  for (const auto& leaf : state.getLeaves()) {
    BOOST_ASSERT(leaf != nullptr);
    update(leaf->getSessionName(), leaf->getSeq());
  }
  return *this;
}

template<ndn::encoding::Tag T>
size_t
State::wireEncode(ndn::encoding::EncodingImpl<T>& block) const
{
  size_t totalLength = 0;

  for (const auto& leaf : m_leaves.get<ordered>() | boost::adaptors::reversed) {
    size_t entryLength = 0;
    entryLength += prependNonNegativeIntegerBlock(block, tlv::SeqNo, leaf->getSeq());
    entryLength += leaf->getSessionName().wireEncode(block);
    entryLength += block.prependVarNumber(entryLength);
    entryLength += block.prependVarNumber(tlv::StateLeaf);
    totalLength += entryLength;
  }

  totalLength += block.prependVarNumber(totalLength);
  totalLength += block.prependVarNumber(tlv::SyncReply);

  return totalLength;
}

NDN_CXX_DEFINE_WIRE_ENCODE_INSTANTIATIONS(State);

const Block&
State::wireEncode() const
{
  if (m_wire.hasWire())
    return m_wire;

  ndn::EncodingEstimator estimator;
  size_t estimatedSize = wireEncode(estimator);

  ndn::EncodingBuffer buffer(estimatedSize, 0);
  wireEncode(buffer);

  m_wire = buffer.block();
  return m_wire;
}

void
State::wireDecode(const Block& wire)
{
  if (!wire.hasWire())
    NDN_THROW(Error("The supplied block does not contain wire format"));

  if (wire.type() != tlv::SyncReply)
    NDN_THROW(Error("Unexpected TLV type when decoding SyncReply: " + ndn::to_string(wire.type())));

  wire.parse();
  m_wire = wire;

  for (auto it = wire.elements_begin(); it != wire.elements_end(); it++) {
    if (it->type() == tlv::StateLeaf) {
      it->parse();

      auto val = it->elements_begin();
      Name info(*val);
      val++;

      if (val != it->elements_end())
        update(info, readNonNegativeInteger(*val));
      else
        NDN_THROW(Error("No SeqNo when decoding SyncReply"));
    }
  }
}

} // namespace chronosync
