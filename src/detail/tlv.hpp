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
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOSYNC_DETAIL_TLV_HPP
#define CHRONOSYNC_DETAIL_TLV_HPP

namespace chronosync {
namespace tlv {

/**
 * @brief Type value of sync reply related TLVs
 * @sa docs/design.rst
 */
enum {
  SyncReply   = 128, // 0x80
  StateLeaf   = 129, // 0x81
  SeqNo       = 130, // 0x82
};

} // namespace tlv
} // namespace chronosync

#endif // CHRONOSYNC_DETAIL_TLV_HPP
