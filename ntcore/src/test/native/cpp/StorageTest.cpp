// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "StorageTest.h"

#include <wpi/SmallString.h>
#include <wpi/StringExtras.h>
#include <wpi/raw_istream.h>
#include <wpi/raw_ostream.h>

#include "MessageMatcher.h"
#include "MockNetworkConnection.h"
#include "Storage.h"
#include "TestPrinters.h"
#include "ValueMatcher.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::IsNull;
using ::testing::Return;

namespace nt {

class StorageEmptyTest : public StorageTest,
                         public ::testing::TestWithParam<bool> {
 public:
  StorageEmptyTest() {
    HookOutgoing(GetParam());
    EXPECT_CALL(notifier, local_notifiers())
        .Times(AnyNumber())
        .WillRepeatedly(Return(true));
  }
};

class StoragePopulateOneTest : public StorageEmptyTest {
 public:
  StoragePopulateOneTest() {
    EXPECT_CALL(dispatcher, QueueOutgoing(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(notifier, NotifyEntry(_, _, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(notifier, local_notifiers())
        .Times(AnyNumber())
        .WillRepeatedly(Return(false));
    storage.SetEntryTypeValue("foo", Value::MakeBoolean(true));
    ::testing::Mock::VerifyAndClearExpectations(&dispatcher);
    ::testing::Mock::VerifyAndClearExpectations(&notifier);
    EXPECT_CALL(notifier, local_notifiers())
        .Times(AnyNumber())
        .WillRepeatedly(Return(true));
  }
};

class StoragePopulatedTest : public StorageEmptyTest {
 public:
  StoragePopulatedTest() {
    EXPECT_CALL(dispatcher, QueueOutgoing(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(notifier, NotifyEntry(_, _, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(notifier, local_notifiers())
        .Times(AnyNumber())
        .WillRepeatedly(Return(false));
    storage.SetEntryTypeValue("foo", Value::MakeBoolean(true));
    storage.SetEntryTypeValue("foo2", Value::MakeDouble(0.0));
    storage.SetEntryTypeValue("bar", Value::MakeDouble(1.0));
    storage.SetEntryTypeValue("bar2", Value::MakeBoolean(false));
    ::testing::Mock::VerifyAndClearExpectations(&dispatcher);
    ::testing::Mock::VerifyAndClearExpectations(&notifier);
    EXPECT_CALL(notifier, local_notifiers())
        .Times(AnyNumber())
        .WillRepeatedly(Return(true));
  }
};

class StoragePersistentTest : public StorageEmptyTest {
 public:
  StoragePersistentTest() {
    EXPECT_CALL(dispatcher, QueueOutgoing(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(notifier, NotifyEntry(_, _, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(notifier, local_notifiers())
        .Times(AnyNumber())
        .WillRepeatedly(Return(false));
    storage.SetEntryTypeValue("boolean/true", Value::MakeBoolean(true));
    storage.SetEntryTypeValue("boolean/false", Value::MakeBoolean(false));
    storage.SetEntryTypeValue("double/neg", Value::MakeDouble(-1.5));
    storage.SetEntryTypeValue("double/zero", Value::MakeDouble(0.0));
    storage.SetEntryTypeValue("double/big", Value::MakeDouble(1.3e8));
    storage.SetEntryTypeValue("string/empty", Value::MakeString(""));
    storage.SetEntryTypeValue("string/normal", Value::MakeString("hello"));
    storage.SetEntryTypeValue(
        "string/special", Value::MakeString(std::string_view("\0\3\5\n", 4)));
    storage.SetEntryTypeValue("string/quoted", Value::MakeString("\"a\""));
    storage.SetEntryTypeValue("raw/empty", Value::MakeRaw(""));
    storage.SetEntryTypeValue("raw/normal", Value::MakeRaw("hello"));
    storage.SetEntryTypeValue("raw/special",
                              Value::MakeRaw(std::string_view("\0\3\5\n", 4)));
    storage.SetEntryTypeValue("booleanarr/empty",
                              Value::MakeBooleanArray(std::vector<int>{}));
    storage.SetEntryTypeValue("booleanarr/one",
                              Value::MakeBooleanArray(std::vector<int>{1}));
    storage.SetEntryTypeValue("booleanarr/two",
                              Value::MakeBooleanArray(std::vector<int>{1, 0}));
    storage.SetEntryTypeValue("doublearr/empty",
                              Value::MakeDoubleArray(std::vector<double>{}));
    storage.SetEntryTypeValue("doublearr/one",
                              Value::MakeDoubleArray(std::vector<double>{0.5}));
    storage.SetEntryTypeValue(
        "doublearr/two",
        Value::MakeDoubleArray(std::vector<double>{0.5, -0.25}));
    storage.SetEntryTypeValue(
        "stringarr/empty", Value::MakeStringArray(std::vector<std::string>{}));
    storage.SetEntryTypeValue(
        "stringarr/one",
        Value::MakeStringArray(std::vector<std::string>{"hello"}));
    storage.SetEntryTypeValue(
        "stringarr/two",
        Value::MakeStringArray(std::vector<std::string>{"hello", "world\n"}));
    storage.SetEntryTypeValue(std::string_view("\0\3\5\n", 4),
                              Value::MakeBoolean(true));
    storage.SetEntryTypeValue("=", Value::MakeBoolean(true));
    ::testing::Mock::VerifyAndClearExpectations(&dispatcher);
    ::testing::Mock::VerifyAndClearExpectations(&notifier);
    EXPECT_CALL(notifier, local_notifiers())
        .Times(AnyNumber())
        .WillRepeatedly(Return(true));
  }
};

class MockLoadWarn {
 public:
  MOCK_METHOD2(Warn, void(size_t line, std::string_view msg));
};

TEST_P(StorageEmptyTest, Construct) {
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, StorageEntryInit) {
  auto entry = GetEntry("foo");
  EXPECT_FALSE(entry->value);
  EXPECT_EQ(0u, entry->flags);
  EXPECT_EQ("foobar", entry->name);  // since GetEntry uses the tmp_entry.
  EXPECT_EQ(0xffffu, entry->id);
  EXPECT_EQ(SequenceNumber(), entry->seq_num);
}

TEST_P(StorageEmptyTest, GetEntryValueNotExist) {
  EXPECT_FALSE(storage.GetEntryValue("foo"));
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, GetEntryValueExist) {
  auto value = Value::MakeBoolean(true);
  EXPECT_CALL(dispatcher, QueueOutgoing(_, IsNull(), IsNull()));
  EXPECT_CALL(notifier, NotifyEntry(_, _, _, _, _));
  storage.SetEntryTypeValue("foo", value);
  EXPECT_EQ(value, storage.GetEntryValue("foo"));
}

TEST_P(StorageEmptyTest, SetEntryTypeValueAssignNew) {
  // brand new entry
  auto value = Value::MakeBoolean(true);
  // id assigned if server
  EXPECT_CALL(dispatcher,
              QueueOutgoing(MessageEq(Message::EntryAssign(
                                "foo", GetParam() ? 0 : 0xffff, 1, value, 0)),
                            IsNull(), IsNull()));
  EXPECT_CALL(notifier, NotifyEntry(0, std::string_view("foo"), value,
                                    NT_NOTIFY_NEW | NT_NOTIFY_LOCAL, UINT_MAX));

  storage.SetEntryTypeValue("foo", value);
  EXPECT_EQ(value, GetEntry("foo")->value);
  if (GetParam()) {
    ASSERT_EQ(1u, idmap().size());
    EXPECT_EQ(value, idmap()[0]->value);
  } else {
    EXPECT_TRUE(idmap().empty());
  }
}

TEST_P(StoragePopulateOneTest, SetEntryTypeValueAssignTypeChange) {
  // update with different type results in assignment message
  auto value = Value::MakeDouble(0.0);

  // id assigned if server; seq_num incremented
  EXPECT_CALL(dispatcher,
              QueueOutgoing(MessageEq(Message::EntryAssign(
                                "foo", GetParam() ? 0 : 0xffff, 2, value, 0)),
                            IsNull(), IsNull()));
  EXPECT_CALL(notifier,
              NotifyEntry(0, std::string_view("foo"), value,
                          NT_NOTIFY_UPDATE | NT_NOTIFY_LOCAL, UINT_MAX));

  storage.SetEntryTypeValue("foo", value);
  EXPECT_EQ(value, GetEntry("foo")->value);
}

TEST_P(StoragePopulateOneTest, SetEntryTypeValueEqualValue) {
  // update with same type and same value: change value contents but no update
  // message is issued (minimizing bandwidth usage)
  auto value = Value::MakeBoolean(true);
  storage.SetEntryTypeValue("foo", value);
  EXPECT_EQ(value, GetEntry("foo")->value);
}

TEST_P(StoragePopulatedTest, SetEntryTypeValueDifferentValue) {
  // update with same type and different value results in value update message
  auto value = Value::MakeDouble(1.0);

  // client shouldn't send an update as id not assigned yet
  if (GetParam()) {
    // id assigned if server; seq_num incremented
    EXPECT_CALL(dispatcher,
                QueueOutgoing(MessageEq(Message::EntryUpdate(1, 2, value)),
                              IsNull(), IsNull()));
  }
  EXPECT_CALL(notifier,
              NotifyEntry(1, std::string_view("foo2"), value,
                          NT_NOTIFY_UPDATE | NT_NOTIFY_LOCAL, UINT_MAX));
  storage.SetEntryTypeValue("foo2", value);
  EXPECT_EQ(value, GetEntry("foo2")->value);

  if (!GetParam()) {
    // seq_num should still be incremented
    EXPECT_EQ(2u, GetEntry("foo2")->seq_num.value());
  }
}

TEST_P(StorageEmptyTest, SetEntryTypeValueEmptyName) {
  auto value = Value::MakeBoolean(true);
  storage.SetEntryTypeValue("", value);
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, SetEntryTypeValueEmptyValue) {
  storage.SetEntryTypeValue("foo", nullptr);
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, SetEntryValueAssignNew) {
  // brand new entry
  auto value = Value::MakeBoolean(true);

  // id assigned if server
  EXPECT_CALL(dispatcher,
              QueueOutgoing(MessageEq(Message::EntryAssign(
                                "foo", GetParam() ? 0 : 0xffff, 1, value, 0)),
                            IsNull(), IsNull()));
  EXPECT_CALL(notifier, NotifyEntry(0, std::string_view("foo"), value,
                                    NT_NOTIFY_NEW | NT_NOTIFY_LOCAL, UINT_MAX));

  EXPECT_TRUE(storage.SetEntryValue("foo", value));
  EXPECT_EQ(value, GetEntry("foo")->value);
}

TEST_P(StoragePopulateOneTest, SetEntryValueAssignTypeChange) {
  // update with different type results in error and no message or notification
  auto value = Value::MakeDouble(0.0);
  EXPECT_FALSE(storage.SetEntryValue("foo", value));
  auto entry = GetEntry("foo");
  EXPECT_NE(value, entry->value);
}

TEST_P(StoragePopulateOneTest, SetEntryValueEqualValue) {
  // update with same type and same value: change value contents but no update
  // message is issued (minimizing bandwidth usage)
  auto value = Value::MakeBoolean(true);
  EXPECT_TRUE(storage.SetEntryValue("foo", value));
  auto entry = GetEntry("foo");
  EXPECT_EQ(value, entry->value);
}

TEST_P(StoragePopulatedTest, SetEntryValueDifferentValue) {
  // update with same type and different value results in value update message
  auto value = Value::MakeDouble(1.0);

  // client shouldn't send an update as id not assigned yet
  if (GetParam()) {
    // id assigned if server; seq_num incremented
    EXPECT_CALL(dispatcher,
                QueueOutgoing(MessageEq(Message::EntryUpdate(1, 2, value)),
                              IsNull(), IsNull()));
  }
  EXPECT_CALL(notifier,
              NotifyEntry(1, std::string_view("foo2"), value,
                          NT_NOTIFY_UPDATE | NT_NOTIFY_LOCAL, UINT_MAX));

  EXPECT_TRUE(storage.SetEntryValue("foo2", value));
  auto entry = GetEntry("foo2");
  EXPECT_EQ(value, entry->value);

  if (!GetParam()) {
    // seq_num should still be incremented
    EXPECT_EQ(2u, GetEntry("foo2")->seq_num.value());
  }
}

TEST_P(StorageEmptyTest, SetEntryValueEmptyName) {
  auto value = Value::MakeBoolean(true);
  EXPECT_TRUE(storage.SetEntryValue("", value));
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, SetEntryValueEmptyValue) {
  EXPECT_TRUE(storage.SetEntryValue("foo", nullptr));
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, SetDefaultEntryAssignNew) {
  // brand new entry
  auto value = Value::MakeBoolean(true);

  // id assigned if server
  EXPECT_CALL(dispatcher,
              QueueOutgoing(MessageEq(Message::EntryAssign(
                                "foo", GetParam() ? 0 : 0xffff, 1, value, 0)),
                            IsNull(), IsNull()));
  EXPECT_CALL(notifier, NotifyEntry(0, std::string_view("foo"), value,
                                    NT_NOTIFY_NEW | NT_NOTIFY_LOCAL, UINT_MAX));

  auto ret_val = storage.SetDefaultEntryValue("foo", value);
  EXPECT_TRUE(ret_val);
  EXPECT_EQ(value, GetEntry("foo")->value);
}

TEST_P(StoragePopulateOneTest, SetDefaultEntryExistsSameType) {
  // existing entry
  auto value = Value::MakeBoolean(true);
  auto ret_val = storage.SetDefaultEntryValue("foo", value);
  EXPECT_TRUE(ret_val);
  EXPECT_NE(value, GetEntry("foo")->value);
}

TEST_P(StoragePopulateOneTest, SetDefaultEntryExistsDifferentType) {
  // existing entry is boolean
  auto value = Value::MakeDouble(2.0);
  auto ret_val = storage.SetDefaultEntryValue("foo", value);
  EXPECT_FALSE(ret_val);
  // should not have updated value in table if it already existed.
  EXPECT_NE(value, GetEntry("foo")->value);
}

TEST_P(StorageEmptyTest, SetDefaultEntryEmptyName) {
  auto value = Value::MakeBoolean(true);
  auto ret_val = storage.SetDefaultEntryValue("", value);
  EXPECT_FALSE(ret_val);
  auto entry = GetEntry("foo");
  EXPECT_FALSE(entry->value);
  EXPECT_EQ(0u, entry->flags);
  EXPECT_EQ("foobar", entry->name);  // since GetEntry uses the tmp_entry.
  EXPECT_EQ(0xffffu, entry->id);
  EXPECT_EQ(SequenceNumber(), entry->seq_num);
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, SetDefaultEntryEmptyValue) {
  auto value = Value::MakeBoolean(true);
  auto ret_val = storage.SetDefaultEntryValue("", nullptr);
  EXPECT_FALSE(ret_val);
  auto entry = GetEntry("foo");
  EXPECT_FALSE(entry->value);
  EXPECT_EQ(0u, entry->flags);
  EXPECT_EQ("foobar", entry->name);  // since GetEntry uses the tmp_entry.
  EXPECT_EQ(0xffffu, entry->id);
  EXPECT_EQ(SequenceNumber(), entry->seq_num);
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StoragePopulatedTest, SetDefaultEntryEmptyName) {
  auto value = Value::MakeBoolean(true);
  auto ret_val = storage.SetDefaultEntryValue("", value);
  EXPECT_FALSE(ret_val);
  // assert that no entries get added
  EXPECT_EQ(4u, entries().size());
  if (GetParam()) {
    EXPECT_EQ(4u, idmap().size());
  } else {
    EXPECT_EQ(0u, idmap().size());
  }
}

TEST_P(StoragePopulatedTest, SetDefaultEntryEmptyValue) {
  auto value = Value::MakeBoolean(true);
  auto ret_val = storage.SetDefaultEntryValue("", nullptr);
  EXPECT_FALSE(ret_val);
  // assert that no entries get added
  EXPECT_EQ(4u, entries().size());
  if (GetParam()) {
    EXPECT_EQ(4u, idmap().size());
  } else {
    EXPECT_EQ(0u, idmap().size());
  }
}

TEST_P(StorageEmptyTest, SetEntryFlagsNew) {
  // flags setting doesn't create an entry
  storage.SetEntryFlags("foo", 0u);
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StoragePopulateOneTest, SetEntryFlagsEqualValue) {
  // update with same value: no update message is issued (minimizing bandwidth
  // usage)
  storage.SetEntryFlags("foo", 0u);
  auto entry = GetEntry("foo");
  EXPECT_EQ(0u, entry->flags);
}

TEST_P(StoragePopulatedTest, SetEntryFlagsDifferentValue) {
  // update with different value results in flags update message
  // client shouldn't send an update as id not assigned yet
  if (GetParam()) {
    // id assigned as this is the server
    EXPECT_CALL(dispatcher, QueueOutgoing(MessageEq(Message::FlagsUpdate(1, 1)),
                                          IsNull(), IsNull()));
  }
  EXPECT_CALL(notifier,
              NotifyEntry(1, std::string_view("foo2"), _,
                          NT_NOTIFY_FLAGS | NT_NOTIFY_LOCAL, UINT_MAX));
  storage.SetEntryFlags("foo2", 1u);
  EXPECT_EQ(1u, GetEntry("foo2")->flags);
}

TEST_P(StorageEmptyTest, SetEntryFlagsEmptyName) {
  storage.SetEntryFlags("", 0u);
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, GetEntryFlagsNotExist) {
  EXPECT_EQ(0u, storage.GetEntryFlags("foo"));
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StoragePopulateOneTest, GetEntryFlagsExist) {
  EXPECT_CALL(dispatcher, QueueOutgoing(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(notifier, NotifyEntry(_, _, _, _, _));
  storage.SetEntryFlags("foo", 1u);
  ::testing::Mock::VerifyAndClearExpectations(&dispatcher);
  EXPECT_EQ(1u, storage.GetEntryFlags("foo"));
}

TEST_P(StorageEmptyTest, DeleteEntryNotExist) {
  storage.DeleteEntry("foo");
}

TEST_P(StoragePopulatedTest, DeleteEntryExist) {
  // client shouldn't send an update as id not assigned yet
  if (GetParam()) {
    // id assigned as this is the server
    EXPECT_CALL(dispatcher, QueueOutgoing(MessageEq(Message::EntryDelete(1)),
                                          IsNull(), IsNull()));
  }
  EXPECT_CALL(
      notifier,
      NotifyEntry(1, std::string_view("foo2"), ValueEq(Value::MakeDouble(0)),
                  NT_NOTIFY_DELETE | NT_NOTIFY_LOCAL, UINT_MAX));

  storage.DeleteEntry("foo2");
  ASSERT_EQ(1u, entries().count("foo2"));
  EXPECT_EQ(nullptr, entries()["foo2"]->value);
  EXPECT_EQ(0xffffu, entries()["foo2"]->id);
  EXPECT_FALSE(entries()["foo2"]->local_write);
  if (GetParam()) {
    ASSERT_TRUE(idmap().size() >= 2);
    EXPECT_FALSE(idmap()[1]);
  }
}

TEST_P(StorageEmptyTest, DeleteAllEntriesEmpty) {
  storage.DeleteAllEntries();
  ASSERT_TRUE(entries().empty());
}

TEST_P(StoragePopulatedTest, DeleteAllEntries) {
  EXPECT_CALL(dispatcher, QueueOutgoing(MessageEq(Message::ClearEntries()),
                                        IsNull(), IsNull()));
  EXPECT_CALL(notifier, NotifyEntry(_, _, _, NT_NOTIFY_DELETE | NT_NOTIFY_LOCAL,
                                    UINT_MAX))
      .Times(4);

  storage.DeleteAllEntries();
  ASSERT_EQ(1u, entries().count("foo2"));
  EXPECT_EQ(nullptr, entries()["foo2"]->value);
}

TEST_P(StoragePopulatedTest, DeleteAllEntriesPersistent) {
  GetEntry("foo2")->flags = NT_PERSISTENT;

  EXPECT_CALL(dispatcher, QueueOutgoing(MessageEq(Message::ClearEntries()),
                                        IsNull(), IsNull()));
  EXPECT_CALL(notifier, NotifyEntry(_, _, _, NT_NOTIFY_DELETE | NT_NOTIFY_LOCAL,
                                    UINT_MAX))
      .Times(3);

  storage.DeleteAllEntries();
  ASSERT_EQ(1u, entries().count("foo2"));
  EXPECT_NE(nullptr, entries()["foo2"]->value);
}

TEST_P(StoragePopulatedTest, GetEntryInfoAll) {
  auto info = storage.GetEntryInfo(0, "", 0u);
  ASSERT_EQ(4u, info.size());
}

TEST_P(StoragePopulatedTest, GetEntryInfoPrefix) {
  auto info = storage.GetEntryInfo(0, "foo", 0u);
  ASSERT_EQ(2u, info.size());
  if (info[0].name == "foo") {
    EXPECT_EQ("foo", info[0].name);
    EXPECT_EQ(NT_BOOLEAN, info[0].type);
    EXPECT_EQ("foo2", info[1].name);
    EXPECT_EQ(NT_DOUBLE, info[1].type);
  } else {
    EXPECT_EQ("foo2", info[0].name);
    EXPECT_EQ(NT_DOUBLE, info[0].type);
    EXPECT_EQ("foo", info[1].name);
    EXPECT_EQ(NT_BOOLEAN, info[1].type);
  }
}

TEST_P(StoragePopulatedTest, GetEntryInfoTypes) {
  auto info = storage.GetEntryInfo(0, "", NT_DOUBLE);
  ASSERT_EQ(2u, info.size());
  EXPECT_EQ(NT_DOUBLE, info[0].type);
  EXPECT_EQ(NT_DOUBLE, info[1].type);
  if (info[0].name == "foo2") {
    EXPECT_EQ("foo2", info[0].name);
    EXPECT_EQ("bar", info[1].name);
  } else {
    EXPECT_EQ("bar", info[0].name);
    EXPECT_EQ("foo2", info[1].name);
  }
}

TEST_P(StoragePopulatedTest, GetEntryInfoPrefixTypes) {
  auto info = storage.GetEntryInfo(0, "bar", NT_BOOLEAN);
  ASSERT_EQ(1u, info.size());
  EXPECT_EQ("bar2", info[0].name);
  EXPECT_EQ(NT_BOOLEAN, info[0].type);
}

TEST_P(StoragePersistentTest, SavePersistentEmpty) {
  wpi::SmallString<256> buf;
  wpi::raw_svector_ostream oss(buf);
  storage.SavePersistent(oss, false);
  ASSERT_EQ("[NetworkTables Storage 3.0]\n", oss.str());
}

TEST_P(StoragePersistentTest, SavePersistent) {
  for (auto& i : entries()) {
    i.getValue()->flags = NT_PERSISTENT;
  }
  wpi::SmallString<256> buf;
  wpi::raw_svector_ostream oss(buf);
  storage.SavePersistent(oss, false);
  std::string_view out = oss.str();
  // std::fputs(out.c_str(), stderr);
  std::string_view line, rem = out;
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("[NetworkTables Storage 3.0]", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("boolean \"\\x00\\x03\\x05\\n\"=true", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("boolean \"\\x3D\"=true", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("boolean \"boolean/false\"=false", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("boolean \"boolean/true\"=true", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("array boolean \"booleanarr/empty\"=", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("array boolean \"booleanarr/one\"=true", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("array boolean \"booleanarr/two\"=true,false", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("double \"double/big\"=1.3e+08", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("double \"double/neg\"=-1.5", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("double \"double/zero\"=0", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("array double \"doublearr/empty\"=", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("array double \"doublearr/one\"=0.5", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("array double \"doublearr/two\"=0.5,-0.25", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("raw \"raw/empty\"=", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("raw \"raw/normal\"=aGVsbG8=", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("raw \"raw/special\"=AAMFCg==", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("string \"string/empty\"=\"\"", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("string \"string/normal\"=\"hello\"", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("string \"string/quoted\"=\"\\\"a\\\"\"", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("string \"string/special\"=\"\\x00\\x03\\x05\\n\"", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("array string \"stringarr/empty\"=", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("array string \"stringarr/one\"=\"hello\"", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("array string \"stringarr/two\"=\"hello\",\"world\\n\"", line);
  std::tie(line, rem) = wpi::split(rem, '\n');
  ASSERT_EQ("", line);
}

TEST_P(StorageEmptyTest, LoadPersistentBadHeader) {
  MockLoadWarn warn;
  auto warn_func = [&](size_t line, const char* msg) { warn.Warn(line, msg); };

  wpi::raw_mem_istream iss("");
  EXPECT_CALL(
      warn,
      Warn(1, std::string_view("header line mismatch, ignoring rest of file")));
  EXPECT_FALSE(storage.LoadEntries(iss, "", true, warn_func));

  wpi::raw_mem_istream iss2("[NetworkTables");
  EXPECT_CALL(
      warn,
      Warn(1, std::string_view("header line mismatch, ignoring rest of file")));

  EXPECT_FALSE(storage.LoadEntries(iss2, "", true, warn_func));
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, LoadPersistentCommentHeader) {
  MockLoadWarn warn;
  auto warn_func = [&](size_t line, const char* msg) { warn.Warn(line, msg); };

  wpi::raw_mem_istream iss(
      "\n; comment\n# comment\n[NetworkTables Storage 3.0]\n");
  EXPECT_TRUE(storage.LoadEntries(iss, "", true, warn_func));
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, LoadPersistentEmptyName) {
  MockLoadWarn warn;
  auto warn_func = [&](size_t line, const char* msg) { warn.Warn(line, msg); };

  wpi::raw_mem_istream iss("[NetworkTables Storage 3.0]\nboolean \"\"=true\n");
  EXPECT_TRUE(storage.LoadEntries(iss, "", true, warn_func));
  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, LoadPersistentAssign) {
  MockLoadWarn warn;
  auto warn_func = [&](size_t line, const char* msg) { warn.Warn(line, msg); };

  auto value = Value::MakeBoolean(true);

  // id assigned if server
  EXPECT_CALL(dispatcher, QueueOutgoing(MessageEq(Message::EntryAssign(
                                            "foo", GetParam() ? 0 : 0xffff, 1,
                                            value, NT_PERSISTENT)),
                                        IsNull(), IsNull()));
  EXPECT_CALL(notifier, NotifyEntry(0, std::string_view("foo"),
                                    ValueEq(Value::MakeBoolean(true)),
                                    NT_NOTIFY_NEW | NT_NOTIFY_LOCAL, UINT_MAX));

  wpi::raw_mem_istream iss(
      "[NetworkTables Storage 3.0]\nboolean \"foo\"=true\n");
  EXPECT_TRUE(storage.LoadEntries(iss, "", true, warn_func));
  auto entry = GetEntry("foo");
  EXPECT_EQ(*value, *entry->value);
  EXPECT_EQ(NT_PERSISTENT, entry->flags);
}

TEST_P(StoragePopulatedTest, LoadPersistentUpdateFlags) {
  MockLoadWarn warn;
  auto warn_func = [&](size_t line, const char* msg) { warn.Warn(line, msg); };

  // client shouldn't send an update as id not assigned yet
  if (GetParam()) {
    // id assigned as this is server
    EXPECT_CALL(dispatcher,
                QueueOutgoing(MessageEq(Message::FlagsUpdate(1, NT_PERSISTENT)),
                              IsNull(), IsNull()));
  }
  EXPECT_CALL(
      notifier,
      NotifyEntry(1, std::string_view("foo2"), ValueEq(Value::MakeDouble(0)),
                  NT_NOTIFY_FLAGS | NT_NOTIFY_LOCAL, UINT_MAX));

  wpi::raw_mem_istream iss(
      "[NetworkTables Storage 3.0]\ndouble \"foo2\"=0.0\n");
  EXPECT_TRUE(storage.LoadEntries(iss, "", true, warn_func));
  auto entry = GetEntry("foo2");
  EXPECT_EQ(*Value::MakeDouble(0.0), *entry->value);
  EXPECT_EQ(NT_PERSISTENT, entry->flags);
}

TEST_P(StoragePopulatedTest, LoadPersistentUpdateValue) {
  MockLoadWarn warn;
  auto warn_func = [&](size_t line, const char* msg) { warn.Warn(line, msg); };

  GetEntry("foo2")->flags = NT_PERSISTENT;

  auto value = Value::MakeDouble(1.0);

  // client shouldn't send an update as id not assigned yet
  if (GetParam()) {
    // id assigned as this is the server; seq_num incremented
    EXPECT_CALL(dispatcher,
                QueueOutgoing(MessageEq(Message::EntryUpdate(1, 2, value)),
                              IsNull(), IsNull()));
  }
  EXPECT_CALL(
      notifier,
      NotifyEntry(1, std::string_view("foo2"), ValueEq(Value::MakeDouble(1)),
                  NT_NOTIFY_UPDATE | NT_NOTIFY_LOCAL, UINT_MAX));

  wpi::raw_mem_istream iss(
      "[NetworkTables Storage 3.0]\ndouble \"foo2\"=1.0\n");
  EXPECT_TRUE(storage.LoadEntries(iss, "", true, warn_func));
  auto entry = GetEntry("foo2");
  EXPECT_EQ(*value, *entry->value);
  EXPECT_EQ(NT_PERSISTENT, entry->flags);

  if (!GetParam()) {
    // seq_num should still be incremented
    EXPECT_EQ(2u, GetEntry("foo2")->seq_num.value());
  }
}

TEST_P(StoragePopulatedTest, LoadPersistentUpdateValueFlags) {
  MockLoadWarn warn;
  auto warn_func = [&](size_t line, const char* msg) { warn.Warn(line, msg); };

  auto value = Value::MakeDouble(1.0);

  // client shouldn't send an update as id not assigned yet
  if (GetParam()) {
    // id assigned as this is the server; seq_num incremented
    EXPECT_CALL(dispatcher,
                QueueOutgoing(MessageEq(Message::EntryUpdate(1, 2, value)),
                              IsNull(), IsNull()));
    EXPECT_CALL(dispatcher,
                QueueOutgoing(MessageEq(Message::FlagsUpdate(1, NT_PERSISTENT)),
                              IsNull(), IsNull()));
  }
  EXPECT_CALL(
      notifier,
      NotifyEntry(1, std::string_view("foo2"), ValueEq(Value::MakeDouble(1)),
                  NT_NOTIFY_FLAGS | NT_NOTIFY_UPDATE | NT_NOTIFY_LOCAL,
                  UINT_MAX));

  wpi::raw_mem_istream iss(
      "[NetworkTables Storage 3.0]\ndouble \"foo2\"=1.0\n");
  EXPECT_TRUE(storage.LoadEntries(iss, "", true, warn_func));
  auto entry = GetEntry("foo2");
  EXPECT_EQ(*value, *entry->value);
  EXPECT_EQ(NT_PERSISTENT, entry->flags);

  if (!GetParam()) {
    // seq_num should still be incremented
    EXPECT_EQ(2u, GetEntry("foo2")->seq_num.value());
  }
}

TEST_P(StorageEmptyTest, LoadPersistent) {
  MockLoadWarn warn;
  auto warn_func = [&](size_t line, const char* msg) { warn.Warn(line, msg); };

  std::string in = "[NetworkTables Storage 3.0]\n";
  in += "boolean \"\\x00\\x03\\x05\\n\"=true\n";
  in += "boolean \"\\x3D\"=true\n";
  in += "boolean \"boolean/false\"=false\n";
  in += "boolean \"boolean/true\"=true\n";
  in += "array boolean \"booleanarr/empty\"=\n";
  in += "array boolean \"booleanarr/one\"=true\n";
  in += "array boolean \"booleanarr/two\"=true,false\n";
  in += "double \"double/big\"=1.3e+08\n";
  in += "double \"double/neg\"=-1.5\n";
  in += "double \"double/zero\"=0\n";
  in += "array double \"doublearr/empty\"=\n";
  in += "array double \"doublearr/one\"=0.5\n";
  in += "array double \"doublearr/two\"=0.5,-0.25\n";
  in += "raw \"raw/empty\"=\n";
  in += "raw \"raw/normal\"=aGVsbG8=\n";
  in += "raw \"raw/special\"=AAMFCg==\n";
  in += "string \"string/empty\"=\"\"\n";
  in += "string \"string/normal\"=\"hello\"\n";
  in += "string \"string/special\"=\"\\x00\\x03\\x05\\n\"\n";
  in += "string \"string/quoted\"=\"\\\"a\\\"\"\n";
  in += "array string \"stringarr/empty\"=\n";
  in += "array string \"stringarr/one\"=\"hello\"\n";
  in += "array string \"stringarr/two\"=\"hello\",\"world\\n\"\n";

  EXPECT_CALL(dispatcher, QueueOutgoing(_, _, _)).Times(23);
  EXPECT_CALL(notifier,
              NotifyEntry(_, _, _, NT_NOTIFY_NEW | NT_NOTIFY_LOCAL, UINT_MAX))
      .Times(23);

  wpi::raw_mem_istream iss(in);
  EXPECT_TRUE(storage.LoadEntries(iss, "", true, warn_func));
  ASSERT_EQ(23u, entries().size());

  EXPECT_EQ(*Value::MakeBoolean(true), *storage.GetEntryValue("boolean/true"));
  EXPECT_EQ(*Value::MakeBoolean(false),
            *storage.GetEntryValue("boolean/false"));
  EXPECT_EQ(*Value::MakeDouble(-1.5), *storage.GetEntryValue("double/neg"));
  EXPECT_EQ(*Value::MakeDouble(0.0), *storage.GetEntryValue("double/zero"));
  EXPECT_EQ(*Value::MakeDouble(1.3e8), *storage.GetEntryValue("double/big"));
  EXPECT_EQ(*Value::MakeString(""), *storage.GetEntryValue("string/empty"));
  EXPECT_EQ(*Value::MakeString("hello"),
            *storage.GetEntryValue("string/normal"));
  EXPECT_EQ(*Value::MakeString(std::string_view("\0\3\5\n", 4)),
            *storage.GetEntryValue("string/special"));
  EXPECT_EQ(*Value::MakeString("\"a\""),
            *storage.GetEntryValue("string/quoted"));
  EXPECT_EQ(*Value::MakeRaw(""), *storage.GetEntryValue("raw/empty"));
  EXPECT_EQ(*Value::MakeRaw("hello"), *storage.GetEntryValue("raw/normal"));
  EXPECT_EQ(*Value::MakeRaw(std::string_view("\0\3\5\n", 4)),
            *storage.GetEntryValue("raw/special"));
  EXPECT_EQ(*Value::MakeBooleanArray(std::vector<int>{}),
            *storage.GetEntryValue("booleanarr/empty"));
  EXPECT_EQ(*Value::MakeBooleanArray(std::vector<int>{1}),
            *storage.GetEntryValue("booleanarr/one"));
  EXPECT_EQ(*Value::MakeBooleanArray(std::vector<int>{1, 0}),
            *storage.GetEntryValue("booleanarr/two"));
  EXPECT_EQ(*Value::MakeDoubleArray(std::vector<double>{}),
            *storage.GetEntryValue("doublearr/empty"));
  EXPECT_EQ(*Value::MakeDoubleArray(std::vector<double>{0.5}),
            *storage.GetEntryValue("doublearr/one"));
  EXPECT_EQ(*Value::MakeDoubleArray(std::vector<double>{0.5, -0.25}),
            *storage.GetEntryValue("doublearr/two"));
  EXPECT_EQ(*Value::MakeStringArray(std::vector<std::string>{}),
            *storage.GetEntryValue("stringarr/empty"));
  EXPECT_EQ(*Value::MakeStringArray(std::vector<std::string>{"hello"}),
            *storage.GetEntryValue("stringarr/one"));
  EXPECT_EQ(
      *Value::MakeStringArray(std::vector<std::string>{"hello", "world\n"}),
      *storage.GetEntryValue("stringarr/two"));
  EXPECT_EQ(*Value::MakeBoolean(true),
            *storage.GetEntryValue(std::string_view("\0\3\5\n", 4)));
  EXPECT_EQ(*Value::MakeBoolean(true), *storage.GetEntryValue("="));
}

TEST_P(StorageEmptyTest, LoadPersistentWarn) {
  MockLoadWarn warn;
  auto warn_func = [&](size_t line, const char* msg) { warn.Warn(line, msg); };

  wpi::raw_mem_istream iss(
      "[NetworkTables Storage 3.0]\nboolean \"foo\"=foo\n");
  EXPECT_CALL(
      warn, Warn(2, std::string_view(
                        "unrecognized boolean value, not 'true' or 'false'")));
  EXPECT_TRUE(storage.LoadEntries(iss, "", true, warn_func));

  EXPECT_TRUE(entries().empty());
  EXPECT_TRUE(idmap().empty());
}

TEST_P(StorageEmptyTest, ProcessIncomingEntryAssign) {
  auto conn = std::make_shared<MockNetworkConnection>();
  auto value = Value::MakeDouble(1.0);
  if (GetParam()) {
    // id assign message reply generated on the server; sent to everyone
    EXPECT_CALL(
        dispatcher,
        QueueOutgoing(MessageEq(Message::EntryAssign("foo", 0, 0, value, 0)),
                      IsNull(), IsNull()));
  }
  EXPECT_CALL(notifier, NotifyEntry(0, std::string_view("foo"), ValueEq(value),
                                    NT_NOTIFY_NEW, UINT_MAX));

  storage.ProcessIncoming(
      Message::EntryAssign("foo", GetParam() ? 0xffff : 0, 0, value, 0),
      conn.get(), conn);
}

TEST_P(StoragePopulateOneTest, ProcessIncomingEntryAssign) {
  auto conn = std::make_shared<MockNetworkConnection>();
  auto value = Value::MakeDouble(1.0);
  EXPECT_CALL(*conn, proto_rev()).WillRepeatedly(Return(0x0300u));
  if (GetParam()) {
    // server broadcasts new value to all *other* connections
    EXPECT_CALL(
        dispatcher,
        QueueOutgoing(MessageEq(Message::EntryAssign("foo", 0, 1, value, 0)),
                      IsNull(), conn.get()));
  }
  EXPECT_CALL(notifier, NotifyEntry(0, std::string_view("foo"), ValueEq(value),
                                    NT_NOTIFY_UPDATE, UINT_MAX));

  storage.ProcessIncoming(Message::EntryAssign("foo", 0, 1, value, 0),
                          conn.get(), conn);
}

TEST_P(StoragePopulateOneTest, ProcessIncomingEntryAssignIgnore) {
  auto conn = std::make_shared<MockNetworkConnection>();
  auto value = Value::MakeDouble(1.0);
  storage.ProcessIncoming(Message::EntryAssign("foo", 0xffff, 1, value, 0),
                          conn.get(), conn);
}

TEST_P(StoragePopulateOneTest, ProcessIncomingEntryAssignWithFlags) {
  auto conn = std::make_shared<MockNetworkConnection>();
  auto value = Value::MakeDouble(1.0);
  EXPECT_CALL(*conn, proto_rev()).WillRepeatedly(Return(0x0300u));
  if (GetParam()) {
    // server broadcasts new value/flags to all *other* connections
    EXPECT_CALL(
        dispatcher,
        QueueOutgoing(MessageEq(Message::EntryAssign("foo", 0, 1, value, 0x2)),
                      IsNull(), conn.get()));
    EXPECT_CALL(notifier,
                NotifyEntry(0, std::string_view("foo"), ValueEq(value),
                            NT_NOTIFY_UPDATE | NT_NOTIFY_FLAGS, UINT_MAX));
  } else {
    // client forces flags back when an assign message is received for an
    // existing entry with different flags
    EXPECT_CALL(dispatcher, QueueOutgoing(MessageEq(Message::FlagsUpdate(0, 0)),
                                          IsNull(), IsNull()));
    EXPECT_CALL(notifier,
                NotifyEntry(0, std::string_view("foo"), ValueEq(value),
                            NT_NOTIFY_UPDATE, UINT_MAX));
  }

  storage.ProcessIncoming(Message::EntryAssign("foo", 0, 1, value, 0x2),
                          conn.get(), conn);
}

TEST_P(StoragePopulateOneTest, DeleteCheckHandle) {
  EXPECT_CALL(dispatcher, QueueOutgoing(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(notifier, NotifyEntry(_, _, _, _, _)).Times(AnyNumber());
  auto handle = storage.GetEntry("foo");
  storage.DeleteEntry("foo");
  storage.SetEntryTypeValue("foo", Value::MakeBoolean(true));
  ::testing::Mock::VerifyAndClearExpectations(&dispatcher);
  ::testing::Mock::VerifyAndClearExpectations(&notifier);

  auto handle2 = storage.GetEntry("foo");
  ASSERT_EQ(handle, handle2);
}

TEST_P(StoragePopulateOneTest, DeletedEntryFlags) {
  EXPECT_CALL(dispatcher, QueueOutgoing(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(notifier, NotifyEntry(_, _, _, _, _)).Times(AnyNumber());
  auto handle = storage.GetEntry("foo");
  storage.SetEntryFlags("foo", 2);
  storage.DeleteEntry("foo");
  ::testing::Mock::VerifyAndClearExpectations(&dispatcher);
  ::testing::Mock::VerifyAndClearExpectations(&notifier);

  EXPECT_EQ(storage.GetEntryFlags("foo"), 0u);
  EXPECT_EQ(storage.GetEntryFlags(handle), 0u);
  storage.SetEntryFlags("foo", 4);
  storage.SetEntryFlags(handle, 4);
  EXPECT_EQ(storage.GetEntryFlags("foo"), 0u);
  EXPECT_EQ(storage.GetEntryFlags(handle), 0u);
}

TEST_P(StoragePopulateOneTest, DeletedDeleteAllEntries) {
  EXPECT_CALL(dispatcher, QueueOutgoing(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(notifier, NotifyEntry(_, _, _, _, _)).Times(AnyNumber());
  storage.DeleteEntry("foo");
  ::testing::Mock::VerifyAndClearExpectations(&dispatcher);
  ::testing::Mock::VerifyAndClearExpectations(&notifier);

  EXPECT_CALL(dispatcher, QueueOutgoing(MessageEq(Message::ClearEntries()),
                                        IsNull(), IsNull()));
  storage.DeleteAllEntries();
}

TEST_P(StoragePopulateOneTest, DeletedGetEntries) {
  EXPECT_CALL(dispatcher, QueueOutgoing(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(notifier, NotifyEntry(_, _, _, _, _)).Times(AnyNumber());
  storage.DeleteEntry("foo");
  ::testing::Mock::VerifyAndClearExpectations(&dispatcher);
  ::testing::Mock::VerifyAndClearExpectations(&notifier);

  EXPECT_TRUE(storage.GetEntries("", 0).empty());
}

INSTANTIATE_TEST_SUITE_P(StorageEmptyTests, StorageEmptyTest,
                         ::testing::Bool());
INSTANTIATE_TEST_SUITE_P(StoragePopulateOneTests, StoragePopulateOneTest,
                         ::testing::Bool());
INSTANTIATE_TEST_SUITE_P(StoragePopulatedTests, StoragePopulatedTest,
                         ::testing::Bool());
INSTANTIATE_TEST_SUITE_P(StoragePersistentTests, StoragePersistentTest,
                         ::testing::Bool());

}  // namespace nt
