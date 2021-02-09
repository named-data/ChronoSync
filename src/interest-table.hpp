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

#ifndef CHRONOSYNC_INTEREST_TABLE_HPP
#define CHRONOSYNC_INTEREST_TABLE_HPP

#include "interest-container.hpp"

#include <boost/asio/io_service.hpp>

namespace chronosync {

/**
 * @brief A table to keep unsatisfied Sync Interest
 */
class InterestTable : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  using iterator = InterestContainer::iterator;
  using const_iterator = InterestContainer::const_iterator;

  explicit
  InterestTable(boost::asio::io_service& io);

  ~InterestTable();

  /**
   * @brief Insert an interest
   *
   * If the interest already exists in the table, the old interest will be replaced,
   * and expire timer will be reset.
   * Interests with the same name are counted as the same.
   * This method assumes that the sync prefix of all interests are the same
   * thus it only compares the digest part.
   *
   * @param interest Interest to insert.
   * @param digest   The value of the last digest component.
   * @param isKnown  false if the digest is an unknown digest.
   */
  void
  insert(const Interest& interest, ConstBufferPtr digest, bool isKnown = false);

  /// @brief check if an interest with the digest exists in the table
  bool
  has(ConstBufferPtr digest);

  /// @brief Delete interest by digest (e.g., when it was satisfied)
  void
  erase(ConstBufferPtr digest);

  const_iterator
  begin() const
  {
    return m_table.begin();
  }

  iterator
  begin()
  {
    return m_table.begin();
  }

  const_iterator
  end() const
  {
    return m_table.end();
  }

  iterator
  end()
  {
    return m_table.end();
  }

  size_t
  size() const;

  void
  clear();

private:
  ndn::Scheduler m_scheduler;
  InterestContainer m_table;
};

} // namespace chronosync

#endif // CHRONOSYNC_INTEREST_TABLE_HPP
