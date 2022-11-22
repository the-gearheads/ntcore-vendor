// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "networktables/NetworkTableInstance.h"

#include <fmt/format.h>
#include <wpi/SmallVector.h>

using namespace nt;

std::shared_ptr<NetworkTable> NetworkTableInstance::GetTable(
    std::string_view key) const {
  if (key.empty() || key == "/") {
    return std::make_shared<NetworkTable>(m_handle, "",
                                          NetworkTable::private_init{});
  } else if (key.front() == NetworkTable::PATH_SEPARATOR_CHAR) {
    return std::make_shared<NetworkTable>(m_handle, key,
                                          NetworkTable::private_init{});
  } else {
    return std::make_shared<NetworkTable>(m_handle, fmt::format("/{}", key),
                                          NetworkTable::private_init{});
  }
}

void NetworkTableInstance::StartClient(
    wpi::span<const std::string_view> servers, unsigned int port) {
  wpi::SmallVector<std::pair<std::string_view, unsigned int>, 8> server_ports;
  for (const auto& server : servers) {
    server_ports.emplace_back(std::make_pair(server, port));
  }
  StartClient(server_ports);
}

void NetworkTableInstance::SetServer(wpi::span<const std::string_view> servers,
                                     unsigned int port) {
  wpi::SmallVector<std::pair<std::string_view, unsigned int>, 8> server_ports;
  for (const auto& server : servers) {
    server_ports.emplace_back(std::make_pair(server, port));
  }
  SetServer(server_ports);
}

NT_EntryListener NetworkTableInstance::AddEntryListener(
    std::string_view prefix,
    std::function<void(const EntryNotification& event)> callback,
    unsigned int flags) const {
  return ::nt::AddEntryListener(m_handle, prefix, callback, flags);
}

NT_ConnectionListener NetworkTableInstance::AddConnectionListener(
    std::function<void(const ConnectionNotification& event)> callback,
    bool immediate_notify) const {
  return ::nt::AddConnectionListener(m_handle, callback, immediate_notify);
}
