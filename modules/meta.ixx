// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

module;

#include <array>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#ifdef JACINTH_USE_REFLECTION
#include <meta>
#else
#include "boost/pfr.hpp"
#endif

export module jacinth:meta;

// meta specialization definitions
export namespace jacinth
{

// kind of entry
enum class entry_kind { rename, include, exclude, from, flatten };

struct rename_entry {
    static constexpr auto kind = entry_kind::rename;
    std::string_view key;

    // this name is a bit confusing but basically
    // key is the json name, field is the C++ name
    std::string_view field;
};

/**
 * Rename the object member to the specified json key.
 * A JSON object with `key` will write that value to the struct `member`.
 * A C++ struct with `member` will write that value to JSON key `key`.
 */
constexpr auto rename(std::string_view member, std::string_view key)
{
    return rename_entry{.key = key, .field = member};
}

template <std::size_t N>
struct include_entry {
    static constexpr auto kind = entry_kind::include;
    std::array<std::string_view, N> keys;
};

/**
 * Include this set of keys in serialization/deserialization.
 * This works on normal keys and renamed keys.
 * Keys that have from-specializations will be included regardless.
 */
template <class... K>
constexpr auto include(K... keys)
{
    return include_entry<sizeof...(K)>{.keys = {std::string_view(keys)...}};
}

template <std::size_t N>
struct exclude_entry {
    static constexpr auto kind = entry_kind::exclude;
    std::array<std::string_view, N> keys;
};

/**
 * Exclude this set of keys from serialization/deserialization.
 * This works on normal keys and renamed keys.
 * Keys that have from-specializations will be included regardless.
 */
template <class... K>
constexpr auto exclude(K... keys)
{
    return exclude_entry<sizeof...(K)>{.keys = {std::string_view(keys)...}};
}

template <class F>
struct from_entry {
    static constexpr auto kind = entry_kind::from;
    std::string_view key;
    F accessor;
};

/**
 * Define a special format for this particular JSON key.
 * This must be a lambda returning a reference to a member.
 * Keys specified with from will be included regardless of include/exclude settings.
 */
template <class F>
constexpr auto from(std::string_view key, F accessor)
{
    return from_entry<F>{key, accessor};
}

template <std::size_t N>
struct flatten_entry {
    static constexpr auto kind = entry_kind::flatten;
    std::array<std::string_view, N> keys;
};

/**
 * Flatten a struct child's members onto this JSON object.
 * e.g. a struct with a child struct with 3 members will have those three
 * members placed into the root of the parent struct's JSON document.
 * THIS DOES NOT AFFECT NON-STRUCT TYPES!
 */
template <class... K>
constexpr auto flatten(K... keys)
{
    return flatten_entry<sizeof...(K)>{.keys = {std::string_view(keys)...}};
}


template <class... E>
struct obj {
    constexpr obj() = default;

    constexpr obj(E... e)
        requires(sizeof...(E) > 0)
        : entries{std::move(e)...}
    {
    }

    std::tuple<E...> entries;
};

// default to no metadata
template <class T>
inline constexpr obj<> meta{};

struct field_meta {
    std::string_view key;
    bool emit;
    bool flatten;
};

// the actual resolver
template <class T>
struct meta_resolver {
    static constexpr auto &spec = jacinth::meta<T>;

    template <class Entry>
    static constexpr bool has_key(const Entry &e, std::string_view key)
    {
        for (auto k : e.keys)
            if (k == key)
                return true;
        return false;
    }

    // include
    static constexpr bool included(std::string_view key)
    {
        bool have_include = false;
        bool found = false;

        const auto check = [&](auto const &e) {
            if constexpr (std::decay_t<decltype(e)>::kind == entry_kind::include) {
                have_include = true;
                found = found || has_key(e, key);
            }
        };

        std::apply([&](auto const &...e) { (check(e), ...); }, spec.entries);

        // if no include spec is defined, assume every key is included
        return !have_include || found;
    }

    // exclude
    static constexpr bool excluded(std::string_view key)
    {
        bool found = false;

        const auto check = [&](auto const &e) {
            if constexpr (std::decay_t<decltype(e)>::kind == entry_kind::exclude)
                found = found || has_key(e, key);
        };

        std::apply([&](auto const &...e) { (check(e), ...); }, spec.entries);

        return found;
    }

    // whether to include a key at all
    static constexpr bool emit(std::string_view key)
    {
        return included(key) && !excluded(key);
    }

    // blah
    inline static constexpr auto field_names =
#ifdef JACINTH_USE_REFLECTION
        [] {
            constexpr auto members = std::define_static_array(
                std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()));

            std::array<std::string_view, members.size()> names{};
            for (std::size_t i = 0; i < members.size(); ++i)
                names[i] = std::meta::identifier_of(members[i]);
            return names;
        }();
#else
        boost::pfr::names_as_array<T>();
#endif

    // rename
    static constexpr std::string_view key_for(std::string_view name)
    {
        std::string_view key = name;

        const auto check = [&](auto const &e) {
            if constexpr (std::decay_t<decltype(e)>::kind == entry_kind::rename)
                if (e.field == name)
                    key = e.key;
        };

        std::apply([&](auto const &...e) { (check(e), ...); }, spec.entries);
        return key;
    }

    // from
    template <class Obj, class Fn>
    static constexpr void for_each_from(Obj &obj, Fn &&fn)
    {
        const auto visit = [&](auto const &e) {
            if constexpr (std::decay_t<decltype(e)>::kind == entry_kind::from)
                fn(e.key, e.accessor(obj));
        };

        std::apply([&](auto const &...e) { (visit(e), ...); }, spec.entries);
    }

    // flatten
    static constexpr bool flatten(std::string_view key)
    {
        bool found = false;

        const auto check = [&](auto const &e) {
            if constexpr (std::decay_t<decltype(e)>::kind == entry_kind::flatten)
                found = found || has_key(e, key);
        };

        std::apply([&](auto const &...e) { (check(e), ...); }, spec.entries);

        return found;
    }

};

// compile-time meta resolution
template <class T>
struct meta_plan {
    inline static constexpr std::size_t count = meta_resolver<T>::field_names.size();

    std::array<field_meta, count> fields{};
};

template <class T>
consteval auto make_plan()
{
    constexpr auto res = meta_resolver<T>();
    constexpr auto names = res.field_names;
    meta_plan<T> p{};

    for (std::size_t i = 0; i < names.size(); ++i) {
        const auto key = res.key_for(names[i]);
        p.fields[i] = field_meta{key, res.emit(key), res.flatten(key)};
    }

    return p;
}

template <class T>
inline constexpr auto plan = make_plan<T>();

} // namespace jacinth