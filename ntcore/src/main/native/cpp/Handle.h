// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#ifndef NTCORE_HANDLE_H_
#define NTCORE_HANDLE_H_

#include <wpi/Synchronization.h>

#include "ntcore_c.h"

namespace nt {

// Handle data layout:
// Bits 30-24: Type
// Bits 23-20: Instance index
// Bits 19-0:  Handle index (0/unused for instance handles)

class Handle {
 public:
  enum Type {
    kConnectionListener = wpi::kHandleTypeNTBase,
    kConnectionListenerPoller,
    kEntry,
    kEntryListener,
    kEntryListenerPoller,
    kInstance,
    kLogger,
    kLoggerPoller,
    kRpcCall,
    kRpcCallPoller,
    kDataLogger,
    kConnectionDataLogger
  };
  enum { kIndexMax = 0xfffff };

  explicit Handle(NT_Handle handle) : m_handle(handle) {}
  operator NT_Handle() const { return m_handle; }

  NT_Handle handle() const { return m_handle; }

  Handle(int inst, int index, Type type) {
    if (inst < 0 || index < 0) {
      m_handle = 0;
      return;
    }
    m_handle = ((static_cast<int>(type) & 0x7f) << 24) | ((inst & 0xf) << 20) |
               (index & 0xfffff);
  }

  int GetIndex() const { return static_cast<int>(m_handle) & 0xfffff; }
  Type GetType() const {
    return static_cast<Type>((static_cast<int>(m_handle) >> 24) & 0x7f);
  }
  int GetInst() const { return (static_cast<int>(m_handle) >> 20) & 0xf; }
  bool IsType(Type type) const { return type == GetType(); }
  int GetTypedIndex(Type type) const { return IsType(type) ? GetIndex() : -1; }
  int GetTypedInst(Type type) const { return IsType(type) ? GetInst() : -1; }

 private:
  NT_Handle m_handle;
};

}  // namespace nt

#endif  // NTCORE_HANDLE_H_
