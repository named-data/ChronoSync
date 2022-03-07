/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2022 University of California, Los Angeles
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
 * @author Sonu Mishra <https://www.linkedin.com/in/mishrasonu>
 */

#include "logic.hpp"
#include "detail/bzip2-helper.hpp"

#include <ndn-cxx/util/backports.hpp>
#include <ndn-cxx/util/exception.hpp>
#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/string-helper.hpp>

NDN_LOG_INIT(sync.Logic);

#define CHRONO_LOG_DBG(v) NDN_LOG_DEBUG("Instance" << m_instanceId << ": " << v)

namespace chronosync {

const uint8_t EMPTY_DIGEST_VALUE[] = {
  0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
  0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
  0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
  0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
};

int Logic::s_instanceCounter = 0;

const ndn::Name Logic::DEFAULT_NAME;
const ndn::Name Logic::EMPTY_NAME;
const std::shared_ptr<Validator> Logic::DEFAULT_VALIDATOR;
const time::steady_clock::Duration Logic::DEFAULT_RESET_TIMER = time::seconds(0);
const time::steady_clock::Duration Logic::DEFAULT_CANCEL_RESET_TIMER = time::milliseconds(500);
const time::milliseconds Logic::DEFAULT_RESET_INTEREST_LIFETIME(1000);
const time::milliseconds Logic::DEFAULT_SYNC_INTEREST_LIFETIME(1000);
const time::milliseconds Logic::DEFAULT_SYNC_REPLY_FRESHNESS(1000);
const time::milliseconds Logic::DEFAULT_RECOVERY_INTEREST_LIFETIME(1000);

const ConstBufferPtr Logic::EMPTY_DIGEST(new ndn::Buffer(EMPTY_DIGEST_VALUE, 32));
const ndn::name::Component Logic::RESET_COMPONENT("reset");
const ndn::name::Component Logic::RECOVERY_COMPONENT("recovery");

const size_t NDNLP_EXPECTED_OVERHEAD = 20;

/**
 * Get maximum packet limit
 *
 * By default, it returns `ndn::MAX_NDN_PACKET_SIZE`.
 * The returned value can be customized using the environment variable `CHRONOSYNC_MAX_PACKET_SIZE`,
 * but the returned value will be at least 500 and no more than `ndn::MAX_NDN_PACKET_SIZE`.
 */
#ifndef CHRONOSYNC_WITH_TESTS
static
#endif // CHRONOSYNC_WITH_TESTS
size_t
getMaxPacketLimit()
{
  static size_t limit = 0;
#ifndef CHRONOSYNC_WITH_TESTS
  if (limit != 0) {
    return limit;
  }
#endif // CHRONOSYNC_WITH_TESTS

  if (getenv("CHRONOSYNC_MAX_PACKET_SIZE") != nullptr) {
    try {
      limit = ndn::clamp<size_t>(boost::lexical_cast<size_t>(getenv("CHRONOSYNC_MAX_PACKET_SIZE")),
                                 500, ndn::MAX_NDN_PACKET_SIZE);
    }
    catch (const boost::bad_lexical_cast&) {
      limit = ndn::MAX_NDN_PACKET_SIZE;
    }
  }
  else {
    limit = ndn::MAX_NDN_PACKET_SIZE;
  }

  return limit;
}

Logic::Logic(ndn::Face& face,
             const Name& syncPrefix,
             const Name& defaultUserPrefix,
             const UpdateCallback& onUpdate,
             const Name& defaultSigningId,
             std::shared_ptr<Validator> validator,
             const time::steady_clock::Duration& resetTimer,
             const time::steady_clock::Duration& cancelResetTimer,
             const time::milliseconds& resetInterestLifetime,
             const time::milliseconds& syncInterestLifetime,
             const time::milliseconds& syncReplyFreshness,
             const time::milliseconds& recoveryInterestLifetime,
             const name::Component& session)
  : m_face(face)
  , m_syncPrefix(syncPrefix)
  , m_syncReset(Name(syncPrefix).append("reset"))
  , m_defaultUserPrefix(defaultUserPrefix)
  , m_interestTable(m_face.getIoService())
  , m_isInReset(false)
  , m_needPeriodReset(resetTimer > time::nanoseconds::zero())
  , m_onUpdate(onUpdate)
  , m_scheduler(m_face.getIoService())
  , m_rng(ndn::random::getRandomNumberEngine())
  , m_rangeUniformRandom(100, 500)
  , m_reexpressionJitter(100, 500)
  , m_resetTimer(resetTimer)
  , m_cancelResetTimer(cancelResetTimer)
  , m_resetInterestLifetime(resetInterestLifetime)
  , m_syncInterestLifetime(syncInterestLifetime)
  , m_syncReplyFreshness(syncReplyFreshness)
  , m_recoveryInterestLifetime(recoveryInterestLifetime)
  , m_validator(validator)
  , m_instanceId(s_instanceCounter++)
{
  CHRONO_LOG_DBG(">> Logic::Logic");
  addUserNode(m_defaultUserPrefix, defaultSigningId, session, false);

  CHRONO_LOG_DBG("Listen to: " << m_syncPrefix);
  m_syncRegisteredPrefix = m_face.setInterestFilter(
    ndn::InterestFilter(m_syncPrefix).allowLoopback(false),
    bind(&Logic::onSyncInterest, this, _1, _2),
    bind(&Logic::onSyncRegisterFailed, this, _1, _2));

  sendSyncInterest();
  CHRONO_LOG_DBG("<< Logic::Logic");
}

Logic::~Logic()
{
  CHRONO_LOG_DBG(">> Logic::~Logic");
  m_interestTable.clear();
  m_scheduler.cancelAllEvents();
  CHRONO_LOG_DBG("<< Logic::~Logic");
}

void
Logic::reset(bool isOnInterest)
{
  m_isInReset = true;

  m_state.reset();
  m_log.clear();

  if (!isOnInterest)
    sendResetInterest();

  sendSyncInterest();

  m_delayedInterestProcessingId = m_scheduler.schedule(m_cancelResetTimer, [this] { cancelReset(); });
}

void
Logic::setDefaultUserPrefix(const Name& defaultUserPrefix)
{
  if (defaultUserPrefix != EMPTY_NAME) {
    if (m_nodeList.find(defaultUserPrefix) != m_nodeList.end()) {
      m_defaultUserPrefix = defaultUserPrefix;
    }
  }
}

void
Logic::addUserNode(const Name& userPrefix, const Name& signingId, const name::Component& session, bool shouldSendReset)
{
  if (userPrefix == EMPTY_NAME)
    return;
  if (m_defaultUserPrefix == EMPTY_NAME) {
    m_defaultUserPrefix = userPrefix;
  }
  if (m_nodeList.find(userPrefix) == m_nodeList.end()) {
    m_nodeList[userPrefix].userPrefix = userPrefix;
    m_nodeList[userPrefix].signingId = signingId;
    Name sessionName = userPrefix;
    if (!session.empty()) {
      sessionName.append(session);
    }
    else {
      sessionName.appendNumber(time::toUnixTimestamp(time::system_clock::now()).count());
    }
    m_nodeList[userPrefix].sessionName = sessionName;
    m_nodeList[userPrefix].seqNo = 0;
    reset(!shouldSendReset);
  }
}

void
Logic::removeUserNode(const Name& userPrefix)
{
  auto userNode = m_nodeList.find(userPrefix);
  if (userNode != m_nodeList.end()) {
    m_nodeList.erase(userNode);
    if (m_defaultUserPrefix == userPrefix) {
      if (!m_nodeList.empty()) {
        m_defaultUserPrefix = m_nodeList.begin()->second.userPrefix;
      }
      else {
        m_defaultUserPrefix = EMPTY_NAME;
      }
    }
    reset(false);
  }
}

const Name&
Logic::getSessionName(Name prefix)
{
  if (prefix == EMPTY_NAME)
    prefix = m_defaultUserPrefix;

  auto node = m_nodeList.find(prefix);
  if (node != m_nodeList.end())
    return node->second.sessionName;

  NDN_THROW(Error("Nonexistent node: " + prefix.toUri()));
}

const SeqNo&
Logic::getSeqNo(Name prefix)
{
  if (prefix == EMPTY_NAME)
    prefix = m_defaultUserPrefix;

  auto node = m_nodeList.find(prefix);
  if (node != m_nodeList.end())
    return node->second.seqNo;

  NDN_THROW(Error("Nonexistent node: " + prefix.toUri()));
}

void
Logic::updateSeqNo(const SeqNo& seqNo, const Name& updatePrefix)
{
  Name prefix;
  if (updatePrefix == EMPTY_NAME) {
    if (m_defaultUserPrefix == EMPTY_NAME)
      return;
    prefix = m_defaultUserPrefix;
  }
  else
    prefix = updatePrefix;

  auto it = m_nodeList.find(prefix);
  if (it != m_nodeList.end()) {
    NodeInfo& node = it->second;
    CHRONO_LOG_DBG(">> Logic::updateSeqNo");
    CHRONO_LOG_DBG("seqNo: " << seqNo << " m_seqNo: " << node.seqNo);
    if (seqNo < node.seqNo || seqNo == 0)
      return;

    node.seqNo = seqNo;
    CHRONO_LOG_DBG("updateSeqNo: m_seqNo " << node.seqNo);

    if (!m_isInReset) {
      CHRONO_LOG_DBG("updateSeqNo: not in Reset");
      ConstBufferPtr previousRoot = m_state.getRootDigest();
      {
        std::string hash = ndn::toHex(previousRoot->data(), previousRoot->size(), false);
        CHRONO_LOG_DBG("Hash: " << hash);
      }

      bool isInserted = false;
      bool isUpdated = false;
      SeqNo oldSeq;
      std::tie(isInserted, isUpdated, oldSeq) = m_state.update(node.sessionName, node.seqNo);

      CHRONO_LOG_DBG("Insert: " << std::boolalpha << isInserted);
      CHRONO_LOG_DBG("Updated: " << std::boolalpha << isUpdated);
      if (isInserted || isUpdated) {
        DiffStatePtr commit = make_shared<DiffState>();
        commit->update(node.sessionName, node.seqNo);
        commit->setRootDigest(m_state.getRootDigest());
        insertToDiffLog(commit, previousRoot);

        satisfyPendingSyncInterests(prefix, commit);
      }
    }
  }
}

ConstBufferPtr
Logic::getRootDigest() const
{
  return m_state.getRootDigest();
}

void
Logic::printState(std::ostream& os) const
{
  for (const auto& leaf : m_state.getLeaves()) {
    os << *leaf << "\n";
  }
}

std::set<Name>
Logic::getSessionNames() const
{
  std::set<Name> sessionNames;
  for (const auto& leaf : m_state.getLeaves()) {
    sessionNames.insert(leaf->getSessionName());
  }
  return sessionNames;
}

void
Logic::onSyncInterest(const Name&, const Interest& interest)
{
  CHRONO_LOG_DBG(">> Logic::onSyncInterest");
  Name name = interest.getName();

  CHRONO_LOG_DBG("InterestName: " << name);

  if (name.size() >= 1 && RESET_COMPONENT == name.get(-1)) {
    processResetInterest(interest);
  }
  else if (name.size() >= 2 && RECOVERY_COMPONENT == name.get(-2)) {
    processRecoveryInterest(interest);
  }
  else {
    processSyncInterest(interest);
  }

  CHRONO_LOG_DBG("<< Logic::onSyncInterest");
}

void
Logic::onSyncRegisterFailed(const Name& prefix, const std::string& msg)
{
  CHRONO_LOG_DBG(">> Logic::onSyncRegisterFailed");
}

void
Logic::onSyncData(const Interest&, const Data& data)
{
  CHRONO_LOG_DBG(">> Logic::onSyncData");
  if (m_validator != nullptr)
    m_validator->validate(data,
                          bind(&Logic::onSyncDataValidated, this, _1),
                          bind(&Logic::onSyncDataValidationFailed, this, _1));
  else
     onSyncDataValidated(data);

  CHRONO_LOG_DBG("<< Logic::onSyncData");
}

void
Logic::onResetData(const Interest&, const Data&)
{
  // This should not happened, drop the received data.
}

void
Logic::onSyncNack(const Interest&, const ndn::lp::Nack& nack)
{
  CHRONO_LOG_DBG(">> Logic::onSyncNack");
  if (nack.getReason() == ndn::lp::NackReason::NO_ROUTE) {
    auto after = ndn::time::milliseconds(m_reexpressionJitter(m_rng));
    CHRONO_LOG_DBG("Schedule sync interest after: " << after);
    m_scheduler.schedule(after, [this] { sendSyncInterest(); });
  }
  CHRONO_LOG_DBG("<< Logic::onSyncNack");
}

void
Logic::onSyncTimeout(const Interest& interest)
{
  // It is OK. Others will handle the time out situation.
  CHRONO_LOG_DBG(">> Logic::onSyncTimeout");
  CHRONO_LOG_DBG("Interest: " << interest.getName());
  CHRONO_LOG_DBG("<< Logic::onSyncTimeout");
}

void
Logic::onSyncDataValidationFailed(const Data&)
{
  // SyncReply cannot be validated.
}

void
Logic::onSyncDataValidated(const Data& data)
{
  Name name = data.getName();
  ConstBufferPtr digest = make_shared<ndn::Buffer>(name.get(-1).value(), name.get(-1).value_size());

  try {
    auto contentBuffer = bzip2::decompress(reinterpret_cast<const char*>(data.getContent().value()),
                                           data.getContent().value_size());
    processSyncData(name, digest, Block(std::move(contentBuffer)));
  }
  catch (const std::ios_base::failure& error) {
    NDN_LOG_WARN("Error decompressing content of " << data.getName() << " (" << error.what() << ")");
  }
}

void
Logic::processSyncInterest(const Interest& interest, bool isTimedProcessing/*=false*/)
{
  CHRONO_LOG_DBG(">> Logic::processSyncInterest");

  Name name = interest.getName();
  ConstBufferPtr digest = make_shared<ndn::Buffer>(name.get(-1).value(), name.get(-1).value_size());

  ConstBufferPtr rootDigest = m_state.getRootDigest();

  // If the digest of the incoming interest is the same as root digest
  // Put the interest into InterestTable
  if (*rootDigest == *digest) {
    CHRONO_LOG_DBG("Oh, we are in the same state");
    m_interestTable.insert(interest, digest, false);

    if (!m_isInReset)
      return;

    if (!isTimedProcessing) {
      CHRONO_LOG_DBG("Non timed processing in reset");
      // Still in reset, our own seq has not been put into state yet
      // Do not hurry, some others may be also resetting and may send their reply
      time::milliseconds after(m_rangeUniformRandom(m_rng));
      CHRONO_LOG_DBG("After: " << after);
      m_delayedInterestProcessingId = m_scheduler.schedule(after,
                                                           [=] { processSyncInterest(interest, true); });
    }
    else {
      CHRONO_LOG_DBG("Timed processing in reset");
      // Now we can get out of reset state by putting our own stuff into m_state.
      cancelReset();
    }

    return;
  }

  // If the digest of incoming interest is an "empty" digest
  if (*digest == *EMPTY_DIGEST) {
    CHRONO_LOG_DBG("Poor guy, he knows nothing");
    sendSyncData(m_defaultUserPrefix, name, m_state);
    return;
  }

  auto stateIter = m_log.find(digest);
  // If the digest of incoming interest can be found from the log
  if (stateIter != m_log.end()) {
    CHRONO_LOG_DBG("It is ok, you are so close");
    sendSyncData(m_defaultUserPrefix, name, *(*stateIter)->diff());
    return;
  }

  if (!isTimedProcessing) {
    CHRONO_LOG_DBG("Let's wait, just wait for a while");
    // Do not hurry, some incoming SyncReplies may help us to recognize the digest
    m_interestTable.insert(interest, digest, true);

    m_delayedInterestProcessingId =
      m_scheduler.schedule(time::milliseconds(m_rangeUniformRandom(m_rng)),
                           [=] { processSyncInterest(interest, true); });
  }
  else {
    // OK, nobody is helping us, just tell the truth.
    CHRONO_LOG_DBG("OK, nobody is helping us, let us try to recover");
    m_interestTable.erase(digest);
    sendRecoveryInterest(digest);
  }

  CHRONO_LOG_DBG("<< Logic::processSyncInterest");
}

void
Logic::processResetInterest(const Interest&)
{
  CHRONO_LOG_DBG(">> Logic::processResetInterest");
  reset(true);
}

void
Logic::processSyncData(const Name&, ConstBufferPtr digest, const Block& syncReply)
{
  CHRONO_LOG_DBG(">> Logic::processSyncData");
  DiffStatePtr commit = make_shared<DiffState>();
  ConstBufferPtr previousRoot = m_state.getRootDigest();

  try {
    m_interestTable.erase(digest); // Remove satisfied interest from PIT

    State reply;
    reply.wireDecode(syncReply);

    std::vector<MissingDataInfo> v;
    for (const auto& leaf : reply.getLeaves().get<ordered>()) {
      BOOST_ASSERT(leaf != nullptr);

      const Name& info = leaf->getSessionName();
      SeqNo seq = leaf->getSeq();

      bool isInserted = false;
      bool isUpdated = false;
      SeqNo oldSeq;
      std::tie(isInserted, isUpdated, oldSeq) = m_state.update(info, seq);
      if (isInserted || isUpdated) {
        commit->update(info, seq);
        oldSeq++;
        v.push_back({info, oldSeq, seq});
      }
    }

    if (!v.empty()) {
      m_onUpdate(v);

      commit->setRootDigest(m_state.getRootDigest());
      insertToDiffLog(commit, previousRoot);
    }
    else {
      CHRONO_LOG_DBG("What? nothing new");
    }
  }
  catch (const State::Error&) {
    CHRONO_LOG_DBG("Something really fishy happened during state decoding");
    commit.reset();
    return;
  }

  if (static_cast<bool>(commit) && !commit->getLeaves().empty()) {
    // state changed and it is safe to express a new interest
    auto after = time::milliseconds(m_reexpressionJitter(m_rng));
    CHRONO_LOG_DBG("Reschedule sync interest after: " << after);
    m_reexpressingInterestId = m_scheduler.schedule(after, [this] { sendSyncInterest(); });
  }
}

void
Logic::satisfyPendingSyncInterests(const Name& updatedPrefix, ConstDiffStatePtr commit)
{
  CHRONO_LOG_DBG(">> Logic::satisfyPendingSyncInterests");
  try {
    CHRONO_LOG_DBG("InterestTable size: " << m_interestTable.size());
    auto it = m_interestTable.begin();
    while (it != m_interestTable.end()) {
      ConstUnsatisfiedInterestPtr request = *it;
      ++it;
      if (request->isUnknown)
        sendSyncData(updatedPrefix, request->interest.getName(), m_state);
      else
        sendSyncData(updatedPrefix, request->interest.getName(), *commit);
    }
    m_interestTable.clear();
  }
  catch (const InterestTable::Error&) {
    // ok. not really an error
  }
  CHRONO_LOG_DBG("<< Logic::satisfyPendingSyncInterests");
}

void
Logic::insertToDiffLog(DiffStatePtr commit, ConstBufferPtr previousRoot)
{
  CHRONO_LOG_DBG(">> Logic::insertToDiffLog");
  // Connect to the history
  if (!m_log.empty())
    (*m_log.find(previousRoot))->setNext(commit);

  // Insert the commit
  m_log.erase(commit->getRootDigest());
  m_log.insert(commit);
  CHRONO_LOG_DBG("<< Logic::insertToDiffLog");
}

void
Logic::sendResetInterest()
{
  CHRONO_LOG_DBG(">> Logic::sendResetInterest");

  if (m_needPeriodReset) {
    CHRONO_LOG_DBG("Need Period Reset");
    CHRONO_LOG_DBG("ResetTimer: " << m_resetTimer);

    m_resetInterestId = m_scheduler.schedule(m_resetTimer + time::milliseconds(m_reexpressionJitter(m_rng)),
                                             [this] { sendResetInterest(); });
  }

  Interest interest(m_syncReset);
  interest.setMustBeFresh(true);
  interest.setInterestLifetime(m_resetInterestLifetime);

  // Assigning to m_pendingResetInterest cancels the previous reset Interest.
  // This is harmless since no Data is expected.
  m_pendingResetInterest = m_face.expressInterest(interest,
    bind(&Logic::onResetData, this, _1, _2),
    bind(&Logic::onSyncTimeout, this, _1), // Nack
    bind(&Logic::onSyncTimeout, this, _1));
  CHRONO_LOG_DBG("<< Logic::sendResetInterest");
}

void
Logic::sendSyncInterest()
{
  CHRONO_LOG_DBG(">> Logic::sendSyncInterest");

  Name interestName;
  interestName.append(m_syncPrefix)
    .append(ndn::name::Component(*m_state.getRootDigest()));

  m_pendingSyncInterestName = interestName;

#ifdef _DEBUG
  printDigest(m_state.getRootDigest());
#endif

  m_reexpressingInterestId = m_scheduler.schedule(m_syncInterestLifetime / 2 +
                                                  time::milliseconds(m_reexpressionJitter(m_rng)),
                                                  [this] { sendSyncInterest(); });

  Interest interest(interestName);
  interest.setMustBeFresh(true);
  interest.setCanBePrefix(true);
  interest.setInterestLifetime(m_syncInterestLifetime);

  m_pendingSyncInterest = m_face.expressInterest(interest,
                                                 bind(&Logic::onSyncData, this, _1, _2),
                                                 bind(&Logic::onSyncNack, this, _1, _2),
                                                 bind(&Logic::onSyncTimeout, this, _1));

  CHRONO_LOG_DBG("Send interest: " << interest.getName());
  CHRONO_LOG_DBG("<< Logic::sendSyncInterest");
}

void
Logic::trimState(State& partialState, const State& state, size_t nExcludedStates)
{
  partialState.reset();

  std::vector<ConstLeafPtr> leaves;
  for (const auto& leaf : state.getLeaves()) {
    leaves.push_back(leaf);
  }

  std::shuffle(leaves.begin(), leaves.end(), m_rng);

  size_t statesToEncode = leaves.size() - std::min(leaves.size() - 1, nExcludedStates);
  for (const auto& leaf : leaves) {
    if (statesToEncode == 0) {
      break;
    }
    partialState.update(leaf->getSessionName(), leaf->getSeq());
    --statesToEncode;
  }
}

Data
Logic::encodeSyncReply(const Name& nodePrefix, const Name& name, const State& state)
{
  Data syncReply(name);
  syncReply.setFreshnessPeriod(m_syncReplyFreshness);

  auto finalizeReply = [this, &nodePrefix, &syncReply] (const State& state) {
    auto contentBuffer = bzip2::compress(reinterpret_cast<const char*>(state.wireEncode().wire()),
                                         state.wireEncode().size());
    syncReply.setContent(contentBuffer);

    if (m_nodeList[nodePrefix].signingId.empty())
      m_keyChain.sign(syncReply);
    else
      m_keyChain.sign(syncReply, security::signingByIdentity(m_nodeList[nodePrefix].signingId));
  };

  finalizeReply(state);

  size_t nExcludedStates = 1;
  while (syncReply.wireEncode().size() > getMaxPacketLimit() - NDNLP_EXPECTED_OVERHEAD) {
    if (nExcludedStates == 1) {
      // To show this debug message only once
      NDN_LOG_DEBUG("Sync reply size exceeded maximum packet limit ("
                    << (getMaxPacketLimit() - NDNLP_EXPECTED_OVERHEAD) << ")");
    }
    State partialState;
    trimState(partialState, state, nExcludedStates);
    finalizeReply(partialState);

    BOOST_ASSERT(state.getLeaves().size() != 0);
    nExcludedStates *= 2;
  }

  return syncReply;
}

void
Logic::sendSyncData(const Name& nodePrefix, const Name& name, const State& state)
{
  CHRONO_LOG_DBG(">> Logic::sendSyncData");
  if (m_nodeList.find(nodePrefix) == m_nodeList.end())
    return;

  m_face.put(encodeSyncReply(nodePrefix, name, state));

  // checking if our own interest got satisfied
  if (m_pendingSyncInterestName == name) {
    // remove outstanding interest
    m_pendingSyncInterest.cancel();

    // re-schedule sending Sync interest
    time::milliseconds after(m_reexpressionJitter(m_rng));
    CHRONO_LOG_DBG("Satisfy our own interest");
    CHRONO_LOG_DBG("Reschedule sync interest after " << after);
    m_reexpressingInterestId = m_scheduler.schedule(after, [this] { sendSyncInterest(); });
  }
  CHRONO_LOG_DBG("<< Logic::sendSyncData");
}

void
Logic::cancelReset()
{
  CHRONO_LOG_DBG(">> Logic::cancelReset");
  if (!m_isInReset)
    return;

  m_isInReset = false;
  for (const auto& node : m_nodeList) {
    updateSeqNo(node.second.seqNo, node.first);
  }
  CHRONO_LOG_DBG("<< Logic::cancelReset");
}

void
Logic::printDigest(ConstBufferPtr digest)
{
  std::string hash = ndn::toHex(digest->data(), digest->size(), false);
  CHRONO_LOG_DBG("Hash: " << hash);
}

void
Logic::sendRecoveryInterest(ConstBufferPtr digest)
{
  CHRONO_LOG_DBG(">> Logic::sendRecoveryInterest");

  Name interestName;
  interestName.append(m_syncPrefix)
              .append(RECOVERY_COMPONENT)
              .append(ndn::name::Component(*digest));

  Interest interest(interestName);
  interest.setMustBeFresh(true);
  interest.setCanBePrefix(true);
  interest.setInterestLifetime(m_recoveryInterestLifetime);

  m_pendingRecoveryInterests[interestName[-1].toUri()] = m_face.expressInterest(interest,
    bind(&Logic::onRecoveryData, this, _1, _2),
    bind(&Logic::onRecoveryTimeout, this, _1), // Nack
    bind(&Logic::onRecoveryTimeout, this, _1));
  CHRONO_LOG_DBG("interest: " << interest.getName());
  CHRONO_LOG_DBG("<< Logic::sendRecoveryInterest");
}

void
Logic::processRecoveryInterest(const Interest& interest)
{
  CHRONO_LOG_DBG(">> Logic::processRecoveryInterest");

  Name name = interest.getName();
  ConstBufferPtr digest = make_shared<ndn::Buffer>(name.get(-1).value(), name.get(-1).value_size());
  ConstBufferPtr rootDigest = m_state.getRootDigest();

  auto stateIter = m_log.find(digest);
  if (stateIter != m_log.end() || *digest == *EMPTY_DIGEST || *rootDigest == *digest) {
    CHRONO_LOG_DBG("I can help you recover");
    sendSyncData(m_defaultUserPrefix, name, m_state);
    return;
  }

  CHRONO_LOG_DBG("<< Logic::processRecoveryInterest");
}

void
Logic::onRecoveryData(const Interest& interest, const Data& data)
{
  CHRONO_LOG_DBG(">> Logic::onRecoveryData");
  m_pendingRecoveryInterests.erase(interest.getName()[-1].toUri());
  onSyncDataValidated(data);
  CHRONO_LOG_DBG("<< Logic::onRecoveryData");
}

void
Logic::onRecoveryTimeout(const Interest& interest)
{
  CHRONO_LOG_DBG(">> Logic::onRecoveryTimeout");
  m_pendingRecoveryInterests.erase(interest.getName()[-1].toUri());
  CHRONO_LOG_DBG("Interest: " << interest.getName());
  CHRONO_LOG_DBG("<< Logic::onRecoveryTimeout");
}

} // namespace chronosync
