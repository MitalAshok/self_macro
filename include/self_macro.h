#ifndef SELF_MACRO_H
#define SELF_MACRO_H


namespace self_macro {
namespace detail {

template<typename T>
struct type_identity { using type = T; };

template<typename T>
struct remove_pointer;
template<typename T>
struct remove_pointer<T*> { using type = T; };

template<bool B, typename T>
struct value_dependant_type_identity { using type = T; };

template<typename Tag>
struct _retrieve_type {

#if defined(__GNUC__) && !defined(__clang__)
// Silence unnecessary warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif

    constexpr friend auto _self_macro_retrieve_type_friend_function(_retrieve_type<Tag>);

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

    constexpr auto _self_macro_retrieve_type_member_function() {
        return _self_macro_retrieve_type_friend_function(*this);
    }
};

template<typename Tag, typename Stored>
struct _store_type {
    constexpr friend auto _self_macro_retrieve_type_friend_function(_retrieve_type<Tag>) {
        return type_identity<Stored>{};
    }
};

}

template<typename Tag, typename ToStore>
constexpr
typename ::self_macro::detail::value_dependant_type_identity<(::self_macro::detail::_store_type<Tag, ToStore>{}, true), void>::type
store() noexcept {
    return static_cast<void>(::self_macro::detail::_store_type<Tag, ToStore>{});
}

template<typename Tag, typename ToStore, typename T>
constexpr
typename ::self_macro::detail::value_dependant_type_identity<(::self_macro::detail::_store_type<Tag, ToStore>{}, true), T&&>::type
store(T&& value) noexcept {
    return (static_cast<void>(::self_macro::detail::_store_type<Tag, ToStore>{}), static_cast<T&&>(value));
}

template<typename Tag, typename ToStore, typename Result = ToStore>
using store_with_type = typename ::self_macro::detail::value_dependant_type_identity<::self_macro::store<Tag, ToStore>(true), Result>::type;

template<typename Tag>
using retrieve = typename decltype(::self_macro::detail::_retrieve_type<Tag>{}._self_macro_retrieve_type_member_function())::type;

}

#define SELF_MACRO_WRAP(...) typename decltype(::self_macro::detail::type_identity< __VA_ARGS__ >{})::type
#define SELF_MACRO_STORE_TYPE_DECL(TAG, ...) static_assert(::self_macro::store< TAG , __VA_ARGS__ >(true), "")
#define SELF_MACRO_STORE_TYPE_EXPLICIT_INST(TAG, ...) template struct self_macro::detail::_store_type< TAG , __VA_ARGS__ >

#define SELF_MACRO_DEFINE_SELF(NAME, ACCESS) \
private: struct _self_macro_self_type_tag; \
auto _self_macro_self_type_tag() -> \
    ::self_macro::store_with_type<struct _self_macro_self_type_tag, \
    typename ::self_macro::detail::remove_pointer<decltype(this)>::type, void>; \
ACCESS: using NAME = ::self_macro::retrieve<struct _self_macro_self_type_tag>

#endif  // SELF_MACRO_H
