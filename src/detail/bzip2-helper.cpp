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

#include "bzip2-helper.hpp"

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/stream.hpp>

#include <ndn-cxx/encoding/buffer-stream.hpp>

namespace chronosync {
namespace bzip2 {

namespace bio = boost::iostreams;

std::shared_ptr<ndn::Buffer>
compress(const char* buffer, size_t bufferSize)
{
  ndn::OBufferStream os;
  bio::filtering_stream<bio::output> out;
  out.push(bio::bzip2_compressor());
  out.push(os);
  bio::stream<bio::array_source> in(buffer, bufferSize);
  bio::copy(in, out);
  return os.buf();
}

std::shared_ptr<ndn::Buffer>
decompress(const char* buffer, size_t bufferSize)
{
  ndn::OBufferStream os;
  bio::filtering_stream<bio::output> out;
  out.push(bio::bzip2_decompressor());
  out.push(os);
  bio::stream<bio::array_source> in(buffer, bufferSize);
  bio::copy(in, out);
  return os.buf();
}

} // namespace bzip2
} // namespace chronosync
