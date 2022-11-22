// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include "wpi/ManagedStatic.h"  // NOLINT(build/include_order)

#include "gtest/gtest.h"

static int refCount = 0;

struct StaticTestClass {
  StaticTestClass() { refCount++; }
  ~StaticTestClass() { refCount--; }

  void Func() {}
};

namespace wpi {
TEST(ManagedStaticTest, LazyDoesNotInitialize) {
  {
    refCount = 0;
    wpi::ManagedStatic<StaticTestClass> managedStatic;
    (void)managedStatic;
    ASSERT_EQ(refCount, 0);
  }
  ASSERT_EQ(refCount, 0);
  wpi_shutdown();
}

TEST(ManagedStaticTest, LazyInitDoesntDestruct) {
  {
    refCount = 0;
    wpi::ManagedStatic<StaticTestClass> managedStatic;
    ASSERT_EQ(refCount, 0);
    managedStatic->Func();
    ASSERT_EQ(refCount, 1);
  }
  ASSERT_EQ(refCount, 1);
  wpi_shutdown();
  ASSERT_EQ(refCount, 0);
}

TEST(ManagedStaticTest, EagerInit) {
  {
    refCount = 0;
    StaticTestClass* test = new StaticTestClass{};
    ASSERT_EQ(refCount, 1);  // NOLINT
    wpi::ManagedStatic<StaticTestClass> managedStatic(
        test, [](void* val) { delete static_cast<StaticTestClass*>(val); });
    ASSERT_EQ(refCount, 1);
    managedStatic->Func();
    ASSERT_EQ(refCount, 1);
  }
  ASSERT_EQ(refCount, 1);
  wpi_shutdown();
  ASSERT_EQ(refCount, 0);
}
}  // namespace wpi
