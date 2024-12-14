/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2024 University of California, Los Angeles
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

#include "identity-management-fixture.hpp"

#include <ndn-cxx/util/io.hpp>

#include <filesystem>
#include <system_error>

namespace ndn::tests {

IdentityManagementFixture::IdentityManagementFixture()
  : m_keyChain("pib-memory:", "tpm-memory:")
{
  m_keyChain.createIdentity("/DEFAULT");
}

IdentityManagementFixture::~IdentityManagementFixture()
{
  std::error_code ec;
  for (const auto& certFile : m_certFiles) {
    std::filesystem::remove(certFile, ec); // ignore error
  }
}

bool
IdentityManagementFixture::addIdentity(const Name& identity, const ndn::KeyParams& params)
{
  try {
    m_keyChain.createIdentity(identity, params);
    return true;
  }
  catch (const std::runtime_error&) {
    return false;
  }
}

bool
IdentityManagementFixture::saveIdentityCertificate(const Name& identity, const std::string& filename, bool wantAdd)
{
  ndn::security::Certificate cert;
  try {
    cert = m_keyChain.getPib().getIdentity(identity).getDefaultKey().getDefaultCertificate();
  }
  catch (const ndn::security::Pib::Error&) {
    if (wantAdd && this->addIdentity(identity)) {
      return this->saveIdentityCertificate(identity, filename, false);
    }
    return false;
  }

  m_certFiles.push_back(filename);
  try {
    ndn::io::save(cert, filename);
    return true;
  }
  catch (const ndn::io::Error&) {
    return false;
  }
}

} // namespace ndn::tests
