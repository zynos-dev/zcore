/**************************************************************************/
/*  error/contract_violation.hpp                                          */
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
 * @file include/zcore/error/contract_violation.hpp
 * @brief Centralized fatal contract-violation handling for zcore invariants.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/contract_violation.hpp>
 * ZCORE_CONTRACT_REQUIRE(ptr != nullptr,
 *                        zcore::detail::ContractViolationCode::NON_NULL_NULL_POINTER,
 *                        "pointer must not be null");
 * @endcode
 */

#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <zcore/foundation.hpp>

namespace zcore::detail {

/**
 * @brief Categorized contract violation identifiers.
 */
enum class ContractViolationCode : u8 {
    /// @brief Generic precondition violation.
    PRECONDITION = 0,
    /// @brief `Option::Value()` called while empty.
    OPTION_EMPTY_VALUE_ACCESS,
    /// @brief `Result::Value()` called on an error branch.
    RESULT_EXPECTED_VALUE,
    /// @brief `Result::Error()` called on a success branch.
    RESULT_EXPECTED_ERROR,
    /// @brief `NonNull` construction attempted with null pointer.
    NON_NULL_NULL_POINTER,
    /// @brief `Slice`/`SliceMut` received invalid pointer+size range.
    SLICE_INVALID_RANGE,
    /// @brief `Slice`/`SliceMut` index out of bounds.
    SLICE_INDEX_OUT_OF_BOUNDS,
    /// @brief `FixedVector` growth attempted at full capacity.
    FIXED_VECTOR_CAPACITY_EXCEEDED,
    /// @brief `FixedVector` element access attempted while empty.
    FIXED_VECTOR_EMPTY_ACCESS,
    /// @brief `FixedVector` index out of bounds.
    FIXED_VECTOR_INDEX_OUT_OF_BOUNDS,
    /// @brief `StringView` received invalid pointer+size range.
    STRING_VIEW_INVALID_RANGE,
    /// @brief `StringView` index out of bounds.
    STRING_VIEW_INDEX_OUT_OF_BOUNDS,
    /// @brief `FixedString` write operation exceeded fixed capacity.
    FIXED_STRING_CAPACITY_EXCEEDED,
    /// @brief `FixedString` index out of bounds.
    FIXED_STRING_INDEX_OUT_OF_BOUNDS,
    /// @brief UTF-8 validation required by API contract failed.
    UTF8_INVALID_SEQUENCE,
};

/**
 * @brief Structured metadata describing a contract violation site.
 */
struct ContractViolationInfo final {
    /// @brief Categorized violation code.
    ContractViolationCode code;
    /// @brief Human-readable violation message.
    const char* message;
    /// @brief Source file where violation was detected.
    const char* file;
    /// @brief Source line where violation was detected.
    u32 line;
    /// @brief Function where violation was detected.
    const char* function;
};

/**
 * @brief Handles all zcore contract violations using a single fatal path.
 * @param info Violation metadata.
 *
 * This function always terminates the process.
 */
[[noreturn]] inline void ContractViolation(const ContractViolationInfo& info) noexcept
{
    std::fprintf(stderr,
                 "zcore contract violation [%u]: %s at %s:%u (%s)\n",
                 static_cast<unsigned int>(info.code),
                 info.message == nullptr ? "contract violation" : info.message,
                 info.file == nullptr ? "<unknown>" : info.file,
                 static_cast<unsigned int>(info.line),
                 info.function == nullptr ? "<unknown>" : info.function);
    assert(false && "zcore contract violation");
    std::abort();
}

} // namespace zcore::detail

/**
 * @brief Reports an unconditional contract violation and terminates.
 */
#define ZCORE_CONTRACT_VIOLATION(violation_code, violation_message)                                                                \
    ::zcore::detail::ContractViolation(::zcore::detail::ContractViolationInfo{                                                     \
            (violation_code),                                                                                                      \
            (violation_message),                                                                                                   \
            __FILE__,                                                                                                              \
            static_cast<::zcore::u32>(__LINE__),                                                                                   \
            __func__,                                                                                                              \
    })

/**
 * @brief Requires expression to hold; otherwise routes to centralized fatal handler.
 */
#define ZCORE_CONTRACT_REQUIRE(expr, violation_code, violation_message)                                                            \
    do {                                                                                                                           \
        if (!(expr)) [[unlikely]] {                                                                                                \
            ZCORE_CONTRACT_VIOLATION((violation_code), (violation_message));                                                       \
        }                                                                                                                          \
    } while (false)
