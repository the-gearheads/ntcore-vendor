// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#ifndef NTCORE_ISTORAGE_H_
#define NTCORE_ISTORAGE_H_

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include <wpi/span.h>

#include "Message.h"
#include "ntcore_cpp.h"

namespace nt {

class IDispatcher;
class INetworkConnection;

class IStorage {
 public:
  IStorage() = default;
  IStorage(const IStorage&) = delete;
  IStorage& operator=(const IStorage&) = delete;
  virtual ~IStorage() = default;

  // Accessors required by Dispatcher.  An interface is used for
  // generation of outgoing messages to break a dependency loop between
  // Storage and Dispatcher.
  virtual void SetDispatcher(IDispatcher* dispatcher, bool server) = 0;
  virtual void ClearDispatcher() = 0;

  // Required for wire protocol 2.0 to get the entry type of an entry when
  // receiving entry updates (because the length/type is not provided in the
  // message itself).  Not used in wire protocol 3.0.
  virtual NT_Type GetMessageEntryType(unsigned int id) const = 0;

  virtual void ProcessIncoming(std::shared_ptr<Message> msg,
                               INetworkConnection* conn,
                               std::weak_ptr<INetworkConnection> conn_weak) = 0;
  virtual void GetInitialAssignments(
      INetworkConnection& conn,
      std::vector<std::shared_ptr<Message>>* msgs) = 0;
  virtual void ApplyInitialAssignments(
      INetworkConnection& conn, wpi::span<std::shared_ptr<Message>> msgs,
      bool new_server, std::vector<std::shared_ptr<Message>>* out_msgs) = 0;

  // Filename-based save/load functions.  Used both by periodic saves and
  // accessible directly via the user API.
  virtual const char* SavePersistent(std::string_view filename,
                                     bool periodic) const = 0;
  virtual const char* LoadPersistent(
      std::string_view filename,
      std::function<void(size_t line, const char* msg)> warn) = 0;
};

}  // namespace nt

#endif  // NTCORE_ISTORAGE_H_
