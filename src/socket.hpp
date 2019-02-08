/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2019 University of California, Los Angeles
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

#ifndef CHRONOSYNC_SOCKET_HPP
#define CHRONOSYNC_SOCKET_HPP

#include "logic.hpp"

#include <unordered_map>

#include <ndn-cxx/ims/in-memory-storage-persistent.hpp>

namespace chronosync {

/**
 * @brief A simple interface to interact with client code
 *
 * Though it is called Socket, it is not a real socket. It just trying to provide
 * a simplified interface for data publishing and fetching.
 *
 * This interface simplify data publishing.  Client can simply dump raw data
 * into this interface without handling the ChronoSync specific details, such
 * as sequence number and session id.
 *
 * This interface also simplify data fetching.  Client only needs to provide a
 * data fetching strategy (through a updateCallback).
 */
class Socket : noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  Socket(const Name& syncPrefix,
         const Name& userPrefix,
         ndn::Face& face,
         const UpdateCallback& updateCallback,
         const Name& signingId = DEFAULT_NAME,
         std::shared_ptr<Validator> validator = DEFAULT_VALIDATOR,
         const time::milliseconds& syncInterestLifetime = Logic::DEFAULT_SYNC_INTEREST_LIFETIME,
         const name::Component& session = name::Component());

  ~Socket();

  using DataValidatedCallback = function<void(const Data&)>;

  using DataValidationErrorCallback = function<void(const Data&, const ValidationError& error)> ;

  /**
   * @brief Add a sync node under same logic
   *
   * This method will add a new sync node in logic and then register this prefix.
   * If pass an empty prefix, it will return directly without doing anything
   * If the prefix is already registered, return directly
   *
   * @param prefix Prefix of the new node
   * @param signingId Signing ID for the packet sent out by the new node
   * @param session Manually defined session number
   */
  void
  addSyncNode(const Name& prefix, const Name& signingId = DEFAULT_NAME,
              const name::Component& session = name::Component());

  /**
   * @brief Remove a sync node under same logic
   *
   * This method will remove a sync node in logic, unregister this prefix
   * and then clear the in memory storage about this prefix.
   * logic will be reset after removal of this node
   *
   * @param prefix Prefix of the node to remove
   */
  void
  removeSyncNode(const Name& prefix);

  /**
   * @brief Publish a data packet in the session and trigger synchronization updates
   *
   * This method will create a data packet with the supplied content.
   * The packet name is the local session + seqNo.
   * The seqNo is automatically maintained by internal Logic.
   *
   * @throws It will throw error, if the prefix does not exist in m_logic
   *
   * @param buf Pointer to the bytes in content
   * @param len size of the bytes in content
   * @param freshness FreshnessPeriod of the data packet.
   * @param prefix The user prefix that will be used to publish the data.
   */
  void
  publishData(const uint8_t* buf, size_t len, const ndn::time::milliseconds& freshness,
              const Name& prefix = DEFAULT_PREFIX);

  /**
   * @brief Publish a data packet in the session and trigger synchronization updates
   *
   * This method will create a data packet with the supplied content.
   * The packet name is the local session + seqNo.
   * The seqNo is set by the application.
   *
   * @throws It will throw error, if the prefix does not exist in m_logic
   *
   * @param buf Pointer to the bytes in content
   * @param len size of the bytes in content
   * @param freshness FreshnessPeriod of the data packet.
   * @param seqNo Sequence number of the data
   * @param prefix The user prefix that will be used to publish the data.
   */
  void
  publishData(const uint8_t* buf, size_t len, const ndn::time::milliseconds& freshness,
              const uint64_t& seqNo, const Name& prefix = DEFAULT_PREFIX);

  /**
   * @brief Publish a data packet in the session and trigger synchronization updates
   *
   * This method will create a data packet with the supplied content.
   * The packet name is the local session + seqNo.
   * The seqNo is automatically maintained by internal Logic.
   *
   * @throws It will throw error, if the prefix does not exist in m_logic
   *
   * @param content Block that will be set as the content of the data packet.
   * @param freshness FreshnessPeriod of the data packet.
   * @param prefix The user prefix that will be used to publish the data.
   */
  void
  publishData(const Block& content, const ndn::time::milliseconds& freshness,
              const Name& prefix = DEFAULT_PREFIX);

  /**
   * @brief Publish a data packet in the session and trigger synchronization updates
   *
   * This method will create a data packet with the supplied content.
   * The packet name is the local session + seqNo.
   * The seqNo is set by the application.
   *
   * @throws It will throw error, if the prefix does not exist in m_logic
   *
   * @param content Block that will be set as the content of the data packet.
   * @param freshness FreshnessPeriod of the data packet.
   * @param seqNo Sequence number of the data
   * @param prefix The user prefix that will be used to publish the data.
   */
  void
  publishData(const Block& content, const ndn::time::milliseconds& freshness,
              const uint64_t& seqNo, const Name& prefix = DEFAULT_PREFIX);

  /**
   * @brief Retrive a data packet with a particular seqNo from a session
   *
   * @param sessionName The name of the target session.
   * @param seq The seqNo of the data packet.
   * @param onValidated The callback when the retrieved packet has been validated.
   * @param nRetries The number of retries.
   */
  void
  fetchData(const Name& sessionName, const SeqNo& seq,
            const DataValidatedCallback& onValidated,
            int nRetries = 0);

  /**
   * @brief Retrive a data packet with a particular seqNo from a session
   *
   * @param sessionName The name of the target session.
   * @param seq The seqNo of the data packet.
   * @param onValidated The callback when the retrieved packet has been validated.
   * @param onValidationFailed The callback when the retrieved packet failed validation.
   * @param onTimeout The callback when data is not retrieved.
   * @param nRetries The number of retries.
   */
  void
  fetchData(const Name& sessionName, const SeqNo& seq,
            const DataValidatedCallback& onValidated,
            const DataValidationErrorCallback& onValidationFailed,
            const ndn::TimeoutCallback& onTimeout,
            int nRetries = 0);

  /// @brief Get the root digest of current sync tree
  ConstBufferPtr
  getRootDigest() const;

  Logic&
  getLogic()
  {
    return m_logic;
  }

private:
  void
  onInterest(const Name& prefix, const Interest& interest);

  void
  onData(const Interest& interest, const Data& data,
         const DataValidatedCallback& dataCallback,
         const DataValidationErrorCallback& failCallback);

  void
  onDataTimeout(const Interest& interest, int nRetries,
                const DataValidatedCallback& dataCallback,
                const DataValidationErrorCallback& failCallback);

  void
  onDataValidationFailed(const Data& data,
                         const ValidationError& error);

public:
  static const ndn::Name DEFAULT_NAME;
  static const ndn::Name DEFAULT_PREFIX;
  static const std::shared_ptr<Validator> DEFAULT_VALIDATOR;

private:
  using RegisteredPrefixList = std::unordered_map<ndn::Name, ndn::RegisteredPrefixHandle>;

  Name m_userPrefix;
  ndn::Face& m_face;
  Logic m_logic;

  Name m_signingId;
  ndn::KeyChain m_keyChain;
  std::shared_ptr<Validator> m_validator;

  RegisteredPrefixList m_registeredPrefixList;
  ndn::InMemoryStoragePersistent m_ims;
};

} // namespace chronosync

#endif // CHRONOSYNC_SOCKET_HPP
