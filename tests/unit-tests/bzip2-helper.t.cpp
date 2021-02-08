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
 */

#include "detail/bzip2-helper.hpp"

#include "tests/boost-test.hpp"

namespace chronosync {
namespace test {

BOOST_AUTO_TEST_SUITE(TestBzip2Helper)

BOOST_AUTO_TEST_CASE(Basic)
{
  std::string message = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
    "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
    "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "
    "nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
    "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. "
    "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
    "mollit anim id est laborum.";

  auto compressed = bzip2::compress(message.data(), message.size());
  BOOST_CHECK_LT(compressed->size(), message.size());

  auto decompressed = bzip2::decompress(reinterpret_cast<const char*>(compressed->data()), compressed->size());
  BOOST_CHECK_EQUAL(message.size(), decompressed->size());
  BOOST_CHECK_EQUAL(message, std::string(reinterpret_cast<const char*>(decompressed->data()), decompressed->size()));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace test
} // namespace chronosync
