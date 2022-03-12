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
 *
 * @author Zhenkai Zhu <http://irl.cs.ucla.edu/~zhenkai/>
 * @author Chaoyi Bian <bcy@pku.edu.cn>
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOSYNC_LEAF_CONTAINER_HPP
#define CHRONOSYNC_LEAF_CONTAINER_HPP

#include "detail/mi-tag.hpp"
#include "leaf.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/tag.hpp>

#include <ndn-cxx/util/sha256.hpp>

namespace chronosync {

namespace mi = boost::multi_index;

struct SessionNameHash
{
  static_assert(ndn::util::Sha256::DIGEST_SIZE >= sizeof(std::size_t), "");

  std::size_t
  operator()(const Name& prefix) const
  {
    auto buffer = ndn::util::Sha256::computeDigest(prefix.wireEncode());
    return *reinterpret_cast<const std::size_t*>(buffer->data());
  }
};

struct SessionNameEqual
{
  bool
  operator()(const Name& prefix1, const Name& prefix2) const
  {
    return prefix1 == prefix2;
  }
};

struct SessionNameCompare : public std::less<Name>
{
  bool
  operator()(const Name& prefix1, const Name& prefix2) const
  {
    return prefix1 < prefix2;
  }
};

/**
 * @brief Container for chronosync leaves
 */
struct LeafContainer : public mi::multi_index_container<
  LeafPtr,
  mi::indexed_by<
    // For fast access to elements using SessionName
    mi::hashed_unique<
      mi::tag<hashed>,
      mi::const_mem_fun<Leaf, const Name&, &Leaf::getSessionName>,
      SessionNameHash,
      SessionNameEqual
      >,

    mi::ordered_unique<
      mi::tag<ordered>,
      mi::const_mem_fun<Leaf, const Name&, &Leaf::getSessionName>,
      SessionNameCompare
      >
    >
  >
{
};

} // namespace chronosync

#endif // CHRONOSYNC_LEAF_CONTAINER_HPP
