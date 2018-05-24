/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2018 University of California, Los Angeles
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
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOSYNC_COMMON_HPP
#define CHRONOSYNC_COMMON_HPP

#include "config.hpp"

#ifdef CHRONOSYNC_HAVE_TESTS
#define CHRONOSYNC_VIRTUAL_WITH_TESTS virtual
#define CHRONOSYNC_PUBLIC_WITH_TESTS_ELSE_PROTECTED public
#define CHRONOSYNC_PUBLIC_WITH_TESTS_ELSE_PRIVATE public
#define CHRONOSYNC_PROTECTED_WITH_TESTS_ELSE_PRIVATE protected
#else
#define CHRONOSYNC_VIRTUAL_WITH_TESTS
#define CHRONOSYNC_PUBLIC_WITH_TESTS_ELSE_PROTECTED protected
#define CHRONOSYNC_PUBLIC_WITH_TESTS_ELSE_PRIVATE private
#define CHRONOSYNC_PROTECTED_WITH_TESTS_ELSE_PRIVATE private
#endif

#include <cstddef>
#include <list>
#include <queue>
#include <set>
#include <vector>
#include <tuple>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/security/v2/validator.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

namespace chronosync {

using std::size_t;

using boost::noncopyable;

using std::bind;
using std::cref;
using std::function;
using std::make_shared;
using std::make_tuple;
using std::ref;
using std::shared_ptr;

using ndn::Block;
using ndn::ConstBufferPtr;
using ndn::Data;
using ndn::Exclude;
using ndn::Interest;
using ndn::Name;
using ndn::security::v2::ValidationError;
using ndn::security::v2::Validator;

namespace tlv {
using namespace ndn::tlv;
} // namespace tlv

namespace name = ndn::name;
namespace time = ndn::time;
namespace security = ndn::security;
namespace encoding = ndn::encoding;

} // namespace chronosync

#endif // CHRONOSYNC_COMMON_HPP
