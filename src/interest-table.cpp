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
 *
 * @author Zhenkai Zhu <http://irl.cs.ucla.edu/~zhenkai/>
 * @author Chaoyi Bian <bcy@pku.edu.cn>
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "interest-table.hpp"

namespace chronosync {

InterestTable::InterestTable(boost::asio::io_service& io)
  : m_scheduler(io)
{
}

InterestTable::~InterestTable()
{
  clear();
}

void
InterestTable::insert(const Interest& interest, ConstBufferPtr digest, bool isKnown/*=false*/)
{
  erase(digest);

  auto request = make_shared<UnsatisfiedInterest>(interest, digest, isKnown);

  time::milliseconds entryLifetime = interest.getInterestLifetime();
  if (entryLifetime < time::milliseconds::zero())
    entryLifetime = ndn::DEFAULT_INTEREST_LIFETIME;

  request->expirationEvent = m_scheduler.schedule(entryLifetime, [=] { erase(digest); });

  m_table.insert(request);
}

void
InterestTable::erase(ConstBufferPtr digest)
{
  auto it = m_table.get<hashed>().find(digest);
  while (it != m_table.get<hashed>().end()) {
    (*it)->expirationEvent.cancel();
    m_table.erase(it);

    it = m_table.get<hashed>().find(digest);
  }
}

bool
InterestTable::has(ConstBufferPtr digest)
{
  if (m_table.get<hashed>().find(digest) != m_table.get<hashed>().end())
    return true;
  else
    return false;
}

size_t
InterestTable::size() const
{
  return m_table.size();
}

void
InterestTable::clear()
{
  for (const auto& item : m_table) {
    item->expirationEvent.cancel();
  }
  m_table.clear();
}

} // namespace chronosync
