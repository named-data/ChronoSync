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
 * @author Sonu Mishra <https://www.linkedin.com/in/mishrasonu>
 */

#ifndef CHRONOSYNC_LOGIC_HPP
#define CHRONOSYNC_LOGIC_HPP

#include "diff-state-container.hpp"
#include "interest-table.hpp"

#include <ndn-cxx/util/random.hpp>

#include <unordered_map>

namespace chronosync {

/**
 * @brief The missing sequence numbers for a session
 *
 * This class is used to notify the clients of Logic
 * the details of state changes.
 *
 * Instances of this class is usually used as elements of some containers
 * such as std::vector, thus it is copyable.
 */
class NodeInfo
{
public:
  Name userPrefix;
  Name signingId;
  Name sessionName;
  SeqNo seqNo;
};

class MissingDataInfo
{
public:
  /// @brief session name
  Name session;
  /// @brief the lowest one of missing sequence numbers
  SeqNo low;
  /// @brief the highest one of missing sequence numbers
  SeqNo high;
};

/**
 * @brief The callback function to handle state updates
 *
 * The parameter is a set of MissingDataInfo, of which each corresponds to
 * a session that has changed its state.
 */
using UpdateCallback = function<void(const std::vector<MissingDataInfo>&)>;

/**
 * @brief Logic of ChronoSync
 */
class Logic : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    using std::runtime_error::runtime_error;
  };

  static const time::steady_clock::Duration DEFAULT_RESET_TIMER;
  static const time::steady_clock::Duration DEFAULT_CANCEL_RESET_TIMER;
  static const time::milliseconds DEFAULT_RESET_INTEREST_LIFETIME;
  static const time::milliseconds DEFAULT_SYNC_INTEREST_LIFETIME;
  static const time::milliseconds DEFAULT_SYNC_REPLY_FRESHNESS;
  static const time::milliseconds DEFAULT_RECOVERY_INTEREST_LIFETIME;

  /**
   * @brief Constructor
   *
   * @param face The face used to communication, will be shutdown in destructor
   * @param syncPrefix The prefix of the sync group
   * @param defaultUserPrefix The prefix of the first user added to this session
   * @param onUpdate The callback function to handle state updates
   * @param defaultSigningId The signing Id of the default user
   * @param validator The validator for packet validation
   * @param resetTimer The timer to periodically send Reset Interest
   * @param cancelResetTimer The timer to exit from Reset state
   * @param resetInterestLifetime The lifetime of Reset Interest
   * @param syncInterestLifetime The lifetime of sync interest
   * @param syncReplyFreshness The FreshnessPeriod of sync reply
   * @param recoveryInterestLifetime The lifetime of recovery interest
   * @param session Manually defined session ID
   */
  Logic(ndn::Face& face,
        const Name& syncPrefix,
        const Name& defaultUserPrefix,
        const UpdateCallback& onUpdate,
        const Name& defaultSigningId = DEFAULT_NAME,
        std::shared_ptr<Validator> validator = DEFAULT_VALIDATOR,
        const time::steady_clock::Duration& resetTimer = DEFAULT_RESET_TIMER,
        const time::steady_clock::Duration& cancelResetTimer = DEFAULT_CANCEL_RESET_TIMER,
        const time::milliseconds& resetInterestLifetime = DEFAULT_RESET_INTEREST_LIFETIME,
        const time::milliseconds& syncInterestLifetime = DEFAULT_SYNC_INTEREST_LIFETIME,
        const time::milliseconds& syncReplyFreshness = DEFAULT_SYNC_REPLY_FRESHNESS,
        const time::milliseconds& recoveryInterestLifetime = DEFAULT_RECOVERY_INTEREST_LIFETIME,
        const name::Component& session = name::Component());

  ~Logic();

  /**
   * @brief Reset the sync tree (and restart synchronization again)
   *
   * @param isOnInterest a flag that tells whether the reset is called by reset interest.
   */
  void
  reset(bool isOnInterest = false);

  /**
   * @brief Set user prefix
   *
   * This method will also change the default user and signing Id of that user.
   *
   * @param defaultUserPrefix The prefix of user.
   */
  void
  setDefaultUserPrefix(const Name& defaultUserPrefix);

  /// @brief Get the name of default user.
  const Name&
  getDefaultUserPrefix() const
  {
    return m_defaultUserPrefix;
  }

  /**
   * @brief Add user node into the local session.
   *
   * This method also reset after adding
   *
   * @param userPrefix prefix of the added node
   * @param signingId signing Id of the added node
   * @param session manually defined session ID
   * @param shouldSendReset enable/disable sending the reset interest
   */
  void
  addUserNode(const Name& userPrefix, const Name& signingId = DEFAULT_NAME,
              const name::Component& session = name::Component(), bool shouldSendReset = true);

  /// @brief remove the node from the local session
  void
  removeUserNode(const Name& userPrefix);

  /**
   * @brief Get the name of the local session.
   *
   * This method gets the session name according to prefix, if prefix is not specified,
   * it returns the session name of default user.
   *
   * @param prefix prefix of the node
   */
  const Name&
  getSessionName(Name prefix = EMPTY_NAME);

  /**
   * @brief Get current seqNo of the local session.
   *
   * This method gets the seqNo according to prefix, if prefix is not specified,
   * it returns the seqNo of default user.
   *
   * @param prefix prefix of the node
   */
  const SeqNo&
  getSeqNo(Name prefix = EMPTY_NAME);

  /**
   * @brief Update the seqNo of the local session
   *
   * The method updates the existing seqNo with the supplied seqNo and prefix.
   *
   * @param seq The new seqNo.
   * @param updatePrefix The prefix of node to update.
   */
  void
  updateSeqNo(const SeqNo& seq, const Name& updatePrefix = EMPTY_NAME);

  /// @brief Get root digest of current sync tree
  ConstBufferPtr
  getRootDigest() const;

  /// @brief Get the name of all sessions
  std::set<Name>
  getSessionNames() const;

CHRONOSYNC_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  void
  printState(std::ostream& os) const;

  ndn::Scheduler&
  getScheduler()
  {
    return m_scheduler;
  }

  State&
  getState()
  {
    return m_state;
  }

  /// Create a subset @p partialState excluding @p nExcludedStates from @p state
  void
  trimState(State& partialState, const State& state, size_t excludedStates);

  Data
  encodeSyncReply(const Name& nodePrefix, const Name& name, const State& state);

private:
  /**
   * @brief Callback to handle Sync Interest
   *
   * This method checks whether an incoming interest is a normal one or a reset
   * and dispatches the incoming interest to corresponding processing methods.
   *
   * @param prefix   The prefix of the sync group.
   * @param interest The incoming sync interest.
   */
  void
  onSyncInterest(const Name& prefix, const Interest& interest);

  /**
   * @brief Callback to handle Sync prefix registration failure
   *
   * This method does nothing for now.
   *
   * @param prefix The prefix of the sync group.
   * @param msg    The error message.
   */
  void
  onSyncRegisterFailed(const Name& prefix, const std::string& msg);

  /**
   * @brief Callback to handle Sync Reply
   *
   * This method calls validator to validate Sync Reply.
   * For now, validation is disabled, Logic::onSyncDataValidated is called
   * directly.
   *
   * @param interest The Sync Interest
   * @param data     The reply to the Sync Interest
   */
  void
  onSyncData(const Interest& interest, const Data& data);

  /**
   * @brief Callback to handle reply to Reset Interest.
   *
   * This method does nothing, since reply to Reset Interest is not useful for now.
   *
   * @param interest The Reset Interest
   * @param data     The reply to the Reset Interest
   */
  void
  onResetData(const Interest& interest, const Data& data);

  /**
   * @brief Callback to handle Sync Interest Nack
   *
   * This method checks whether the Nack is of type NoRoute
   * and schedules a sync interest in m_reexpressionJitter
   *
   * @param interest The sync interest for which the Nack happened
   * @param nack The incoming Nack
   */
  void
  onSyncNack(const Interest& interest, const ndn::lp::Nack& nack);

  /**
   * @brief Callback to handle Sync Interest timeout.
   *
   * This method does nothing, since Logic per se handles timeout explicitly.
   *
   * @param interest The Sync Interest
   */
  void
  onSyncTimeout(const Interest& interest);

  /**
   * @brief Callback to invalid Sync Reply.
   *
   * This method does nothing but drops the invalid reply.
   *
   * @param data The invalid Sync Reply
   */
  void
  onSyncDataValidationFailed(const Data& data);

  /**
   * @brief Callback to valid Sync Reply.
   *
   * This method simply passes the valid reply to processSyncData.
   *
   * @param data The valid Sync Reply.
   */
  void
  onSyncDataValidated(const Data& data);

  /**
   * @brief Process normal Sync Interest
   *
   * This method extracts the digest from the incoming Sync Interest,
   * compares it against current local digest, and process the Sync
   * Interest according to the comparison result.  See docs/design.rst
   * for more details.
   *
   * @param interest          The incoming interest
   * @param isTimedProcessing True if the interest needs an immediate reply,
   *                          otherwise hold the interest for a while before
   *                          making a reply (to avoid unnecessary recovery)
   */
  void
  processSyncInterest(const Interest& interest, bool isTimedProcessing = false);

  /**
   * @brief Process reset Sync Interest
   *
   * This method simply call Logic::reset()
   *
   * @param interest The incoming interest.
   */
  void
  processResetInterest(const Interest& interest);

  /**
   * @brief Process Sync Reply.
   *
   * This method extracts state update information from Sync Reply and applies
   * it to the Sync Tree and re-express Sync Interest.
   *
   * @param name      The data name of the Sync Reply.
   * @param digest    The digest in the data name.
   * @param syncReply The content of the Sync Reply.
   */
  void
  processSyncData(const Name& name, ConstBufferPtr digest, const Block& syncReply);

  /**
   * @brief Insert state diff into log
   *
   * @param diff         The diff.
   * @param previousRoot The root digest before state changes.
   */
  void
  insertToDiffLog(DiffStatePtr diff, ConstBufferPtr previousRoot);

  /**
   * @brief Reply to all pending Sync Interests with a particular commit (or diff)
   *
   * @param commit The diff.
   */
  void
  satisfyPendingSyncInterests(const Name& updatedPrefix, ConstDiffStatePtr commit);

  /// @brief Helper method to send normal Sync Interest
  void
  sendSyncInterest();

  /// @brief Helper method to send reset Sync Interest
  void
  sendResetInterest();

  /// @brief Helper method to send Sync Reply
  void
  sendSyncData(const Name& nodePrefix, const Name& name, const State& state);

  /**
   * @brief Unset reset status
   *
   * By invoking this method, one can add its own state into the Sync Tree, thus
   * jumping out of the reset status
   */
  void
  cancelReset();

  void
  printDigest(ConstBufferPtr digest);

  /**
   * @brief Helper method to send Recovery Interest
   *
   * @param digest    The digest to be included in the recovery interest
   */
  void
  sendRecoveryInterest(ConstBufferPtr digest);

  /**
   * @brief Process Recovery Interest
   *
   * This method extracts the digest from the incoming Recovery Interest.
   * If it recognizes this incoming digest, then it sends its full state
   * as reply.
   *
   * @param interest    The incoming interest
   */
  void
  processRecoveryInterest(const Interest& interest);

  /**
   * @brief Callback to handle Recovery Reply
   *
   * This method calls Logic::onSyncDataValidated directly.
   *
   * @param interest The Recovery Interest
   * @param data     The reply to the Recovery Interest
   */
  void
  onRecoveryData(const Interest& interest, const Data& data);

  /**
   * @brief Callback to handle Recovery Interest timeout.
   *
   * This method does nothing.
   *
   * @param interest The Recovery Interest
   */
  void
  onRecoveryTimeout(const Interest& interest);

public:
  static const ndn::Name DEFAULT_NAME;
  static const ndn::Name EMPTY_NAME;
  static const std::shared_ptr<Validator> DEFAULT_VALIDATOR;

private:
  using NodeList = std::unordered_map<ndn::Name, NodeInfo>;

  static const ConstBufferPtr EMPTY_DIGEST;
  static const ndn::name::Component RESET_COMPONENT;
  static const ndn::name::Component RECOVERY_COMPONENT;

  // Communication
  ndn::Face& m_face;
  Name m_syncPrefix;
  ndn::ScopedRegisteredPrefixHandle m_syncRegisteredPrefix;
  Name m_syncReset;
  Name m_defaultUserPrefix;

  // State
  NodeList m_nodeList;
  State m_state;
  DiffStateContainer m_log;
  InterestTable m_interestTable;
  Name m_pendingSyncInterestName;
  ndn::ScopedPendingInterestHandle m_pendingSyncInterest;
  ndn::ScopedPendingInterestHandle m_pendingResetInterest;
  std::map<std::string, ndn::ScopedPendingInterestHandle> m_pendingRecoveryInterests;
  bool m_isInReset;
  bool m_needPeriodReset;

  // Callback
  UpdateCallback m_onUpdate;

  // Event
  ndn::Scheduler m_scheduler;
  ndn::scheduler::ScopedEventId m_delayedInterestProcessingId;
  ndn::scheduler::ScopedEventId m_reexpressingInterestId;
  ndn::scheduler::ScopedEventId m_resetInterestId;

  // Timer
  ndn::random::RandomNumberEngine& m_rng;
  std::uniform_int_distribution<> m_rangeUniformRandom;
  std::uniform_int_distribution<> m_reexpressionJitter;
  /// @brief Timer to send next reset 0 for no reset
  time::steady_clock::Duration m_resetTimer;
  /// @brief Timer to cancel reset state
  time::steady_clock::Duration m_cancelResetTimer;
  /// @brief Lifetime of reset interest
  time::milliseconds m_resetInterestLifetime;
  /// @brief Lifetime of sync interest
  time::milliseconds m_syncInterestLifetime;
  /// @brief FreshnessPeriod of SyncReply
  time::milliseconds m_syncReplyFreshness;
  /// @brief Lifetime of recovery interest
  time::milliseconds m_recoveryInterestLifetime;

  // Security
  ndn::KeyChain m_keyChain;
  std::shared_ptr<Validator> m_validator;

  int m_instanceId;
  static int s_instanceCounter;
};

#ifdef CHRONOSYNC_WITH_TESTS
size_t
getMaxPacketLimit();
#endif // CHRONOSYNC_WITH_TESTS

} // namespace chronosync

#endif // CHRONOSYNC_LOGIC_HPP
