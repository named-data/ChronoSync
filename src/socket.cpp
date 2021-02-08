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

#include "socket.hpp"

#include <ndn-cxx/util/logger.hpp>

NDN_LOG_INIT(sync.Socket);

namespace chronosync {

const ndn::Name Socket::DEFAULT_NAME;
const ndn::Name Socket::DEFAULT_PREFIX;
const std::shared_ptr<Validator> Socket::DEFAULT_VALIDATOR;

Socket::Socket(const Name& syncPrefix,
               const Name& userPrefix,
               ndn::Face& face,
               const UpdateCallback& updateCallback,
               const Name& signingId,
               std::shared_ptr<Validator> validator,
               const time::milliseconds& syncInterestLifetime,
               const name::Component& session)
  : m_userPrefix(userPrefix)
  , m_face(face)
  , m_logic(face, syncPrefix, userPrefix, updateCallback, Logic::DEFAULT_NAME, Logic::DEFAULT_VALIDATOR,
            Logic::DEFAULT_RESET_TIMER, Logic::DEFAULT_CANCEL_RESET_TIMER, Logic::DEFAULT_RESET_INTEREST_LIFETIME,
            syncInterestLifetime, Logic::DEFAULT_SYNC_REPLY_FRESHNESS, Logic::DEFAULT_RECOVERY_INTEREST_LIFETIME,
            session)
  , m_signingId(signingId)
  , m_validator(validator)
{
  NDN_LOG_DEBUG(">> Socket::Socket");
  if (m_userPrefix != DEFAULT_NAME)
    m_registeredPrefixList[m_userPrefix] =
      m_face.setInterestFilter(m_userPrefix,
                               bind(&Socket::onInterest, this, _1, _2),
                               [] (const Name&, const std::string&) {});
  NDN_LOG_DEBUG("<< Socket::Socket");
}

Socket::~Socket()
{
  for (auto& itr : m_registeredPrefixList) {
    itr.second.unregister();
  }
}

void
Socket::addSyncNode(const Name& prefix, const Name& signingId, const name::Component& session)
{
  if (prefix == DEFAULT_NAME)
    return;

  NDN_LOG_DEBUG(">> addSyncNode");

  auto itr = m_registeredPrefixList.find(prefix);
  if (itr != m_registeredPrefixList.end())
    return;

  if (m_userPrefix == DEFAULT_NAME)
    m_userPrefix = prefix;
  m_logic.addUserNode(prefix, signingId, session);
  m_registeredPrefixList[prefix] =
    m_face.setInterestFilter(prefix,
                             bind(&Socket::onInterest, this, _1, _2),
                             [] (const Name&, const std::string&) {});

  NDN_LOG_DEBUG("<< addSyncNode");
}

void
Socket::removeSyncNode(const Name& prefix)
{
  if (prefix == DEFAULT_NAME)
    return;

  auto itr = m_registeredPrefixList.find(prefix);
  if (itr != m_registeredPrefixList.end()) {
    itr->second.unregister();
    m_registeredPrefixList.erase(itr);
  }

  m_ims.erase(prefix);
  m_logic.removeUserNode(prefix);
}

void
Socket::publishData(const uint8_t* buf, size_t len, const ndn::time::milliseconds& freshness,
                    const Name& prefix)
{
  publishData(ndn::encoding::makeBinaryBlock(ndn::tlv::Content, buf, len), freshness, prefix);
}

void
Socket::publishData(const uint8_t* buf, size_t len, const ndn::time::milliseconds& freshness,
                    const uint64_t& seqNo, const Name& prefix)
{
  publishData(ndn::encoding::makeBinaryBlock(ndn::tlv::Content, buf, len), freshness, seqNo, prefix);
}

void
Socket::publishData(const Block& content, const ndn::time::milliseconds& freshness,
                    const Name& prefix)
{
  shared_ptr<Data> data = make_shared<Data>();
  data->setContent(content);
  data->setFreshnessPeriod(freshness);

  SeqNo newSeq = m_logic.getSeqNo(prefix) + 1;
  Name dataName;
  dataName.append(m_logic.getSessionName(prefix)).appendNumber(newSeq);
  data->setName(dataName);

  if (m_signingId.empty())
    m_keyChain.sign(*data);
  else
    m_keyChain.sign(*data, security::signingByIdentity(m_signingId));

  m_ims.insert(*data);

  m_logic.updateSeqNo(newSeq, prefix);
}

void
Socket::publishData(const Block& content, const ndn::time::milliseconds& freshness,
                    const uint64_t& seqNo, const Name& prefix)
{
  shared_ptr<Data> data = make_shared<Data>();
  data->setContent(content);
  data->setFreshnessPeriod(freshness);

  SeqNo newSeq = seqNo;
  Name dataName;
  dataName.append(m_logic.getSessionName(prefix)).appendNumber(newSeq);
  data->setName(dataName);

  if (m_signingId.empty())
    m_keyChain.sign(*data);
  else
    m_keyChain.sign(*data, security::signingByIdentity(m_signingId));

  m_ims.insert(*data);

  m_logic.updateSeqNo(newSeq, prefix);
}

void
Socket::fetchData(const Name& sessionName, const SeqNo& seqNo,
                  const DataValidatedCallback& dataCallback,
                  int nRetries)
{
  Name interestName;
  interestName.append(sessionName).appendNumber(seqNo);

  Interest interest(interestName);
  interest.setMustBeFresh(true);
  interest.setCanBePrefix(false);

  DataValidationErrorCallback failureCallback =
    bind(&Socket::onDataValidationFailed, this, _1, _2);

  m_face.expressInterest(interest,
                         bind(&Socket::onData, this, _1, _2, dataCallback, failureCallback),
                         bind(&Socket::onDataTimeout, this, _1, nRetries,
                              dataCallback, failureCallback), // Nack
                         bind(&Socket::onDataTimeout, this, _1, nRetries,
                              dataCallback, failureCallback));
}

void
Socket::fetchData(const Name& sessionName, const SeqNo& seqNo,
                  const DataValidatedCallback& dataCallback,
                  const DataValidationErrorCallback& failureCallback,
                  const ndn::TimeoutCallback& onTimeout,
                  int nRetries)
{
  NDN_LOG_DEBUG(">> Socket::fetchData");
  Name interestName;
  interestName.append(sessionName).appendNumber(seqNo);

  Interest interest(interestName);
  interest.setMustBeFresh(true);
  interest.setCanBePrefix(false);

  m_face.expressInterest(interest,
                         bind(&Socket::onData, this, _1, _2, dataCallback, failureCallback),
                         bind(onTimeout, _1), // Nack
                         onTimeout);

  NDN_LOG_DEBUG("<< Socket::fetchData");
}

void
Socket::onInterest(const Name&, const Interest& interest)
{
  auto data = m_ims.find(interest);
  if (data != nullptr) {
    m_face.put(*data);
  }
}

void
Socket::onData(const Interest&, const Data& data,
               const DataValidatedCallback& onValidated,
               const DataValidationErrorCallback& onFailed)
{
  NDN_LOG_DEBUG("Socket::onData");

  if (static_cast<bool>(m_validator))
    m_validator->validate(data, onValidated, onFailed);
  else
    onValidated(data);
}

void
Socket::onDataTimeout(const Interest& interest, int nRetries,
                      const DataValidatedCallback& onValidated,
                      const DataValidationErrorCallback& onFailed)
{
  NDN_LOG_DEBUG("Socket::onDataTimeout");

  if (nRetries <= 0)
    return;

  Interest newNonceInterest(interest);
  newNonceInterest.refreshNonce();

  m_face.expressInterest(newNonceInterest,
                         bind(&Socket::onData, this, _1, _2, onValidated, onFailed),
                         bind(&Socket::onDataTimeout, this, _1, nRetries - 1,
                              onValidated, onFailed), // Nack
                         bind(&Socket::onDataTimeout, this, _1, nRetries - 1,
                              onValidated, onFailed));
}

void
Socket::onDataValidationFailed(const Data& data,
                               const ValidationError& error)
{
}

ConstBufferPtr
Socket::getRootDigest() const
{
  return m_logic.getRootDigest();
}

} // namespace chronosync
