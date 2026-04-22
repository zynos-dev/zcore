/**************************************************************************/
/*  error/extension_policy.hpp                                            */
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
 * @file include/zcore/error/extension_policy.hpp
 * @brief Error-code extension acceptance policy for forward compatibility.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/extension_policy.hpp>
 * const zcore::ExtensionPolicy policy =
 *     zcore::ExtensionPolicy::SameDomainForwardCompatible(zcore::kZcoreErrorDomain, 100);
 * @endcode
 */

#pragma once

#include <zcore/error/error.hpp>
#include <zcore/foundation.hpp>

namespace zcore {

/**
 * @brief Policy mode for accepting extension error codes.
 */
enum class ExtensionPolicyMode : u8 {
    /// @brief Accept only known codes from the configured base domain.
    STRICT = 0U,
    /// @brief Accept known and extension codes from base domain only.
    SAME_DOMAIN_FORWARD_COMPATIBLE = 1U,
    /// @brief Accept known and extension codes from any failure domain.
    ANY_DOMAIN_FORWARD_COMPATIBLE = 2U,
};

/**
 * @brief Deterministic policy for validating domain-qualified error-code extensions.
 *
 * Terms:
 * - known code: same domain as base domain and `1..BuiltInMaxCode()`
 * - extension code: any non-success failure code that is not known
 */
class [[nodiscard("ExtensionPolicy must be handled explicitly.")]] ExtensionPolicy final {
public:
    /// @brief Constructs invalid policy.
    constexpr ExtensionPolicy() noexcept
            : BaseDomain_(ErrorDomain::Success()), BuiltInMaxCode_(0), Mode_(ExtensionPolicyMode::STRICT), AllowOk_(false)
    {
    }

    /**
   * @brief Constructs policy from explicit parts.
   * @param baseDomain Base domain for known-code classification.
   * @param builtInMaxCode Inclusive maximum known code value.
   * @param mode Extension acceptance mode.
   * @param allowOk Whether canonical success code is accepted.
   */
    constexpr ExtensionPolicy(ErrorDomain baseDomain, i32 builtInMaxCode, ExtensionPolicyMode mode, bool allowOk = false) noexcept
            : BaseDomain_(baseDomain), BuiltInMaxCode_(builtInMaxCode), Mode_(mode), AllowOk_(allowOk)
    {
    }

    constexpr ExtensionPolicy(const ExtensionPolicy&) noexcept = default;
    constexpr ExtensionPolicy& operator=(const ExtensionPolicy&) noexcept = default;
    constexpr ExtensionPolicy(ExtensionPolicy&&) noexcept = default;
    constexpr ExtensionPolicy& operator=(ExtensionPolicy&&) noexcept = default;
    ~ExtensionPolicy() = default;

    /// @brief Returns canonical invalid policy.
    [[nodiscard]] static constexpr ExtensionPolicy Invalid() noexcept
    {
        return ExtensionPolicy();
    }

    /**
   * @brief Constructs strict policy for base-domain built-in codes only.
   */
    [[nodiscard]] static constexpr ExtensionPolicy Strict(ErrorDomain baseDomain, i32 builtInMaxCode, bool allowOk = false) noexcept
    {
        return ExtensionPolicy(baseDomain, builtInMaxCode, ExtensionPolicyMode::STRICT, allowOk);
    }

    /**
   * @brief Constructs forward-compatible policy for base-domain extensions.
   */
    [[nodiscard]] static constexpr ExtensionPolicy
    SameDomainForwardCompatible(ErrorDomain baseDomain, i32 builtInMaxCode, bool allowOk = false) noexcept
    {
        return ExtensionPolicy(baseDomain, builtInMaxCode, ExtensionPolicyMode::SAME_DOMAIN_FORWARD_COMPATIBLE, allowOk);
    }

    /**
   * @brief Constructs forward-compatible policy for any-domain extensions.
   */
    [[nodiscard]] static constexpr ExtensionPolicy
    AnyDomainForwardCompatible(ErrorDomain baseDomain, i32 builtInMaxCode, bool allowOk = false) noexcept
    {
        return ExtensionPolicy(baseDomain, builtInMaxCode, ExtensionPolicyMode::ANY_DOMAIN_FORWARD_COMPATIBLE, allowOk);
    }

    /**
   * @brief Constructs policy from raw values without validation.
   */
    [[nodiscard]] static constexpr ExtensionPolicy
    FromRawUnchecked(ErrorDomain baseDomain, i32 builtInMaxCode, ExtensionPolicyMode mode, bool allowOk = false) noexcept
    {
        return ExtensionPolicy(baseDomain, builtInMaxCode, mode, allowOk);
    }

    /// @brief Returns configured base domain.
    [[nodiscard]] constexpr ErrorDomain BaseDomain() const noexcept
    {
        return BaseDomain_;
    }

    /// @brief Returns inclusive maximum known built-in code value.
    [[nodiscard]] constexpr i32 BuiltInMaxCode() const noexcept
    {
        return BuiltInMaxCode_;
    }

    /// @brief Returns extension acceptance mode.
    [[nodiscard]] constexpr ExtensionPolicyMode Mode() const noexcept
    {
        return Mode_;
    }

    /// @brief Returns `true` when canonical success code is accepted.
    [[nodiscard]] constexpr bool AllowsOk() const noexcept
    {
        return AllowOk_;
    }

    /**
   * @brief Returns `true` when policy shape is valid.
   *
   * Valid shape:
   * - base domain is well-formed failure domain
   * - built-in max code is positive
   */
    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        if (!BaseDomain_.IsFailureDomain() || BuiltInMaxCode_ <= 0) {
            return false;
        }

        switch (Mode_) {
        case ExtensionPolicyMode::STRICT:
        case ExtensionPolicyMode::SAME_DOMAIN_FORWARD_COMPATIBLE:
        case ExtensionPolicyMode::ANY_DOMAIN_FORWARD_COMPATIBLE:
            return true;
        }
        return false;
    }

    /// @brief Returns `true` when policy shape is invalid.
    [[nodiscard]] constexpr bool IsInvalid() const noexcept
    {
        return !IsValid();
    }

    /**
   * @brief Returns `true` when code is in the known built-in range for base domain.
   */
    [[nodiscard]] constexpr bool IsKnownCode(ErrorCode code) const noexcept
    {
        return IsValid() && DomainsEqual(code.domain, BaseDomain_) && code.value > 0 && code.value <= BuiltInMaxCode_;
    }

    /**
   * @brief Returns `true` when code is an extension relative to this policy.
   */
    [[nodiscard]] constexpr bool IsExtensionCode(ErrorCode code) const noexcept
    {
        if (!IsValid() || code.IsOk() || !code.domain.IsFailureDomain() || code.value <= 0) {
            return false;
        }
        return !IsKnownCode(code);
    }

    /**
   * @brief Returns `true` when code is accepted by policy.
   */
    [[nodiscard]] constexpr bool Allows(ErrorCode code) const noexcept
    {
        if (code.IsOk()) {
            return AllowOk_;
        }
        if (!IsValid() || !code.domain.IsFailureDomain() || code.value <= 0) {
            return false;
        }

        const bool domainMatches = DomainsEqual(code.domain, BaseDomain_);
        if (domainMatches) {
            if (code.value <= BuiltInMaxCode_) {
                return true;
            }
            return Mode_ != ExtensionPolicyMode::STRICT;
        }

        return Mode_ == ExtensionPolicyMode::ANY_DOMAIN_FORWARD_COMPATIBLE;
    }

    /**
   * @brief Returns `true` when error payload code is accepted by policy.
   */
    [[nodiscard]] constexpr bool Allows(const ErrorInfo& errorInfo) const noexcept
    {
        return Allows(errorInfo.code);
    }

    /// @brief Resets to invalid policy.
    constexpr void Reset() noexcept
    {
        *this = Invalid();
    }

    [[nodiscard]] constexpr bool operator==(const ExtensionPolicy& other) const noexcept
    {
        return DomainsEqual(BaseDomain_, other.BaseDomain_) && BuiltInMaxCode_ == other.BuiltInMaxCode_ && Mode_ == other.Mode_
               && AllowOk_ == other.AllowOk_;
    }

private:
    [[nodiscard]] static constexpr bool DomainsEqual(ErrorDomain lhs, ErrorDomain rhs) noexcept
    {
        return lhs.id == rhs.id && detail::CStrEquals(lhs.name, rhs.name);
    }

    ErrorDomain BaseDomain_;
    i32 BuiltInMaxCode_;
    ExtensionPolicyMode Mode_;
    bool AllowOk_;
};

/**
 * @brief Extension-policy validation error codes.
 */
enum class ExtensionPolicyErrorCode : u8 {
    /// @brief Policy shape is invalid.
    INVALID_POLICY = 1,
    /// @brief Candidate error domain is disallowed by policy.
    DOMAIN_REJECTED = 2,
    /// @brief Candidate error code value is disallowed by policy.
    CODE_REJECTED = 3,
};

/// @brief Built-in extension-policy error domain identifier.
inline constexpr ErrorDomain kExtensionPolicyErrorDomain{
        .id = 6U,
        .name = "extension_policy",
};

/**
 * @brief Constructs extension-policy-domain error payload.
 * @param code Extension-policy error code.
 * @param operation Operation identifier.
 * @param message Human-readable message.
 * @param file Source file path.
 * @param line Source line.
 * @return Structured extension-policy error.
 */
[[nodiscard]] constexpr Error MakeExtensionPolicyError(
        ExtensionPolicyErrorCode code, const char* operation, const char* message, const char* file = "", u32 line = 0U) noexcept
{
    return MakeError(kExtensionPolicyErrorDomain,
                     static_cast<i32>(code),
                     MakeErrorContext("extension_policy", operation, message, file, line));
}

} // namespace zcore
