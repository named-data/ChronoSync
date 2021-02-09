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
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 * @author Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOSYNC_DETAIL_COMMON_HPP
#define CHRONOSYNC_DETAIL_COMMON_HPP

#include "config.hpp"

#ifdef CHRONOSYNC_WITH_TESTS
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
#include <functional>
#include <memory>
#include <set>
#include <vector>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/security/validator.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/assert.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/lexical_cast.hpp>

namespace chronosync {

using boost::noncopyable;

using std::size_t;

using std::bind;
using std::function;
using std::make_shared;
using std::ref;
using std::shared_ptr;

using ndn::Block;
using ndn::ConstBufferPtr;
using ndn::Data;
using ndn::Interest;
using ndn::Name;
using ndn::security::Validator;
using ndn::security::ValidationError;

namespace name = ndn::name;
namespace time = ndn::time;
namespace security = ndn::security;

} // namespace chronosync

#endif // CHRONOSYNC_DETAIL_COMMON_HPP
