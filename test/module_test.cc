/* Copyright 2014 Peter Goodman, all rights reserved. */

#include <gmock/gmock.h>

#define GRANARY_INTERNAL

#include "granary/base/base.h"
#include "granary/base/cast.h"
#include "granary/base/string.h"

#include "granary/module.h"

#include "test/context.h"

using namespace granary;
using namespace ::testing;

class ModuleManagerTest : public Test {
 protected:
  ModuleManagerTest(void)
      : context(),
        m1(&context),
        m2(&context),
        mod(new Module(ModuleKind::KERNEL_MODULE, GRANARY_NAME_STRING)) {
    m2.RegisterAllBuiltIn();
  }

  MockContext context;
  ModuleManager m1;
  ModuleManager m2;
  Module *mod;
};

TEST_F(ModuleManagerTest, EmptyDoesNotFindLibC) {
  ASSERT_TRUE(nullptr == m1.FindByName("c"));
}

TEST_F(ModuleManagerTest, EmptyDoesNotHaveExit) {
  ASSERT_TRUE(nullptr == m1.FindByAppPC(UnsafeCast<AppPC>(exit)));
}

TEST_F(ModuleManagerTest, WithBuiltinFindsGranary) {
  ASSERT_TRUE(nullptr != m2.FindByName(GRANARY_NAME_STRING));
}

TEST_F(ModuleManagerTest, WithBuiltinFindsLibC) {
  ASSERT_TRUE(nullptr != m2.FindByName("c"));
}

TEST_F(ModuleManagerTest, WithBuiltinFindsPthreads) {
  ASSERT_TRUE(nullptr != m2.FindByName("pthread"));
}

TEST_F(ModuleManagerTest, WithBuiltinFindsLibDL) {
  ASSERT_TRUE(nullptr != m2.FindByName("dl"));
}

TEST_F(ModuleManagerTest, FindRegisteredModule) {
  m1.Register(mod);
  auto found_mod = m1.FindByName(GRANARY_NAME_STRING);
  ASSERT_TRUE(nullptr != found_mod);
  ASSERT_EQ(mod, found_mod);
}

TEST_F(ModuleManagerTest, FindRegisteredModulePC) {
  m1.Register(mod);
  mod->AddRange(100, 200, 0, 0);
  for (auto addr = 0UL; addr < 300; ++addr) {
    if (100 <= addr && 200 > addr) {
      EXPECT_EQ(mod, m1.FindByAppPC(UnsafeCast<AppPC>(addr)));
    } else {
      EXPECT_TRUE(nullptr == m1.FindByAppPC(UnsafeCast<AppPC>(addr)));
    }
  }
}

class ModuleTest : public Test {
 protected:
  ModuleTest(void)
      : mod(ModuleKind::KERNEL_MODULE, GRANARY_NAME_STRING) {}
  Module mod;
};

TEST_F(ModuleTest, DoesNotContainNullptr) {
  ASSERT_FALSE(mod.Contains(nullptr));
}

TEST_F(ModuleTest, ReturnsInvalidOffset) {
  auto offset = mod.OffsetOf(nullptr);
  EXPECT_TRUE(nullptr == offset.module);
  EXPECT_TRUE(0 == offset.offset);
}

TEST_F(ModuleTest, HasInitializedKind) {
  ASSERT_TRUE(ModuleKind::KERNEL_MODULE == mod.Kind());
}

TEST_F(ModuleTest, HasInitializedName) {
  ASSERT_TRUE(StringsMatch(GRANARY_NAME_STRING, mod.Name()));
}

class ModuleRangeTest : public Test {
 protected:
  ModuleRangeTest(void)
      : mod(ModuleKind::KERNEL_MODULE, GRANARY_NAME_STRING) {
    mod.AddRange(100, 200, 0, 0);
  }

  void TestPCMembership(void) {
    for (auto addr = 0UL; addr < 300UL; ++addr) {
      if (100 > addr || 200 <= addr) {
        EXPECT_FALSE(mod.Contains(UnsafeCast<AppPC>(addr)));
      } else {
        EXPECT_TRUE(mod.Contains(UnsafeCast<AppPC>(addr)));
      }
    }
  }

  void TestOffsetsInRange(void) {
    uintptr_t addr_offset(0);
    for (auto addr = 100UL; addr < 200UL; ++addr) {
      auto offset = mod.OffsetOf(UnsafeCast<AppPC>(addr));
      EXPECT_EQ(&mod, offset.module);
      EXPECT_EQ(addr_offset, offset.offset);
      ++addr_offset;
    }
  }

  Module mod;
};

TEST_F(ModuleRangeTest, PCsInAndOutOfRange) {
  TestPCMembership();
}

TEST_F(ModuleRangeTest, OffsetsOfPCsInRangeMatch) {
  TestOffsetsInRange();
}

// Incrementally split the range [100, 200) into many small ranges that cover
// the same larger range. The range is split from the left-hand side.
//      [100,101), [101,102), ..., [199,200)
// and test that PC membership within the ranges continue to work.
TEST_F(ModuleRangeTest, SplitRangeLHS) {
  uintptr_t addr_offset(0);
  for (auto addr = 100UL; addr < 200UL; ++addr) {
    mod.AddRange(addr, addr + 1, addr_offset++, 0);
    TestPCMembership();
  }
  TestPCMembership();
  TestOffsetsInRange();
}

// Incrementally split the range [100, 200) into many small ranges that cover
// the same larger range. The range is split from the right-hand side.
//      [100,101), [101,102), ..., [199,200)
// and test that PC membership within the ranges continue to work.
TEST_F(ModuleRangeTest, SplitRangeRHS) {
  uintptr_t addr_offset(100);
  for (auto addr = 200UL; addr > 100UL; --addr) {
    mod.AddRange(addr - 1, addr, --addr_offset, 0);
    TestPCMembership();
  }
  TestPCMembership();
  TestOffsetsInRange();
}

// Split the range into three sub-ranges:
//    [100,125),[125,175),[175,200)
TEST_F(ModuleRangeTest, SplitRangeMid) {
  mod.AddRange(125, 175, 25, 0);
  TestPCMembership();
  TestOffsetsInRange();
}

// Split the range into two sub-ranges by removing a middle range:
//    [100,125),[125,175),[175,200)
TEST_F(ModuleRangeTest, RemoveRangeMid) {
  mod.RemoveRange(125, 175);
  for (auto addr = 0UL; addr < 300UL; ++addr) {
    if ((100 <= addr && 125 > addr) ||
        (175 <= addr && 200 > addr)) {
      EXPECT_TRUE(mod.Contains(UnsafeCast<AppPC>(addr)));
    } else {
      EXPECT_FALSE(mod.Contains(UnsafeCast<AppPC>(addr)));
    }
  }
}

// Split two contiguous ranges [100,150) and [150,200) into three sub-ranges
// [100,125), [125,175), and [175,200).
TEST_F(ModuleRangeTest, SplitRangeCross) {
  mod.AddRange(100, 150, 0, 0);
  mod.AddRange(150, 200, 50, 0);
  TestPCMembership();
  TestOffsetsInRange();

  mod.AddRange(125, 175, 25, 0);
  TestPCMembership();
  TestOffsetsInRange();
}
