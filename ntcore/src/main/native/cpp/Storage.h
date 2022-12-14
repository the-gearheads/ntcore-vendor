// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#ifndef NTCORE_STORAGE_H_
#define NTCORE_STORAGE_H_

#include <stdint.h>

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <wpi/DenseMap.h>
#include <wpi/SmallSet.h>
#include <wpi/SmallVector.h>
#include <wpi/StringMap.h>
#include <wpi/UidVector.h>
#include <wpi/condition_variable.h>
#include <wpi/mutex.h>
#include <wpi/span.h>

#include "IStorage.h"
#include "Message.h"
#include "SequenceNumber.h"
#include "ntcore_cpp.h"

namespace wpi {
class Logger;
class raw_istream;
class raw_ostream;
}  // namespace wpi

namespace nt {

class IEntryNotifier;
class INetworkConnection;
class IRpcServer;
class IStorageTest;

class Storage : public IStorage {
  friend class StorageTest;

 public:
  Storage(IEntryNotifier& notifier, IRpcServer& rpcserver, wpi::Logger& logger);
  Storage(const Storage&) = delete;
  Storage& operator=(const Storage&) = delete;

  ~Storage() override;

  // Accessors required by Dispatcher.  An interface is used for
  // generation of outgoing messages to break a dependency loop between
  // Storage and Dispatcher.
  void SetDispatcher(IDispatcher* dispatcher, bool server) override;
  void ClearDispatcher() override;

  // Required for wire protocol 2.0 to get the entry type of an entry when
  // receiving entry updates (because the length/type is not provided in the
  // message itself).  Not used in wire protocol 3.0.
  NT_Type GetMessageEntryType(unsigned int id) const override;

  void ProcessIncoming(std::shared_ptr<Message> msg, INetworkConnection* conn,
                       std::weak_ptr<INetworkConnection> conn_weak) override;
  void GetInitialAssignments(
      INetworkConnection& conn,
      std::vector<std::shared_ptr<Message>>* msgs) override;
  void ApplyInitialAssignments(
      INetworkConnection& conn, wpi::span<std::shared_ptr<Message>> msgs,
      bool new_server,
      std::vector<std::shared_ptr<Message>>* out_msgs) override;

  // User functions.  These are the actual implementations of the corresponding
  // user API functions in ntcore_cpp.
  std::shared_ptr<Value> GetEntryValue(std::string_view name) const;
  std::shared_ptr<Value> GetEntryValue(unsigned int local_id) const;

  bool SetDefaultEntryValue(std::string_view name,
                            std::shared_ptr<Value> value);
  bool SetDefaultEntryValue(unsigned int local_id,
                            std::shared_ptr<Value> value);

  bool SetEntryValue(std::string_view name, std::shared_ptr<Value> value);
  bool SetEntryValue(unsigned int local_id, std::shared_ptr<Value> value);

  void SetEntryTypeValue(std::string_view name, std::shared_ptr<Value> value);
  void SetEntryTypeValue(unsigned int local_id, std::shared_ptr<Value> value);

  void SetEntryFlags(std::string_view name, unsigned int flags);
  void SetEntryFlags(unsigned int local_id, unsigned int flags);

  unsigned int GetEntryFlags(std::string_view name) const;
  unsigned int GetEntryFlags(unsigned int local_id) const;

  void DeleteEntry(std::string_view name);
  void DeleteEntry(unsigned int local_id);

  void DeleteAllEntries();

  std::vector<EntryInfo> GetEntryInfo(int inst, std::string_view prefix,
                                      unsigned int types);

  unsigned int AddListener(
      std::string_view prefix,
      std::function<void(const EntryNotification& event)> callback,
      unsigned int flags) const;
  unsigned int AddListener(
      unsigned int local_id,
      std::function<void(const EntryNotification& event)> callback,
      unsigned int flags) const;

  unsigned int AddPolledListener(unsigned int poller_uid,
                                 std::string_view prefix,
                                 unsigned int flags) const;
  unsigned int AddPolledListener(unsigned int poller_uid, unsigned int local_id,
                                 unsigned int flags) const;

  unsigned int StartDataLog(wpi::log::DataLog& log, std::string_view prefix,
                            std::string_view log_prefix);
  void StopDataLog(unsigned int uid);

  // Index-only
  unsigned int GetEntry(std::string_view name);
  std::vector<unsigned int> GetEntries(std::string_view prefix,
                                       unsigned int types);
  EntryInfo GetEntryInfo(int inst, unsigned int local_id) const;
  std::string GetEntryName(unsigned int local_id) const;
  NT_Type GetEntryType(unsigned int local_id) const;
  uint64_t GetEntryLastChange(unsigned int local_id) const;

  // Filename-based save/load functions.  Used both by periodic saves and
  // accessible directly via the user API.
  const char* SavePersistent(std::string_view filename,
                             bool periodic) const override;
  const char* LoadPersistent(
      std::string_view filename,
      std::function<void(size_t line, const char* msg)> warn) override;

  const char* SaveEntries(std::string_view filename,
                          std::string_view prefix) const;
  const char* LoadEntries(
      std::string_view filename, std::string_view prefix,
      std::function<void(size_t line, const char* msg)> warn);

  // Stream-based save/load functions (exposed for testing purposes).  These
  // implement the guts of the filename-based functions.
  void SavePersistent(wpi::raw_ostream& os, bool periodic) const;
  bool LoadEntries(wpi::raw_istream& is, std::string_view prefix,
                   bool persistent,
                   std::function<void(size_t line, const char* msg)> warn);

  void SaveEntries(wpi::raw_ostream& os, std::string_view prefix) const;

  // RPC configuration needs to come through here as RPC definitions are
  // actually special Storage value types.
  void CreateRpc(unsigned int local_id, std::string_view def,
                 unsigned int rpc_uid);
  unsigned int CallRpc(unsigned int local_id, std::string_view params);
  bool GetRpcResult(unsigned int local_id, unsigned int call_uid,
                    std::string* result);
  bool GetRpcResult(unsigned int local_id, unsigned int call_uid,
                    std::string* result, double timeout, bool* timed_out);
  void CancelRpcResult(unsigned int local_id, unsigned int call_uid);

 private:
  struct DataLoggerEntry {
    DataLoggerEntry(wpi::log::DataLog* log, int entry, unsigned int logger_uid)
        : log{log}, entry{entry}, logger_uid{logger_uid} {}

    wpi::log::DataLog* log;
    int entry;
    unsigned int logger_uid;
  };

  struct DataLogger {
    DataLogger() = default;
    DataLogger(wpi::log::DataLog& log, std::string_view prefix,
               std::string_view log_prefix)
        : log{&log}, prefix{prefix}, log_prefix{log_prefix} {}

    explicit operator bool() const { return log != nullptr; }

    wpi::log::DataLog* log = nullptr;
    std::string prefix;
    std::string log_prefix;
    unsigned int uid;
  };

  // Data for each table entry.
  struct Entry {
    explicit Entry(std::string_view name_) : name(name_) {}
    bool IsPersistent() const { return (flags & NT_PERSISTENT) != 0; }

    // We redundantly store the name so that it's available when accessing the
    // raw Entry* via the ID map.
    std::string name;

    // The current value and flags.
    std::shared_ptr<Value> value;
    unsigned int flags{0};

    // Unique ID for this entry as used in network messages.  The value is
    // assigned by the server, so on the client this is 0xffff until an
    // entry assignment is received back from the server.
    unsigned int id{0xffff};

    // Local ID.
    unsigned int local_id{UINT_MAX};

    // Sequence number for update resolution.
    SequenceNumber seq_num;

    // If value has been written locally.  Used during initial handshake
    // on client to determine whether or not to accept remote changes.
    bool local_write{false};

    // RPC handle.
    unsigned int rpc_uid{UINT_MAX};

    // Last UID used when calling this RPC (primarily for client use).  This
    // is incremented for each call.
    unsigned int rpc_call_uid{0};

    // log entries
    wpi::SmallVector<DataLoggerEntry, 0> datalogs;
    NT_Type datalog_type{NT_UNASSIGNED};
  };

  using EntriesMap = wpi::StringMap<Entry*>;
  using IdMap = std::vector<Entry*>;
  using LocalMap = std::vector<std::unique_ptr<Entry>>;
  using RpcIdPair = std::pair<unsigned int, unsigned int>;
  using RpcResultMap = wpi::DenseMap<RpcIdPair, std::string>;
  using RpcBlockingCallSet = wpi::SmallSet<RpcIdPair, 12>;

  mutable wpi::mutex m_mutex;
  EntriesMap m_entries;
  IdMap m_idmap;
  LocalMap m_localmap;
  RpcResultMap m_rpc_results;
  RpcBlockingCallSet m_rpc_blocking_calls;
  wpi::UidVector<DataLogger, 4> m_dataloggers;
  // If any persistent values have changed
  mutable bool m_persistent_dirty = false;

  // condition variable and termination flag for blocking on a RPC result
  std::atomic_bool m_terminating;
  wpi::condition_variable m_rpc_results_cond;

  // configured by dispatcher at startup
  IDispatcher* m_dispatcher = nullptr;
  bool m_server = true;

  IEntryNotifier& m_notifier;
  IRpcServer& m_rpc_server;
  wpi::Logger& m_logger;

  void ProcessIncomingEntryAssign(std::shared_ptr<Message> msg,
                                  INetworkConnection* conn);
  void ProcessIncomingEntryUpdate(std::shared_ptr<Message> msg,
                                  INetworkConnection* conn);
  void ProcessIncomingFlagsUpdate(std::shared_ptr<Message> msg,
                                  INetworkConnection* conn);
  void ProcessIncomingEntryDelete(std::shared_ptr<Message> msg,
                                  INetworkConnection* conn);
  void ProcessIncomingClearEntries(std::shared_ptr<Message> msg,
                                   INetworkConnection* conn);
  void ProcessIncomingExecuteRpc(std::shared_ptr<Message> msg,
                                 INetworkConnection* conn,
                                 std::weak_ptr<INetworkConnection> conn_weak);
  void ProcessIncomingRpcResponse(std::shared_ptr<Message> msg,
                                  INetworkConnection* conn);

  bool GetPersistentEntries(
      bool periodic,
      std::vector<std::pair<std::string, std::shared_ptr<Value>>>* entries)
      const;
  bool GetEntries(std::string_view prefix,
                  std::vector<std::pair<std::string, std::shared_ptr<Value>>>*
                      entries) const;
  void SetEntryValueImpl(Entry* entry, std::shared_ptr<Value> value,
                         std::unique_lock<wpi::mutex>& lock, bool local);
  void SetEntryFlagsImpl(Entry* entry, unsigned int flags,
                         std::unique_lock<wpi::mutex>& lock, bool local);
  void DeleteEntryImpl(Entry* entry, std::unique_lock<wpi::mutex>& lock,
                       bool local);

  void Notify(Entry* entry, unsigned int flags, bool local,
              std::shared_ptr<Value> value = {});

  // Must be called with m_mutex held
  template <typename F>
  void DeleteAllEntriesImpl(bool local, F should_delete);
  void DeleteAllEntriesImpl(bool local);
  Entry* GetOrNew(std::string_view name);
};

}  // namespace nt

#endif  // NTCORE_STORAGE_H_
