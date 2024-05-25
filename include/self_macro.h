/// This file can be found at https://github.com/MitalAshok/self_macro
/// See the README for help
/// Quick usage: `SELF_MACRO_DEFINE_SELF(ACCESS, NAME)` is equivalent to
/// `ACCESS: using NAME = *self*;`, where `NAME` will become a type alias
/// for the class the macro was used in.

/// MIT License
///
/// Copyright (c) 2022-2024 Mital Ashok
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in all
/// copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.

#ifndef SELF_MACRO_H
#define SELF_MACRO_H

// NOLINTBEGIN

namespace self_macro {
namespace detail {

template<typename T>
struct type_identity { using type = T; };

template<typename T>
struct remove_pointer;
template<typename T>
struct remove_pointer<T*> { using type = T; };

template<bool, typename T>
struct value_dependant_type_identity { using type = T; };

template<typename Tag>
struct _retrieve_type {

#if defined(__GNUC__) && !defined(__clang__)
// Silence unnecessary warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif

    constexpr friend auto _self_macro_retrieve_type_friend_function(_retrieve_type<Tag>) noexcept;

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

    constexpr auto _self_macro_retrieve_type_member_function() noexcept {
        return _self_macro_retrieve_type_friend_function(*this);
    }
};

template<typename Tag, typename Stored>
struct _store_type {
    constexpr friend auto _self_macro_retrieve_type_friend_function(_retrieve_type<Tag>) noexcept {
        return type_identity<Stored>{};
    }
};

}  // namespace detail

template<typename Tag, typename ToStore, typename Result = ToStore>
using store_with_type = typename ::self_macro::detail::value_dependant_type_identity<(static_cast<void>(::self_macro::detail::_store_type<Tag, ToStore>{}), false), Result>::type;

template<typename Tag, typename ToStore, typename T = void>
constexpr store_with_type<Tag, ToStore, T> store() noexcept(noexcept(T())) {
    return T();
}

template<typename Tag, typename ToStore, typename T>
constexpr store_with_type<Tag, ToStore, T&&> store(T&& value) noexcept {
    return static_cast<T&&>(value);
}

template<typename Tag>
using retrieve = typename decltype(::self_macro::detail::_retrieve_type<Tag>{}._self_macro_retrieve_type_member_function())::type;

}  // namespace self_macro

#define SELF_MACRO_WRAP(...) typename decltype(::self_macro::detail::type_identity< __VA_ARGS__ >{})::type
#define SELF_MACRO_STORE_TYPE_DECL(TAG, ...) static_assert(::self_macro::store< TAG , __VA_ARGS__ >(true), "")
#define SELF_MACRO_STORE_TYPE_EXPLICIT_INST(TAG, ...) template struct self_macro::detail::_store_type< TAG , __VA_ARGS__ >
#define SELF_MACRO_STORE_TYPE_XCONCAT(A, B, C) A ## B ## C
#define SELF_MACRO_STORE_TYPE_CONCAT(A, B, C) SELF_MACRO_STORE_TYPE_XCONCAT(A, B, C)

#define SELF_MACRO_DEFINE_SELF(NAME, ACCESS) SELF_MACRO_DEFINE_SELF_WITH_NAME(NAME, ACCESS, SELF_MACRO_STORE_TYPE_CONCAT(SELF_MACRO, NAME, SELF_MACRO))

#define SELF_MACRO_DEFINE_SELF_WITH_NAME(NAME, ACCESS, TAG_NAME) \
private: struct TAG_NAME ; \
auto TAG_NAME () -> \
    ::self_macro::store_with_type<struct TAG_NAME , \
    typename ::self_macro::detail::remove_pointer<decltype(this)>::type, void>; \
ACCESS: using NAME = ::self_macro::retrieve<struct TAG_NAME >

// NOLINTEND

#endif  // SELF_MACRO_H
