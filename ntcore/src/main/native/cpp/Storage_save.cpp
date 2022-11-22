// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include <cctype>
#include <string>

#include <fmt/format.h>
#include <wpi/Base64.h>
#include <wpi/SmallString.h>
#include <wpi/StringExtras.h>
#include <wpi/fs.h>
#include <wpi/raw_ostream.h>

#include "Log.h"
#include "Storage.h"

using namespace nt;

namespace {

class SavePersistentImpl {
 public:
  using Entry = std::pair<std::string, std::shared_ptr<Value>>;

  explicit SavePersistentImpl(wpi::raw_ostream& os) : m_os(os) {}

  void Save(wpi::span<const Entry> entries);

 private:
  void WriteString(std::string_view str);
  void WriteHeader();
  void WriteEntries(wpi::span<const Entry> entries);
  void WriteEntry(std::string_view name, const Value& value);
  bool WriteType(NT_Type type);
  void WriteValue(const Value& value);

  wpi::raw_ostream& m_os;
};

}  // namespace

/* Escapes and writes a string, including start and end double quotes */
void SavePersistentImpl::WriteString(std::string_view str) {
  m_os << '"';
  for (auto c : str) {
    switch (c) {
      case '\\':
        m_os << "\\\\";
        break;
      case '\t':
        m_os << "\\t";
        break;
      case '\n':
        m_os << "\\n";
        break;
      case '"':
        m_os << "\\\"";
        break;
      default:
        if (std::isprint(c) && c != '=') {
          m_os << c;
          break;
        }

        // Write out the escaped representation.
        m_os << "\\x";
        m_os << wpi::hexdigit((c >> 4) & 0xF);
        m_os << wpi::hexdigit((c >> 0) & 0xF);
    }
  }
  m_os << '"';
}

void SavePersistentImpl::Save(wpi::span<const Entry> entries) {
  WriteHeader();
  WriteEntries(entries);
}

void SavePersistentImpl::WriteHeader() {
  m_os << "[NetworkTables Storage 3.0]\n";
}

void SavePersistentImpl::WriteEntries(wpi::span<const Entry> entries) {
  for (auto& i : entries) {
    if (!i.second) {
      continue;
    }
    WriteEntry(i.first, *i.second);
  }
}

void SavePersistentImpl::WriteEntry(std::string_view name, const Value& value) {
  if (!WriteType(value.type())) {
    return;  // type
  }
  WriteString(name);  // name
  m_os << '=';        // '='
  WriteValue(value);  // value
  m_os << '\n';       // eol
}

bool SavePersistentImpl::WriteType(NT_Type type) {
  switch (type) {
    case NT_BOOLEAN:
      m_os << "boolean ";
      break;
    case NT_DOUBLE:
      m_os << "double ";
      break;
    case NT_STRING:
      m_os << "string ";
      break;
    case NT_RAW:
      m_os << "raw ";
      break;
    case NT_BOOLEAN_ARRAY:
      m_os << "array boolean ";
      break;
    case NT_DOUBLE_ARRAY:
      m_os << "array double ";
      break;
    case NT_STRING_ARRAY:
      m_os << "array string ";
      break;
    default:
      return false;
  }
  return true;
}

void SavePersistentImpl::WriteValue(const Value& value) {
  switch (value.type()) {
    case NT_BOOLEAN:
      m_os << (value.GetBoolean() ? "true" : "false");
      break;
    case NT_DOUBLE:
      m_os << fmt::format("{:g}", value.GetDouble());
      break;
    case NT_STRING:
      WriteString(value.GetString());
      break;
    case NT_RAW: {
      wpi::Base64Encode(m_os, value.GetRaw());
      break;
    }
    case NT_BOOLEAN_ARRAY: {
      bool first = true;
      for (auto elem : value.GetBooleanArray()) {
        if (!first) {
          m_os << ',';
        }
        first = false;
        m_os << (elem ? "true" : "false");
      }
      break;
    }
    case NT_DOUBLE_ARRAY: {
      bool first = true;
      for (auto elem : value.GetDoubleArray()) {
        if (!first) {
          m_os << ',';
        }
        first = false;
        m_os << fmt::format("{:g}", elem);
      }
      break;
    }
    case NT_STRING_ARRAY: {
      bool first = true;
      for (auto& elem : value.GetStringArray()) {
        if (!first) {
          m_os << ',';
        }
        first = false;
        WriteString(elem);
      }
      break;
    }
    default:
      break;
  }
}

void Storage::SavePersistent(wpi::raw_ostream& os, bool periodic) const {
  std::vector<SavePersistentImpl::Entry> entries;
  if (!GetPersistentEntries(periodic, &entries)) {
    return;
  }
  SavePersistentImpl(os).Save(entries);
}

const char* Storage::SavePersistent(std::string_view filename,
                                    bool periodic) const {
  std::string fn{filename};
  auto tmp = fmt::format("{}.tmp", filename);
  auto bak = fmt::format("{}.bak", filename);

  // Get entries before creating file
  std::vector<SavePersistentImpl::Entry> entries;
  if (!GetPersistentEntries(periodic, &entries)) {
    return nullptr;
  }

  const char* err = nullptr;

  // start by writing to temporary file
  std::error_code ec;
  wpi::raw_fd_ostream os(tmp, ec, fs::F_Text);
  if (ec.value() != 0) {
    err = "could not open file";
    goto done;
  }
  DEBUG0("saving persistent file '{}'", filename);
  SavePersistentImpl(os).Save(entries);
  os.close();
  if (os.has_error()) {
    std::remove(tmp.c_str());
    err = "error saving file";
    goto done;
  }

  // Safely move to real file.  We ignore any failures related to the backup.
  std::remove(bak.c_str());
  std::rename(fn.c_str(), bak.c_str());
  if (std::rename(tmp.c_str(), fn.c_str()) != 0) {
    std::rename(bak.c_str(), fn.c_str());  // attempt to restore backup
    err = "could not rename temp file to real file";
    goto done;
  }

done:
  // try again if there was an error
  if (err && periodic) {
    m_persistent_dirty = true;
  }
  return err;
}

void Storage::SaveEntries(wpi::raw_ostream& os, std::string_view prefix) const {
  std::vector<SavePersistentImpl::Entry> entries;
  if (!GetEntries(prefix, &entries)) {
    return;
  }
  SavePersistentImpl(os).Save(entries);
}

const char* Storage::SaveEntries(std::string_view filename,
                                 std::string_view prefix) const {
  std::string fn{filename};
  auto tmp = fmt::format("{}.tmp", filename);
  auto bak = fmt::format("{}.bak", filename);

  // Get entries before creating file
  std::vector<SavePersistentImpl::Entry> entries;
  if (!GetEntries(prefix, &entries)) {
    return nullptr;
  }

  // start by writing to temporary file
  std::error_code ec;
  wpi::raw_fd_ostream os(tmp, ec, fs::F_Text);
  if (ec.value() != 0) {
    return "could not open file";
  }
  DEBUG0("saving file '{}'", filename);
  SavePersistentImpl(os).Save(entries);
  os.close();
  if (os.has_error()) {
    std::remove(tmp.c_str());
    return "error saving file";
  }

  // Safely move to real file.  We ignore any failures related to the backup.
  std::remove(bak.c_str());
  std::rename(fn.c_str(), bak.c_str());
  if (std::rename(tmp.c_str(), fn.c_str()) != 0) {
    std::rename(bak.c_str(), fn.c_str());  // attempt to restore backup
    return "could not rename temp file to real file";
  }

  return nullptr;
}
