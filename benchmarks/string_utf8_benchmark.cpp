/**************************************************************************/
/*  string_utf8_benchmark.cpp                                             */
/**************************************************************************/
/*  zcore                                                                 */
/*  Copyright (c) 2026 Logan Yagadics                                     */
/*  Copyright (c) 2026 zcore Contributors                                 */
/*                                                                        */
/*  This file is part of the zcore project.                               */
/*  Use of this source code is governed by the license defined            */
/*  in the LICENSE file at the root of this repository.                   */
/**************************************************************************/
/**
 * @file benchmarks/string_utf8_benchmark.cpp
 * @brief Microbenchmark for UTF-8 code-point vs byte-oriented string operations.
 */

#include <zcore/fixed_string.hpp>
#include <zcore/string_view.hpp>

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>

namespace {

using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

template <typename FnT>
Nanoseconds RunLoop(zcore::usize iterations, FnT&& fn) {
  volatile zcore::usize sink = 0U;
  const auto begin = Clock::now();
  for (zcore::usize index = 0U; index < iterations; ++index) {
    sink += fn();
  }
  const auto end = Clock::now();
  if (sink == 0xFFFFFFFFU) {
    std::cerr << "sink: " << sink << '\n';
  }
  return std::chrono::duration_cast<Nanoseconds>(end - begin);
}

void PrintResult(const char* name, Nanoseconds elapsed, zcore::usize iterations) {
  const double nsPerOp = static_cast<double>(elapsed.count()) / static_cast<double>(iterations);
  std::cout << std::left << std::setw(34) << name << " : " << std::fixed << std::setprecision(2) << nsPerOp
            << " ns/op\n";
}

}  // namespace

int main() noexcept {  // NOLINT(bugprone-exception-escape)
  constexpr zcore::usize kIterations = 1'000'000U;
  constexpr char kSample[] = {
      'A',
      static_cast<char>(0xC2),  // ¢
      static_cast<char>(0xA2),
      static_cast<char>(0xE2),  // €
      static_cast<char>(0x82),
      static_cast<char>(0xAC),
      static_cast<char>(0xF0),  // 😀
      static_cast<char>(0x9F),
      static_cast<char>(0x98),
      static_cast<char>(0x80),
      'B',
      'C',
      'D',
      'E',
      'F',
      'G',
      'H',
      'I',
      'J',
      static_cast<char>(0x00),
  };

  const zcore::StringView base = zcore::StringView::FromCString(kSample);

  const Nanoseconds substrCodePoint = RunLoop(kIterations, [&base]() -> zcore::usize {
    const auto sub = base.Substr(1U, 3U);
    return sub.HasValue() ? sub.Value().Size() : 0U;
  });

  const Nanoseconds substrBytes = RunLoop(kIterations, [&base]() -> zcore::usize {
    const auto sub = base.SubstrBytes(1U, 3U);
    return sub.HasValue() ? sub.Value().Size() : 0U;
  });

  const Nanoseconds mutateCodePoint = RunLoop(kIterations, [&base]() -> zcore::usize {
    zcore::StringView mutableView = base;
    mutableView.RemovePrefix(2U);
    mutableView.RemoveSuffix(1U);
    return mutableView.Size();
  });

  const Nanoseconds mutateBytes = RunLoop(kIterations, [&base]() -> zcore::usize {
    zcore::StringView mutableView = base;
    mutableView.RemovePrefixBytes(2U);
    mutableView.RemoveSuffixBytes(1U);
    return mutableView.Size();
  });

  zcore::FixedString<64> fixed = zcore::FixedString<64>::FromCString(kSample);
  const Nanoseconds fixedAppendCodePoint = RunLoop(kIterations, [&fixed]() mutable -> zcore::usize {
    zcore::FixedString<64> local = fixed;
    const bool appended = local.TryAppendCodePoint(0x20ACU);
    if (!appended) {
      return 0U;
    }
    return local.Size();
  });

  std::cout << "zcore UTF-8 operation microbenchmark\n";
  std::cout << "Iterations: " << kIterations << "\n\n";
  PrintResult("StringView::Substr (code point)", substrCodePoint, kIterations);
  PrintResult("StringView::SubstrBytes", substrBytes, kIterations);
  PrintResult("StringView remove (code point)", mutateCodePoint, kIterations);
  PrintResult("StringView remove bytes", mutateBytes, kIterations);
  PrintResult("FixedString::TryAppendCodePoint", fixedAppendCodePoint, kIterations);

  return 0;
}
