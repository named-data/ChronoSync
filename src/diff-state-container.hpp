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

#ifndef CHRONOSYNC_DIFF_STATE_CONTAINER_HPP
#define CHRONOSYNC_DIFF_STATE_CONTAINER_HPP

#include "detail/mi-tag.hpp"
#include "diff-state.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/tag.hpp>

namespace chronosync {

namespace mi = boost::multi_index;

struct DigestPtrHash
{
  std::size_t
  operator()(ConstBufferPtr digest) const
  {
    BOOST_ASSERT(digest->size() > sizeof(std::size_t));

    return *reinterpret_cast<const std::size_t*>(digest->data());
  }
};

struct DigestPtrEqual
{
  bool
  operator()(ConstBufferPtr digest1, ConstBufferPtr digest2) const
  {
    return *digest1 == *digest2;
  }
};

/**
 * @brief Container for differential states
 */
struct DiffStateContainer : public mi::multi_index_container<
  DiffStatePtr,
  mi::indexed_by<
    // For fast access to elements using DiffState hashes
    mi::hashed_unique<
      mi::tag<hashed>,
      mi::const_mem_fun<DiffState, ConstBufferPtr, &DiffState::getRootDigest>,
      DigestPtrHash,
      DigestPtrEqual
      >,
    // sequenced index to access older/newer element (like in list)
    mi::sequenced<mi::tag<sequenced>>
    >
  >
{
};

} // namespace chronosync

#endif // CHRONOSYNC_DIFF_STATE_CONTAINER_HPP
