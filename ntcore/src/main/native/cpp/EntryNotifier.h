// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#ifndef NTCORE_ENTRYNOTIFIER_H_
#define NTCORE_ENTRYNOTIFIER_H_

#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <wpi/CallbackManager.h>

#include "Handle.h"
#include "IEntryNotifier.h"
#include "ntcore_cpp.h"

namespace wpi {
class Logger;
}  // namespace wpi

namespace nt {

namespace impl {

struct EntryListenerData
    : public wpi::CallbackListenerData<
          std::function<void(const EntryNotification& event)>> {
  EntryListenerData() = default;
  EntryListenerData(
      std::function<void(const EntryNotification& event)> callback_,
      std::string_view prefix_, unsigned int flags_)
      : CallbackListenerData(callback_), prefix(prefix_), flags(flags_) {}
  EntryListenerData(
      std::function<void(const EntryNotification& event)> callback_,
      NT_Entry entry_, unsigned int flags_)
      : CallbackListenerData(callback_), entry(entry_), flags(flags_) {}
  EntryListenerData(unsigned int poller_uid_, std::string_view prefix_,
                    unsigned int flags_)
      : CallbackListenerData(poller_uid_), prefix(prefix_), flags(flags_) {}
  EntryListenerData(unsigned int poller_uid_, NT_Entry entry_,
                    unsigned int flags_)
      : CallbackListenerData(poller_uid_), entry(entry_), flags(flags_) {}

  std::string prefix;
  NT_Entry entry = 0;
  unsigned int flags;
};

class EntryNotifierThread
    : public wpi::CallbackThread<EntryNotifierThread, EntryNotification,
                                 EntryListenerData> {
 public:
  EntryNotifierThread(std::function<void()> on_start,
                      std::function<void()> on_exit, int inst)
      : CallbackThread(std::move(on_start), std::move(on_exit)), m_inst(inst) {}

  bool Matches(const EntryListenerData& listener,
               const EntryNotification& data);

  void SetListener(EntryNotification* data, unsigned int listener_uid) {
    data->listener =
        Handle(m_inst, listener_uid, Handle::kEntryListener).handle();
  }

  void DoCallback(std::function<void(const EntryNotification& event)> callback,
                  const EntryNotification& data) {
    callback(data);
  }

  int m_inst;
};

}  // namespace impl

class EntryNotifier
    : public IEntryNotifier,
      public wpi::CallbackManager<EntryNotifier, impl::EntryNotifierThread> {
  friend class EntryNotifierTest;
  friend class wpi::CallbackManager<EntryNotifier, impl::EntryNotifierThread>;

 public:
  explicit EntryNotifier(int inst, wpi::Logger& logger);

  void Start();

  bool local_notifiers() const override;

  unsigned int Add(std::function<void(const EntryNotification& event)> callback,
                   std::string_view prefix, unsigned int flags) override;
  unsigned int Add(std::function<void(const EntryNotification& event)> callback,
                   unsigned int local_id, unsigned int flags) override;
  unsigned int AddPolled(unsigned int poller_uid, std::string_view prefix,
                         unsigned int flags) override;
  unsigned int AddPolled(unsigned int poller_uid, unsigned int local_id,
                         unsigned int flags) override;

  void NotifyEntry(unsigned int local_id, std::string_view name,
                   std::shared_ptr<Value> value, unsigned int flags,
                   unsigned int only_listener = UINT_MAX) override;

 private:
  int m_inst;
  wpi::Logger& m_logger;
  std::atomic_bool m_local_notifiers;
};

}  // namespace nt

#endif  // NTCORE_ENTRYNOTIFIER_H_
