//===-- common_test.cpp -----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "internal_defs.h"
#include "tests/scudo_unit_test.h"

#include "common.h"
#include <algorithm>
#include <fstream>

namespace scudo {

static uptr getResidentMemorySize() {
  if (!SCUDO_LINUX)
    UNREACHABLE("Not implemented!");
  uptr Size;
  uptr Resident;
  std::ifstream IFS("/proc/self/statm");
  IFS >> Size;
  IFS >> Resident;
  return Resident * getPageSizeCached();
}

// Fuchsia needs getResidentMemorySize implementation.
TEST(ScudoCommonTest, SKIP_ON_FUCHSIA(ResidentMemorySize)) {
  uptr OnStart = getResidentMemorySize();
  EXPECT_GT(OnStart, 0UL);

  const uptr Size = 1ull << 30;
  const uptr Threshold = Size >> 3;

  MapPlatformData Data = {};
  uptr *P = reinterpret_cast<uptr *>(
      map(nullptr, Size, "ResidentMemorySize", 0, &Data));
  const size_t N = Size / sizeof(*P);
  ASSERT_NE(nullptr, P);
  EXPECT_EQ((size_t)std::count(P, P + N, 0), N);
  EXPECT_LT(getResidentMemorySize() - OnStart, Threshold);

  memset(P, 1, Size);
  EXPECT_EQ(std::count(P, P + N, 0), 0);
  EXPECT_LT(getResidentMemorySize() - Size, Threshold);

  releasePagesToOS((uptr)P, 0, Size, &Data);
  EXPECT_EQ((size_t)std::count(P, P + N, 0), N);
  // FIXME: does not work with QEMU-user.
  // EXPECT_LT(getResidentMemorySize() - OnStart, Threshold);

  memset(P, 1, Size);
  EXPECT_EQ(std::count(P, P + N, 0), 0);
  EXPECT_LT(getResidentMemorySize() - Size, Threshold);
}

} // namespace scudo