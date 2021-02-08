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

#ifndef CHRONOSYNC_LEAF_HPP
#define CHRONOSYNC_LEAF_HPP

#include "detail/common.hpp"

#include <ndn-cxx/util/sha256.hpp>

namespace chronosync {

using SeqNo = uint64_t;

/**
 * @brief Sync tree leaf
 *
 * The leaf node should be copyable when used to construct diff between two states.
 */
class Leaf
{
public:
  Leaf(const Name& sessionName, const SeqNo& seq);

  Leaf(const Name& userPrefix, uint64_t session, const SeqNo& seq);

  virtual
  ~Leaf();

  const Name&
  getSessionName() const
  {
    return m_sessionName;
  }

  const SeqNo&
  getSeq() const
  {
    return m_seq;
  }

  ConstBufferPtr
  getDigest() const;

  /**
   * @brief Update sequence number of the leaf
   * @param seq Sequence number
   *
   * If seq is no greater than getSeq(), this operation has no effect.
   */
  virtual void
  setSeq(const SeqNo& seq);

private:
  void
  updateDigest();

private:
  Name     m_sessionName;
  SeqNo    m_seq;

  mutable ndn::util::Sha256 m_digest;
};

using LeafPtr = shared_ptr<Leaf>;
using ConstLeafPtr = shared_ptr<const Leaf>;

std::ostream&
operator<<(std::ostream& os, const Leaf& leaf);

} // namespace chronosync

#endif // CHRONOSYNC_LEAF_HPP
