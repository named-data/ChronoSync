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

#ifndef CHRONOSYNC_TESTS_UNIT_TEST_TIME_FIXTURE_HPP
#define CHRONOSYNC_TESTS_UNIT_TEST_TIME_FIXTURE_HPP

#include <ndn-cxx/util/time-unit-test-clock.hpp>

#include <boost/asio/io_context.hpp>

namespace ndn::tests {

class UnitTestTimeFixture
{
public:
  UnitTestTimeFixture()
    : steadyClock(make_shared<time::UnitTestSteadyClock>())
    , systemClock(make_shared<time::UnitTestSystemClock>())
  {
    time::setCustomClocks(steadyClock, systemClock);
  }

  ~UnitTestTimeFixture()
  {
    time::setCustomClocks(nullptr, nullptr);
  }

  void
  advanceClocks(time::nanoseconds tick, size_t nTicks = 1)
  {
    for (size_t i = 0; i < nTicks; ++i) {
      steadyClock->advance(tick);
      systemClock->advance(tick);

      if (io.stopped()) {
        io.restart();
      }
      io.poll();
    }
  }

public:
  shared_ptr<time::UnitTestSteadyClock> steadyClock;
  shared_ptr<time::UnitTestSystemClock> systemClock;
  boost::asio::io_context io;
};

} // namespace ndn::tests

#endif // CHRONOSYNC_TESTS_UNIT_TEST_TIME_FIXTURE_HPP
