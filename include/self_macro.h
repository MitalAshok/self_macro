#ifndef SELF_MACRO_H
#define SELF_MACRO_H


namespace self_macro_namespace {
namespace detail {

template<typename T>
struct type_identity { using type = T; };

template<typename T>
struct remove_pointer;
template<typename T>
struct remove_pointer<T*> { using type = T; };

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
}

#define SELF_MACRO_WRAP(...) typename decltype(::self_macro_namespace::detail::type_identity< __VA_ARGS__ >{})::type
#define SELF_MACRO_STORE_TYPE(TAG, ...) static_cast<void>(::self_macro_namespace::detail::_store_type< TAG , __VA_ARGS__ >{})
#define SELF_MACRO_STORE_TYPE_WITH_EXPR(TAG, TYPE, ...) (SELF_MACRO_STORE_TYPE( TAG , TYPE ), (__VA_ARGS__))
#define SELF_MACRO_STORE_TYPE_WITH_TYPE(TAG, TYPE, ...) typename decltype(SELF_MACRO_STORE_TYPE_WITH_EXPR( TAG , TYPE , ::self_macro_namespace::detail::type_identity< __VA_ARGS__ >{}))::type
#define SELF_MACRO_STORE_TYPE_DECL(TAG, ...) static_assert(SELF_MACRO_STORE_TYPE_WITH_EXPR( TAG, SELF_MACRO_WRAP( __VA_ARGS__ ), true), "")
#define SELF_MACRO_STORE_TYPE_EXPLICIT_INST(TAG, ...) template struct self_macro_namespace::detail::_store_type< TAG, __VA_ARGS__ >
#define SELF_MACRO_RETRIEVE_TYPE(...) typename decltype(::self_macro_namespace::detail::_retrieve_type< __VA_ARGS__ >{}._self_macro_retrieve_type_member_function())::type

#define SELF_MACRO_DEFINE_SELF(NAME, ACCESS) \
private: struct _self_macro_self_type_tag; \
auto _self_macro_self_type_tag() -> decltype( \
    SELF_MACRO_STORE_TYPE(struct _self_macro_self_type_tag, typename ::self_macro_namespace::detail::remove_pointer<decltype(this)>::type) \
); \
ACCESS: using NAME = SELF_MACRO_RETRIEVE_TYPE(struct _self_macro_self_type_tag)

#endif  // SELF_MACRO_H
