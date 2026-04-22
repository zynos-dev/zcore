/**************************************************************************/
/*  type_constraints.hpp                                                  */
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
 * @file include/zcore/type_constraints.hpp
 * @brief Shared compile-time type constraints for zcore templates.
 * @par Usage
 * @code{.cpp}
 * #include <zcore/type_constraints.hpp>
 * @endcode
 */

#pragma once

#include <type_traits>

namespace zcore::constraints {

template <typename ValueT>
concept NonReferenceType = !std::is_reference_v<ValueT>;

template <typename ValueT>
concept NonVoidType = !std::is_void_v<ValueT>;

template <typename ValueT>
concept NonArrayType = !std::is_array_v<ValueT>;

template <typename ValueT>
concept NonFunctionType = !std::is_function_v<ValueT>;

template <typename ValueT>
concept NonConstType = !std::is_const_v<ValueT>;

template <typename ValueT>
concept ObjectType = std::is_object_v<ValueT>;

template <typename ValueT>
concept ObjectOrVoidType = std::is_object_v<ValueT> || std::is_void_v<ValueT>;

template <typename ValueT>
concept NonReferenceNonVoidType = NonReferenceType<ValueT> && NonVoidType<ValueT>;

template <typename ValueT>
concept ValueObjectType =
        ObjectType<ValueT> && NonReferenceType<ValueT> && NonVoidType<ValueT> && NonArrayType<ValueT> && NonFunctionType<ValueT>;

template <typename ValueT>
concept MutableValueObjectType = ValueObjectType<ValueT> && NonConstType<ValueT>;

template <typename ValueT>
concept BorrowablePointeeType = NonReferenceType<ValueT> && ObjectOrVoidType<ValueT> && NonFunctionType<ValueT>;

template <typename ValueT>
concept MutableBorrowablePointeeType = BorrowablePointeeType<ValueT> && NonConstType<ValueT>;

template <typename ValueT>
concept NonReferenceNonVoidNonFunctionType = NonReferenceType<ValueT> && NonVoidType<ValueT> && NonFunctionType<ValueT>;

template <typename ValueT>
concept MoveConstructibleType = std::is_move_constructible_v<ValueT>;

template <typename ValueT>
concept NothrowMoveConstructibleType = std::is_nothrow_move_constructible_v<ValueT>;

template <typename ValueT>
concept MoveAssignableType = std::is_move_assignable_v<ValueT>;

template <typename ValueT>
concept MutableNothrowMovableValueType = MutableValueObjectType<ValueT> && MoveConstructibleType<ValueT>
                                         && NothrowMoveConstructibleType<ValueT> && MoveAssignableType<ValueT>;

} // namespace zcore::constraints

#define ZCORE_STATIC_REQUIRE(...) static_assert(__VA_ARGS__)
