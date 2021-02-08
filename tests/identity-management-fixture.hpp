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

#ifndef NDN_TESTS_IDENTITY_MANAGEMENT_FIXTURE_HPP
#define NDN_TESTS_IDENTITY_MANAGEMENT_FIXTURE_HPP

#include "detail/common.hpp"

#include "unit-test-time-fixture.hpp"

namespace ndn {
namespace tests {

/** \brief a fixture that cleans up KeyChain identities and certificate files upon destruction
 */
class IdentityManagementFixture
{
public:
  IdentityManagementFixture();

  /** \brief deletes saved certificate files
   */
  ~IdentityManagementFixture();

  /** \brief add identity
   *  \return whether successful
   */
  bool
  addIdentity(const Name& identity,
              const ndn::KeyParams& params = ndn::KeyChain::getDefaultKeyParams());

  /** \brief save identity certificate to a file
   *  \param identity identity name
   *  \param filename file name, should be writable
   *  \param wantAdd if true, add new identity when necessary
   *  \return whether successful
   */
  bool
  saveIdentityCertificate(const Name& identity, const std::string& filename, bool wantAdd = false);

protected:
  ndn::KeyChain m_keyChain;

private:
  std::vector<std::string> m_certFiles;
};

/** \brief convenience base class for inheriting from both UnitTestTimeFixture
 *         and IdentityManagementFixture
 */
class IdentityManagementTimeFixture : public UnitTestTimeFixture
                                    , public IdentityManagementFixture
{
};

} // namespace tests
} // namespace ndn

#endif // NDN_TESTS_IDENTITY_MANAGEMENT_FIXTURE_HPP
