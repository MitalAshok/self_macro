#include <type_traits>
#include <iostream>

#include <self_macro.h>

template<typename T, typename U>
static constexpr bool is_same_v = std::is_same<T, U>::value;

// test::self is self
struct test1 {
    SELF_MACRO_DEFINE_SELF(self, public);
    self* x() { return this; }
};
static_assert(is_same_v<test1, typename test1::self>, "");

// Works in templates
template<typename T>
struct test2 {
    SELF_MACRO_DEFINE_SELF(self, public);
    self* x() { return this; }
};
static_assert(is_same_v<test2<int>, typename test2<int>::self>, "");

// Test anonymous structs
struct test3 {
    struct tag;
    struct {
        int anon_member = SELF_MACRO_STORE_TYPE_WITH_EXPR(tag, typename std::remove_pointer<decltype(this)>::type, 0);
    };
    int other_member;
    static void f() {
        // Must be done inside after initializers have been instantiated
        using anonymous_struct_type = SELF_MACRO_RETRIEVE_TYPE(tag);
        static_assert(!is_same_v<anonymous_struct_type, test3>, "");
        constexpr anonymous_struct_type a{1};
#if defined(__clang__) || defined(_MSC_VER)
        static_assert(a.anon_member == 1, "");  // gcc thinks that this member is inaccessible
#endif
#ifdef __clang__
        constexpr test3 b{a};  // gcc thinks this constructor (aggregate initialization) is inaccessible
                               // msvc doesn't think it exists at all
        static_assert(b.anon_member == 1, "");
#endif
    }

};

// Convenience function to print a string which includes a type name
template<typename T>
void p() {
    std::cout <<
#ifdef _MSC_VER
        __FUNCSIG__ << '\n' <<
#elif defined(__GNUC__) || defined(__clang__)
        __PRETTY_FUNCTION__ << '\n' <<
#endif
        typeid(T).name() << '\n';
}


int main() {
    test1 a;
    if (a.x() != &a) return 1;
    test2<int> b;
    if (b.x() != &b) return 1;

    using anonymous_struct_type = SELF_MACRO_RETRIEVE_TYPE(test3::tag);
    static_assert(sizeof(anonymous_struct_type) == sizeof(int), "");
    static_assert(sizeof(test3) == 2 * sizeof(int), "");
    p<anonymous_struct_type>();
#if defined(__clang__) || defined(_MSC_VER)
    anonymous_struct_type c;  // gcc thinks the constructor is inaccessible from main
#endif
    test3::f();

    struct anon_union_tag;
    union {
        int function_scope_anon_union = SELF_MACRO_STORE_TYPE_WITH_EXPR(anon_union_tag, typename std::remove_pointer<decltype(this)>::type, 0);
    };
    using anon_union_type = SELF_MACRO_RETRIEVE_TYPE(anon_union_tag);
    p<anon_union_type>();
}

template<int K>
struct map_key : std::integral_constant<int, K> {};

// Different possible ways to store types
static_assert((
    // SELF_MACRO_STORE_TYPE(map_key<1>, signed char),
    // SELF_MACRO_STORE_TYPE(map_key<2>, short),
    SELF_MACRO_STORE_TYPE(map_key<4>, SELF_MACRO_STORE_TYPE_WITH_TYPE(map_key<3>, int, long)),
    SELF_MACRO_STORE_TYPE(map_key<5>, long long),
    SELF_MACRO_STORE_TYPE_WITH_EXPR(map_key<0>, char, true) &&
    SELF_MACRO_STORE_TYPE_WITH_EXPR(map_key<-1>, unsigned char, true) &&
    SELF_MACRO_STORE_TYPE_WITH_EXPR(map_key<-2>, unsigned short, true) &&
    SELF_MACRO_STORE_TYPE_WITH_EXPR(map_key<-3>, unsigned int, true) &&
    SELF_MACRO_STORE_TYPE_WITH_EXPR(map_key<-4>, unsigned long, true) &&
    SELF_MACRO_STORE_TYPE_WITH_EXPR(map_key<-5>, unsigned long long, true) &&
    true
), "");

SELF_MACRO_STORE_TYPE_EXPLICIT_INST(map_key<1>, signed char);
SELF_MACRO_STORE_TYPE_DECL(map_key<2>, short);

template<int i>
using get_from_map = SELF_MACRO_RETRIEVE_TYPE(map_key<i>);

static_assert(is_same_v<get_from_map<0>, char>, "");
static_assert(is_same_v<get_from_map<1>, signed char>, "");
static_assert(is_same_v<get_from_map<2>, short>, "");
static_assert(is_same_v<get_from_map<3>, int>, "");
static_assert(is_same_v<get_from_map<4>, long>, "");
static_assert(is_same_v<get_from_map<5>, long long>, "");
