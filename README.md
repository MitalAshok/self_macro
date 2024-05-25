# self_macro

A C++14 header only library that exposes a macro that creates a type alias
for the current class without naming it.

Also exposes macros for easy stateful template metaprogramming.

For example:

```c++
struct struct_name {
    SELF_MACRO_GET_SELF(type_alias_name, public);
    static_assert(std::is_same_v<type_alias_name, struct_name>);
};
```

Useful for code generation via other macros.

The only C++14 feature used is return type deduction `__cpp_decltype_auto >= 201304L`.
If your compiler supports this in C++11 mode, this library can also be used
in C++11. (Clang and GCC currently do not support this extension)

## Usage

### `#define SELF_MACRO_DEFINE_SELF(NAME, ACCESS)`

`NAME` should be an id, and `ACCESS` one of `private`, `protected` or `public`.
Equivalent to `ACCESS: using NAME = <self>`, where `<self>` is the type of the
current class. This will not be an injected-class-name.

### `#define SELF_MACRO_DEFINE_SELF_NAMED_TAG(NAME, ACCESS, TAG_NAME)`

`SELF_MACRO_DEFINE_SELF` uses a name
`SELF_MACRO ## NAME ## SELF_MACRO`
internally to define a member function and member nested class
with that name. This macro lets you use a different custom name
if the default is a problem.

### `store<Tag, ToStore>()`

```c++
namespace self_macro {

template<typename Tag, typename ToStore, typename T = void>
constexpr T store() noexcept(noexcept(T()));

template<typename Tag, typename ToStore, typename T>
constexpr T&& store(T&& value) noexcept;

}
```

Associates `ToStore` with `Tag` when these functions are used (even in an unevaluated context).

If passed an argument, return that same argument. Otherwise, returns `T()`.

The association is done during overload resolution so if these
are called via ADL (or if the compiler implements two phase lookup in a
nonstandard way) the call may need to be instantiated first.

### `typename retrieve<T>`

```c++
namespace self_macro {

template<typename Tag>
using retrieve = /* ... */;

}
```

If a type has been previously associated with the given `Tag`, this will be an
alias for that type. If there is no association or more than one association,
a non-SFINAE compile time error will occur.

### `typename store_with_type<Tag, ToStore, Result>`

```c++
namespace self_macro {

template<typename Tag, typename ToStore, typename Result = ToStore>
using store_with_type = /* ... */ Result;

}
```

Like `store` but evaluates to a type so that it can be used in contexts that
require a type: An alias for `Result` that associates `ToStore` with `Tag`
when instantiated.

### `#define SELF_MACRO_WRAP(...)`

Wraps a variadic argument naming a type so it is safe to use as a single
macro argument. E.g., `std::array<int, 5>` would be two macro
arguments, `std::array<int` and `5>`, but `SELF_MACRO_WRAP(std::array<int, 5>)`
would name the same type but be passed as one macro argument.

### `#define SELF_MACRO_STORE_TYPE_DECL(TAG, TOSTORE)`

Expands to a (`static_assert`) declaration that associates the type named by
`TOSTORE` to the type named by `TAG` when instantiated. (Variadic, so no need to wrap
either type name)

### `#define SELF_MACRO_STORE_TYPE_EXPLICIT_INST(TAG, TOSTORE)`

When placed in the `::` namespace (at file scope), expands to an explicit template
specialisation that associates the type named by `TOSTORE` with the type named by
`TAG`. In particular, this means that access checks are not done in any subexpression.

## How?

It is not easy to get the type of the enclosing struct at some points in the
struct without explicitly naming it. One way is to use `this` which is of
type "pointer-to-struct". However, `this` can only be used in two circumstances:

 1. In the declaration and definition of non-static member functions at any point after the cv-qualifiers
 2. Within a default member initializer

These look like:

```c++
struct S {
    int a;
    int b /* this cannot be used here */ = /* this can be used here */ this->a;

    int f(int x) /* this cannot be used up to here */ & /* this can be used here */ noexcept(noexcept(this)) {
        return x + this->b;
    }
    auto g(int y) /* this can be used here */ -> decltype(this->b + y) {
        return this->b + y;
    }

    /* this cannot be used here */
    // using self = std::remove_pointer_t<decltype(this)>;
    // Nor in static member functions
};
```

One way to get the magic "self" type is to take the type of `this` from
a place it is allowed (say a trailing return type) and get its value
in the place it's needed (to make a type alias).

This can be achieved similar to how private members can be accessed
using an explicit instantiation, although we only need an implicit
instantiation here to get access to `decltype(this)`.

```c++
#include <type_traits>

// Or use C++20 std::type_identity
template<typename T> struct type_identity { using type = T; };

template<typename Tag>
struct retrieve_type {
    // This auto placeholder is resolved later (which is why C++14 is needed)
    friend constexpr auto retrieve_type_friend(retrieve_type<Tag>);

    friend auto get() {
        return retrieve_type_friend(*this);
    }
};

template<typename Tag, typename Stored>
struct store_type {
    // This will resolve the auto for retrieve_type_friend(retrieve_type<Tag>),
    // which will be found by `retrieve_type<Tag>::get` to get `Stored`
    friend constexpr auto retrieve_type_friend(retrieve_type<Tag>) {
        return std::type_identity<Stored>{};
    }
};

// Implicitly instantiate `store_type<TAG, TYPE>` to make
// `retrieve_type<TAG>::get` be `type_identity<TYPE>`
#define STORE_TYPE(TAG, TYPE) static_cast<void>(store_type<TAG, TYPE>{})
// Retrieve `TYPE` from `type_identity<TYPE>`
#define RETRIEVE_TYPE(TAG) typename decltype(retrieve_type<TAG>::get())::type

// Now we only need to store the type at a place we can use `this` and
// retrieve it outside. For the tag, use a nested class with an unused name
// (Here, both the function and the tag are called `_`)
#define DEFINE_SELF(NAME, ACCESS) \
private: \
    struct _; \
    auto _() -> decltype( \
        STORE_TYPE(struct _, typename std::remove_pointer<decltype(this)>::type) \
    ); \
ACCESS: using NAME = RETRIEVE_TYPE(struct _)

// Calling `DEFINE_SELF(NAME, ACCESS)` inside a struct `S` will
// store `S` to the tag `S::_`, and `_` can easily be named inside the class
```

The above is a minimal snippet for the `DEFINE_SELF` macro (called
`SELF_MACRO_DEFINE_SELF` in this project's header file).

To make this C++11 compatible, we would have to be able to get rid of the
placeholder type in `retrieve_type_friend`.

This could be done by declaring it like `template<typename T> T retrieve_type_friend(retrieve_type<Tag>)`,
and having the definition give `T` a default value of `Stored`. However, this is non-standard, as a template
friend declaration with default arguments can only be declared once.

## Alt uses

The names of anonymous classes and unions can
be retrieved outside the struct:

```c++
struct S {
    struct tag;
    union {
        int x = /* this here refers to the type of the anonymous union */
            self_macro::store<
                tag,
                std::remove_pointer<decltype(this)>::type
            >(0);  // tag is now associated with the unspeakable type!
    };
    int a;
    // Can't use `self_macro::retrieve<tag>` here because x's
    // initializer hasn't been instantiated yet (not a complete-class context)
};


using anonymous_union_type = self_macro::retrieve<S::tag>;
static_assert(sizeof(anonymous_union_type) == sizeof(int));
static_assert(sizeof(S) == 2 * sizeof(int));
// anonymous_class_type is "S::<unnamed union>"
```

(It can also be named by simply calling a function in
an initializer, like `int x = f<decltype(this)>()`, but
it can't be smuggled out further)

Whether or not anonymous union types exist doesn't seem entirely clear. The standard says
the names of members exist solely in the scope of the enclosing type, so it might be that it declares a type with
no members (which is a confusing defintion), or that no type exists and `this` should refer to
the enclosing type (which is how MSVC implements anonymous unions).

---

You can also use this as a general mapping structure, for example:

```c++
// map_key<K> is the key to the map by being a unique type for each key
template<int K>
struct map_key : std::integral_constant<int, K> {};

static_assert((
    self_macro::store<map_key<0>, signed char>(),
    self_macro::store<map_key<1>, short>(),
    self_macro::store<map_key<2>, int>(),
    self_macro::store<map_key<3>, long>(),
    self_macro::store<map_key<4>, long long>(),
    true
), "");

template<int i>
using get_from_map = self_macro::retrieve<map_key<i>>;

static_assert(std::is_same<get_from_map<0>, signed char>::value, "");
static_assert(std::is_same<get_from_map<3>, long>::value, "");
```

[Try it on Compiler Explorer](https://godbolt.org/z/8GEf86hKn)

Using a wrapper around `type_identity` for type keys and having
`integral_constant` values for non-type values.

---

You can also reimplement the private member accessor with these:

```c++
// Setup: explicitly instantiate to store the type
template<typename Tag, typename Type, Type Value, self_macro::store_with_type<Tag, std::integral_constant<Type, Value>, std::nullptr_t> = nullptr>
struct store_value {};

// Type with private member we want to access
struct v {
    constexpr int get_x() const { return x; }
private:
    int x = 4;
};

// Explicitly instantiate `store_value` to map `v_x_tag` to
// an integral_constant holding `&v::x`
struct v_x_tag;
template
struct store_value<v_x_tag, decltype(&v::x), &v::x>;

// Or use the convenience macro:
// SELF_MACRO_STORE_TYPE_EXPLICIT_INST(v_x_tag, std::integral_constant<decltype(&v::x), &v::x>);

// Retrieve that value
constexpr int v::* v_x = self_macro::retrieve<v_x_tag>::value;


// Test it
constexpr v get_default() {
    v a;
    return a;
}
constexpr v get_modified(int to) {
    v a;
    a.*v_x = to;
    return a;
}

static_assert(get_default().get_x() == 4, "");
static_assert(get_modified(7).get_x() == 7, "");
```

[Try it on Compiler Explorer](https://godbolt.org/z/8nEzn918v)

(Though with a C++14 requirement over a C++11 requirement)
