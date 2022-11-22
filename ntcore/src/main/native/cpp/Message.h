// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#ifndef NTCORE_MESSAGE_H_
#define NTCORE_MESSAGE_H_

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "networktables/NetworkTableValue.h"

namespace nt {

class WireDecoder;
class WireEncoder;

class Message {
  struct private_init {};

 public:
  enum MsgType {
    kUnknown = -1,
    kKeepAlive = 0x00,
    kClientHello = 0x01,
    kProtoUnsup = 0x02,
    kServerHelloDone = 0x03,
    kServerHello = 0x04,
    kClientHelloDone = 0x05,
    kEntryAssign = 0x10,
    kEntryUpdate = 0x11,
    kFlagsUpdate = 0x12,
    kEntryDelete = 0x13,
    kClearEntries = 0x14,
    kExecuteRpc = 0x20,
    kRpcResponse = 0x21
  };
  using GetEntryTypeFunc = std::function<NT_Type(unsigned int id)>;

  Message() = default;
  Message(MsgType type, const private_init&) : m_type(type) {}

  MsgType type() const { return m_type; }
  bool Is(MsgType type) const { return type == m_type; }

  // Message data accessors.  Callers are responsible for knowing what data is
  // actually provided for a particular message.
  std::string_view str() const { return m_str; }
  std::shared_ptr<Value> value() const { return m_value; }
  unsigned int id() const { return m_id; }
  unsigned int flags() const { return m_flags; }
  unsigned int seq_num_uid() const { return m_seq_num_uid; }

  // Read and write from wire representation
  void Write(WireEncoder& encoder) const;
  static std::shared_ptr<Message> Read(WireDecoder& decoder,
                                       GetEntryTypeFunc get_entry_type);

  // Create messages without data
  static std::shared_ptr<Message> KeepAlive() {
    return std::make_shared<Message>(kKeepAlive, private_init());
  }
  static std::shared_ptr<Message> ProtoUnsup() {
    return std::make_shared<Message>(kProtoUnsup, private_init());
  }
  static std::shared_ptr<Message> ServerHelloDone() {
    return std::make_shared<Message>(kServerHelloDone, private_init());
  }
  static std::shared_ptr<Message> ClientHelloDone() {
    return std::make_shared<Message>(kClientHelloDone, private_init());
  }
  static std::shared_ptr<Message> ClearEntries() {
    return std::make_shared<Message>(kClearEntries, private_init());
  }

  // Create messages with data
  static std::shared_ptr<Message> ClientHello(std::string_view self_id);
  static std::shared_ptr<Message> ServerHello(unsigned int flags,
                                              std::string_view self_id);
  static std::shared_ptr<Message> EntryAssign(std::string_view name,
                                              unsigned int id,
                                              unsigned int seq_num,
                                              std::shared_ptr<Value> value,
                                              unsigned int flags);
  static std::shared_ptr<Message> EntryUpdate(unsigned int id,
                                              unsigned int seq_num,
                                              std::shared_ptr<Value> value);
  static std::shared_ptr<Message> FlagsUpdate(unsigned int id,
                                              unsigned int flags);
  static std::shared_ptr<Message> EntryDelete(unsigned int id);
  static std::shared_ptr<Message> ExecuteRpc(unsigned int id, unsigned int uid,
                                             std::string_view params);
  static std::shared_ptr<Message> RpcResponse(unsigned int id, unsigned int uid,
                                              std::string_view result);

  Message(const Message&) = delete;
  Message& operator=(const Message&) = delete;

 private:
  MsgType m_type{kUnknown};

  // Message data.  Use varies by message type.
  std::string m_str;
  std::shared_ptr<Value> m_value;
  unsigned int m_id{0};  // also used for proto_rev
  unsigned int m_flags{0};
  unsigned int m_seq_num_uid{0};
};

}  // namespace nt

#endif  // NTCORE_MESSAGE_H_
