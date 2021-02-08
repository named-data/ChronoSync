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

#ifndef CHRONOSYNC_INTEREST_CONTAINER_HPP
#define CHRONOSYNC_INTEREST_CONTAINER_HPP

#include "diff-state-container.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/tag.hpp>

namespace chronosync {

namespace mi = boost::multi_index;

/// @brief Entry to record unsatisfied Sync Interest
class UnsatisfiedInterest : noncopyable
{
public:
  UnsatisfiedInterest(const Interest& interest,
                      ConstBufferPtr digest,
                      bool isUnknown = false);

public:
  const Interest interest;
  ConstBufferPtr digest;
  bool isUnknown;
  ndn::scheduler::EventId expirationEvent;
};

using UnsatisfiedInterestPtr = shared_ptr<UnsatisfiedInterest>;
using ConstUnsatisfiedInterestPtr = shared_ptr<const UnsatisfiedInterest>;

/**
 * @brief Container for unsatisfied Sync Interests
 */
struct InterestContainer : public mi::multi_index_container<
  UnsatisfiedInterestPtr,
  mi::indexed_by<
    mi::hashed_unique<
      mi::tag<hashed>,
      mi::member<UnsatisfiedInterest, ConstBufferPtr, &UnsatisfiedInterest::digest>,
      DigestPtrHash,
      DigestPtrEqual
      >
    >
  >
{
};

} // namespace chronosync

#endif // CHRONOSYNC_INTEREST_CONTAINER_HPP
