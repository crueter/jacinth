module;

#include <array>
#include <cstdint>
#include <cstring>
#include <expected>
#include <initializer_list>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

// force yyjson to have external linkage
#define yyjson_api_inline yyjson_inline
#include "yyjson.h"

#ifdef JACINTH_USE_REFLECTION
#include <meta>
#else
#include "boost/pfr.hpp"
#endif

export module jacinth;

export namespace jacinth
{
enum class errc {
    ok,
    invalid_json,
    type_mismatch,
    missing_value,
    write_error,
};
}

template <typename T>
jacinth::errc parseValue(yyjson_val *val, T &field);
template <typename T>
jacinth::errc writeValue(yyjson_mut_doc *doc, yyjson_mut_val *val, const T &field);

export namespace jacinth
{

// std::error_code
namespace detail
{

class error : public std::error_category {
    const char *name() const noexcept override
    {
        return "jacinth";
    }

    std::string message(int c) const override
    {
        switch (errc(c)) {
        [[unlikely]] case errc::ok:
            return "jacinth: OK";
        case errc::invalid_json:
            return "jacinth: JSON text failed to parse";
        case errc::type_mismatch:
            return "jacinth: type mismatch while parsing value";
        case errc::missing_value:
            return "jacinth: required value is missing";
        case errc::write_error:
            return "jacinth: JSON write error";
        }
        return "jacinth: unknown error";
    }
};

inline const std::error_category &jacinth_category() noexcept
{
    static const detail::error cat;
    return cat;
}

} // namespace detail

inline std::error_code make_error_code(errc e) noexcept
{
    return {int(e), detail::jacinth_category()};
}

// iterator forwarders
template <typename Iter>
struct iterable_view {
    Iter b, e;
    Iter begin() const
    {
        return b;
    }
    Iter end() const
    {
        return e;
    }
};

class const_array_iterator;
class const_object_iterator;
class mut_array_iterator;
class mut_object_iterator;

// forwarder for yyjson_write_flag
struct write_opts {
    bool pretty = false;
    bool escapeUnicode = false;
    bool escapeSlashes = false;
    bool allowInfAndNan = false;
    bool writeInfAndNanAsNull = false;
    bool allowInvalidUnicode = false;
    bool prettyTwoSpaces = false;
    bool endingNewline = false;

    yyjson_write_flag to_flags() const
    {
        yyjson_write_flag f = 0;
        f |= pretty ? YYJSON_WRITE_PRETTY : 0;
        f |= escapeUnicode ? YYJSON_WRITE_ESCAPE_UNICODE : 0;
        f |= escapeSlashes ? YYJSON_WRITE_ESCAPE_SLASHES : 0;
        f |= allowInfAndNan ? YYJSON_WRITE_ALLOW_INF_AND_NAN : 0;
        f |= writeInfAndNanAsNull ? YYJSON_WRITE_INF_AND_NAN_AS_NULL : 0;
        f |= allowInvalidUnicode ? YYJSON_WRITE_ALLOW_INVALID_UNICODE : 0;
        f |= prettyTwoSpaces ? YYJSON_WRITE_PRETTY_TWO_SPACES : 0;
        f |= endingNewline ? YYJSON_WRITE_NEWLINE_AT_END : 0;
        return f;
    }
};

// initializer-list specializations
template <typename T>
struct is_init_list : std::false_type {};

template <typename T>
struct is_init_list<std::initializer_list<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_init_list_v = is_init_list<T>::value;

// exclude target types that are not meaningful JSON values (nullptr, init lists, and char *)
// without this, assignment to an std::string or std::vector is ambiguous
template <typename T>
concept json_convertible = !std::is_pointer_v<T> && !std::is_same_v<T, std::nullptr_t> &&
                           !std::is_same_v<std::remove_cv_t<T>, char> && !is_init_list_v<T>;

// TODO: is_<type> funcs

// An immutable JSON-ish value
class value {
protected:
    yyjson_doc *m_doc;
    yyjson_val *m_val;

public:
    explicit value() : m_doc(nullptr), m_val(nullptr) {}
    value(yyjson_doc *d, yyjson_val *v) : m_doc(d), m_val(v) {}

    // validity checks
    bool is_null() const
    {
        return !m_val || yyjson_is_null(m_val);
    }

    // access ops
    value operator[](std::string_view key) const
    {
        return {m_doc, yyjson_obj_getn(m_val, key.data(), key.size())};
    }

    value operator[](const char *key) const
    {
        return operator[](std::string_view{key});
    }

    value operator[](int i) const
    {
        return operator[](static_cast<std::size_t>(i));
    }

    value operator[](std::size_t i) const
    {
        return {m_doc, yyjson_arr_get(m_val, i)};
    }

    template <typename T>
    requires json_convertible<T>
    operator T() const
    {
        T t{};
        [[maybe_unused]] auto _ = parseValue(m_val, t);
        return t;
    }

    // explicit conv
    template <typename T>
    T get() const
    {
        T t = operator T();
        return t;
    }

    // explicit conv, with error handling
    template <typename T>
    std::expected<T, std::error_code> try_get() const
    {
        T t{};
        auto res = parseValue(m_val, t);
        if (res == jacinth::errc::ok)
            return t;
        return std::unexpected(jacinth::make_error_code(res));
    }

    // Try to get a value/index, or null if not
    template <typename Key>
    value get(Key key) const
    {
        value v = operator[](key);
        return !v.is_null() ? v : value{m_doc, nullptr};
    }

    // Try to get a value/index, or the default if not
    template <typename T, typename Key>
    T get(Key key, T default_value = {}) const
    {
        value v = operator[](key);
        return !v.is_null() ? T(v) : default_value;
    }

    // Try to get a value/index, return an error if not
    template <typename Key>
    std::expected<value, std::error_code> try_get(Key key) const
    {
        return operator[](key).template try_get<value>();
    }

    // Try to get a value/index, return an error if not
    template <typename T, typename Key>
    std::expected<T, std::error_code> try_get(Key key) const
    {
        return operator[](key).template try_get<T>();
    }

    // TODO: byte size? obj size?
    std::size_t size() const
    {
        return yyjson_arr_size(m_val);
    }

    bool is_object() const
    {
        return yyjson_is_obj(m_val);
    }

    bool is_array() const
    {
        return yyjson_is_arr(m_val);
    }

    // TODO: try_dump?
    // dump this subtree as a JSON string
    std::string dump(write_opts opts = {}) const
    {
        std::string s;
        dump_to(s, opts);
        return s;
    }

    // non-allocating dump into an existing string
    void dump_to(std::string &s, write_opts opts = {}) const
    {
        std::size_t len = 0;
        char *buf = yyjson_val_write(m_val, opts.to_flags(), &len);
        s.assign(buf ? buf : "", buf ? len : 0);
        free(buf);
    }

    // iter
    iterable_view<const_object_iterator> as_object() const;
    iterable_view<const_array_iterator> as_array() const;
};

// Read-only JSON tree
class doc : public value {
public:
    // TODO: read_opts
    static doc read(std::string_view s, yyjson_read_flag flg = 0)
    {
        return doc{yyjson_read(s.data(), s.size(), flg)};
    }

    // read, with error handling
    static std::expected<doc, std::error_code> try_read(std::string_view s,
                                                        yyjson_read_flag flg = 0)
    {
        yyjson_read_err err;
        yyjson_doc *d = yyjson_read_opts(const_cast<char *>(s.data()), s.size(), flg, NULL, &err);
        if (!d)
            return std::unexpected(jacinth::make_error_code(jacinth::errc::invalid_json));
        return doc(d);
    }

    // try reading directly to parseable type
    template <typename T>
    static std::expected<T, std::error_code> try_read(std::string_view s, yyjson_read_flag flg = 0)
    {
        auto d = try_read(s, flg);
        if (!d)
            return std::unexpected(d.error());
        return d.value().template try_get<T>();
    }

    explicit doc(yyjson_doc *d) noexcept : value(d, d ? d->root : nullptr) {}

    doc(doc &&o) noexcept : value(std::exchange(o.m_doc, nullptr), std::exchange(o.m_val, nullptr))
    {
    }

    doc(doc const &) = delete;
    doc &operator=(doc const &) = delete;
    doc &operator=(doc &&o) noexcept
    {
        std::swap(static_cast<value &>(*this), static_cast<value &>(o));
        return *this;
    }

    yyjson_doc *raw() const
    {
        return m_doc;
    }

    value root() const
    {
        return *this;
    }

    ~doc()
    {
        yyjson_doc_free(m_doc);
    }
};

// A mutable JSON-ish value
class mutable_value {
protected:
    yyjson_mut_doc *m_doc;
    yyjson_mut_val *m_val;

    mutable_value() = default;

private:
    // force the value to be an array/obj if need be
    void ensure_object()
    {
        if (yyjson_mut_is_null(m_val))
            yyjson_mut_set_obj(m_val);
    }

    void ensure_array()
    {
        if (yyjson_mut_is_null(m_val))
            yyjson_mut_set_arr(m_val);
    }

public:
    mutable_value(yyjson_mut_doc *d, yyjson_mut_val *v) : m_doc(d), m_val(v) {}

    // validity checks
    bool is_null() const
    {
        return !m_val || yyjson_mut_is_null(m_val);
    }

    // assignment
    template <typename T>
    mutable_value &operator=(const T &v)
    {
        [[maybe_unused]] auto _ = writeValue(m_doc, m_val, v);
        return *this;
    }

    // initializer-list assignment
    template <typename T>
    mutable_value &operator=(std::initializer_list<T> values)
    {
        yyjson_mut_set_arr(m_val);
        for (const auto &item : values) {
            auto *elem = yyjson_mut_null(m_doc);
            yyjson_mut_arr_append(m_val, elem);
            if (writeValue(m_doc, elem, item) != jacinth::errc::ok)
                break;
        }
        return *this;
    }

    template <typename T>
    mutable_value &assign(const T &v)
    {
        return *this = v;
    }

    template <typename T>
    requires json_convertible<T>
    operator T() const
    {
        T t{};
        // parseValue can't take mutable values...
        auto *idoc = yyjson_mut_val_imut_copy(m_val, nullptr);
        [[maybe_unused]] auto _ = parseValue(idoc->root, t);
        yyjson_doc_free(idoc);
        return t;
    }

    // explicit conv
    template <typename T>
    T get() const
    {
        T t = operator T();
        return t;
    }

    // explicit conv, with error handling
    template <typename T>
    std::expected<T, std::error_code> try_get() const
    {
        if (!m_val)
            return std::unexpected(jacinth::make_error_code(jacinth::errc::missing_value));

        T t{};
        auto *idoc = yyjson_mut_val_imut_copy(m_val, nullptr);
        auto res = parseValue(idoc->root, t);
        yyjson_doc_free(idoc);
        if (res == jacinth::errc::ok)
            return t;

        return std::unexpected(jacinth::make_error_code(res));
    }

    // Try to get a value/index, or null if not
    template <typename Key>
    mutable_value get(Key key)
    {
        mutable_value v = operator[](key);
        return !v.is_null() ? v : mutable_value{m_doc, nullptr};
    }

    // Try to get a value/index, or the default if not
    template <typename T, typename Key>
    T get(Key key, T default_value = {})
    {
        mutable_value v = operator[](key);
        return !v.is_null() ? T(v) : default_value;
    }

    // Try to get a value/index, return an error if not
    template <typename Key>
    std::expected<mutable_value, std::error_code> try_get(Key key)
    {
        return operator[](key).template try_get<mutable_value>();
    }

    // Try to get a value/index, return an error if not
    template <typename T, typename Key>
    std::expected<T, std::error_code> try_get(Key key)
    {
        return operator[](key).template try_get<T>();
    }

    // TODO: append/prepend/insert, set?

    // mutation
    bool remove(std::string_view key)
    {
        ensure_object();
        return yyjson_mut_obj_remove_keyn(m_val, key.data(), key.size());
    }

    // TODO: ranges?
    bool remove(std::size_t i)
    {
        ensure_array();
        return yyjson_mut_arr_remove(m_val, i);
    }

    bool remove(std::size_t pos, std::size_t n)
    {
        ensure_array();
        return yyjson_mut_arr_remove_range(m_val, pos, n);
    }

    bool clear()
    {
        return yyjson_mut_is_obj(m_val) ? yyjson_mut_obj_clear(m_val) : yyjson_mut_arr_clear(m_val);
    }

    mutable_value pop_back()
    {
        return {m_doc, yyjson_mut_arr_remove_last(m_val)};
    }

    mutable_value pop_front()
    {
        return {m_doc, yyjson_mut_arr_remove_first(m_val)};
    }

    // indexing
    mutable_value operator[](std::string_view key)
    {
        // this is only valid for objects
        ensure_object();
        auto *v = yyjson_mut_obj_getn(m_val, key.data(), key.size());

        // if the value doesn't exist, add it to the object
        if (!v) {
            v = yyjson_mut_null(m_doc);
            auto *c_key = yyjson_mut_strncpy(m_doc, key.data(), key.size());
            yyjson_mut_obj_put(m_val, c_key, v);
        }

        return {m_doc, v};
    }

    mutable_value operator[](const char *key)
    {
        return operator[](std::string_view{key});
    }

    mutable_value operator[](int i)
    {
        return operator[](static_cast<std::size_t>(i));
    }

    mutable_value operator[](std::size_t i)
    {
        // this is only valid for arrays
        ensure_array();
        auto *v = yyjson_mut_arr_get(m_val, i);
        size_t n = yyjson_mut_arr_size(m_val);

        // fill the array with zeroes until it's big enough
        // technically this is an anti-pattern
        if (!v)
            while (n <= i) {
                v = yyjson_mut_null(m_doc);
                yyjson_mut_arr_append(m_val, v);
                ++n;
            }

        return {m_doc, v};
    }

    std::size_t size() const
    {
        return yyjson_mut_arr_size(m_val);
    }

    bool is_object() const
    {
        return yyjson_mut_is_obj(m_val);
    }

    bool is_array() const
    {
        return yyjson_mut_is_arr(m_val);
    }

    // dump this subtree as a JSON string
    std::string dump(write_opts opts = {}) const
    {
        std::string s;
        dump_to(s, opts);
        return s;
    }

    // non-allocating dump into an existing string
    void dump_to(std::string &s, write_opts opts = {}) const
    {
        std::size_t len = 0;
        char *buf = m_val ? yyjson_mut_val_write(m_val, opts.to_flags(), &len) : nullptr;
        s.assign(buf ? buf : "", buf ? len : 0);
        free(buf);
    }

    // iter
    iterable_view<mut_object_iterator> as_object();
    iterable_view<mut_array_iterator> as_array();
};

// Read-write JSON tree
class json : public mutable_value {
    using base = mutable_value;

    explicit json(yyjson_mut_doc *d) noexcept : base(d, d ? d->root : nullptr) {}

public:
    using base::operator=;

    json() : base(yyjson_mut_doc_new(nullptr), nullptr)
    {
        auto root = yyjson_mut_null(m_doc);
        yyjson_mut_doc_set_root(m_doc, root);
        m_val = root;
    }

    explicit json(doc const &d) : json(yyjson_doc_mut_copy(d.raw(), nullptr)) {}

    template <typename T>
    json(const T &v) : json()
    {
        base::operator=(v);
    }

    // initializer-list construction
    template <typename T>
    json(std::initializer_list<T> values) : json()
    {
        base::operator=(values);
    }

    ~json()
    {
        yyjson_mut_doc_free(m_doc);
    }

    json(json &&o) noexcept : base(std::exchange(o.m_doc, nullptr), std::exchange(o.m_val, nullptr))
    {
    }
    json(json const &o) : json(yyjson_mut_doc_mut_copy(o.m_doc, nullptr)) {}
    json &operator=(json &&o) noexcept
    {
        std::swap(static_cast<base &>(*this), static_cast<base &>(o));
        return *this;
    }

    mutable_value root() const
    {
        return *this;
    }

    // un-hide the inherited member dump/dump_to, shadowed by the statics below
    using base::dump;
    using base::dump_to;

    // TODO: non-alloc methods

    // i/o
    static json read(std::string_view s, yyjson_read_flag flg = 0)
    {
        yyjson_doc *d = yyjson_read(s.data(), s.size(), flg);
        yyjson_mut_doc *m = yyjson_doc_mut_copy(d, nullptr);
        yyjson_doc_free(d);
        return json(m);
    }

    // read, with error handling
    static std::expected<json, std::error_code> try_read(std::string_view s,
                                                         yyjson_read_flag flg = 0)
    {
        yyjson_read_err err;
        yyjson_doc *d = yyjson_read_opts(const_cast<char *>(s.data()), s.size(), flg, NULL, &err);
        if (!d)
            return std::unexpected(jacinth::make_error_code(jacinth::errc::invalid_json));

        yyjson_mut_doc *m = yyjson_doc_mut_copy(d, nullptr);
        yyjson_doc_free(d);
        return json(m);
    }

    // try reading directly to parseable type
    template <typename T>
    static std::expected<T, std::error_code> try_read(std::string_view s, yyjson_read_flag flg = 0)
    {
        auto d = try_read(s, flg);
        if (!d)
            return std::unexpected(d.error());

        return d.value().template try_get<T>();
    }

    // Write directly from an object (non-allocating)
    template <typename T>
    static void dump_to(const T &value, std::string &s, write_opts opts = {})
    {
        json json = value;
        json.dump_to(s, opts);
    }

    // Write directly from an object
    template <typename T>
    static std::string dump(const T &value, write_opts opts = {})
    {
        json json = value;
        return json.dump(opts);
    }

    // dump from an existing mutable node
    static std::string dump(const mutable_value &value, write_opts opts = {})
    {
        std::string s;
        dump_to(value, s, opts);
        return s;
    }

    static void dump_to(const mutable_value &value, std::string &s, write_opts opts = {})
    {
        value.dump_to(s, opts);
    }

    // dump an existing read-only doc
    static std::string dump(const doc &value, write_opts opts = {})
    {
        std::string s;
        dump_to(value, s, opts);
        return s;
    }

    static void dump_to(const doc &value, std::string &s, write_opts opts = {})
    {
        value.dump_to(s, opts);
    }

    // dump an existing immutable value
    static std::string dump(const value &node, write_opts opts = {})
    {
        std::string s;
        dump_to(node, s, opts);
        return s;
    }

    static void dump_to(const value &node, std::string &s, write_opts opts = {})
    {
        node.dump_to(s, opts);
    }

    // freeze this into a read-only doc
    doc freeze() const
    {
        yyjson_doc *d = yyjson_mut_doc_imut_copy(m_doc, nullptr);
        return doc(d);
    }
};

// iterators
// TODO: find less duped solution
class const_array_iterator {
    yyjson_doc *m_doc{};
    yyjson_arr_iter m_iter{};
    yyjson_val *m_val{};
    std::size_t m_idx{0};

public:
    const_array_iterator() = default;
    const_array_iterator(yyjson_doc *doc, yyjson_val *arr) : m_doc(doc)
    {
        if (arr && yyjson_is_arr(arr)) {
            yyjson_arr_iter_init(arr, &m_iter);
            m_val = yyjson_arr_iter_next(&m_iter);
        }
    }

    bool operator!=(const const_array_iterator &other) const
    {
        return m_val != other.m_val;
    }
    const_array_iterator &operator++()
    {
        m_val = yyjson_arr_iter_next(&m_iter);
        ++m_idx;
        return *this;
    }

    value operator*() const
    {
        return {m_doc, m_val};
    }
};

class const_object_iterator {
    yyjson_doc *m_doc{};
    yyjson_obj_iter m_iter{};
    yyjson_val *m_key{};

public:
    const_object_iterator() = default;
    const_object_iterator(yyjson_doc *doc, yyjson_val *obj) : m_doc(doc)
    {
        if (obj && yyjson_is_obj(obj)) {
            yyjson_obj_iter_init(obj, &m_iter);
            m_key = yyjson_obj_iter_next(&m_iter);
        }
    }

    bool operator!=(const const_object_iterator &other) const
    {
        return m_key != other.m_key;
    }

    const_object_iterator &operator++()
    {
        m_key = yyjson_obj_iter_next(&m_iter);
        return *this;
    }

    std::pair<std::string_view, value> operator*() const
    {
        return {std::string_view(yyjson_get_str(m_key), yyjson_get_len(m_key)),
                value(m_doc, yyjson_obj_iter_get_val(m_key))};
    }
};

class mut_array_iterator {
    yyjson_mut_doc *m_doc{};
    yyjson_mut_arr_iter m_iter{};
    yyjson_mut_val *m_val{};
    std::size_t m_idx{0};

public:
    mut_array_iterator() = default;
    mut_array_iterator(yyjson_mut_doc *doc, yyjson_mut_val *arr) : m_doc(doc)
    {
        if (arr && yyjson_mut_is_arr(arr)) {
            yyjson_mut_arr_iter_init(arr, &m_iter);
            m_val = yyjson_mut_arr_iter_next(&m_iter);
        }
    }

    bool operator!=(const mut_array_iterator &other) const
    {
        return m_val != other.m_val;
    }

    mut_array_iterator &operator++()
    {
        m_val = yyjson_mut_arr_iter_next(&m_iter);
        ++m_idx;
        return *this;
    }

    mutable_value operator*() const
    {
        return {m_doc, m_val};
    }
};

class mut_object_iterator {
    yyjson_mut_doc *m_doc{};
    yyjson_mut_obj_iter m_iter{};
    yyjson_mut_val *m_key{};

public:
    mut_object_iterator() = default;
    mut_object_iterator(yyjson_mut_doc *doc, yyjson_mut_val *obj) : m_doc(doc)
    {
        if (obj && yyjson_mut_is_obj(obj)) {
            yyjson_mut_obj_iter_init(obj, &m_iter);
            m_key = yyjson_mut_obj_iter_next(&m_iter);
        }
    }

    bool operator!=(const mut_object_iterator &other) const
    {
        return m_key != other.m_key;
    }

    mut_object_iterator &operator++()
    {
        m_key = yyjson_mut_obj_iter_next(&m_iter);
        return *this;
    }

    std::pair<std::string_view, mutable_value> operator*() const
    {
        return {std::string_view(yyjson_mut_get_str(m_key), yyjson_mut_get_len(m_key)),
                mutable_value(m_doc, yyjson_mut_obj_iter_get_val(m_key))};
    }
};

// iter defs
inline iterable_view<const_object_iterator> value::as_object() const
{
    return {const_object_iterator(m_doc, m_val), const_object_iterator()};
}

inline iterable_view<const_array_iterator> value::as_array() const
{
    return {const_array_iterator(m_doc, m_val), const_array_iterator()};
}

inline iterable_view<mut_object_iterator> mutable_value::as_object()
{
    return {mut_object_iterator(m_doc, m_val), mut_object_iterator()};
}

inline iterable_view<mut_array_iterator> mutable_value::as_array()
{
    return {mut_array_iterator(m_doc, m_val), mut_array_iterator()};
}

} // namespace jacinth

template <>
struct std::is_error_code_enum<jacinth::errc> : std::true_type {};

// vector specializations
// TODO: generalize this onto all vector/array-likes
template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

// array specializations
template <typename T>
struct is_array : std::false_type {};

template <typename T, std::size_t N>
struct is_array<std::array<T, N>> : std::true_type {};

template <typename T>
inline constexpr bool is_array_v = is_array<T>::value;

// span specializations
template <typename T>
struct is_span : std::false_type {};

template <typename T, std::size_t E>
struct is_span<std::span<T, E>> : std::true_type {};

template <typename T>
inline constexpr bool is_span_v = is_span<T>::value;

// optional specializations
template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_optional_v = is_optional<T>::value;

// map specializations
template <typename T>
struct is_map : std::false_type {};

template <typename Key, typename Value, typename Compare, typename Alloc>
struct is_map<std::map<Key, Value, Compare, Alloc>> : std::true_type {};

template <typename T>
inline constexpr bool is_map_v = is_map<T>::value;

// used for static assert
template <typename...>
inline constexpr bool always_false = false;

// impls
template <typename T>
[[nodiscard]] jacinth::errc parseValue(yyjson_val *val, T &field)
{
    using FieldType = std::decay_t<T>;

    // optionals
    if constexpr (is_optional_v<FieldType>) {
        if (val && !yyjson_is_null(val)) {
            typename FieldType::value_type item;
            auto res = parseValue(val, item);
            field = std::move(item);
            return res;
        } else
            field.reset();

        return {};
    }

    if (!val)
        return jacinth::errc::missing_value;

    if constexpr (std::is_same_v<FieldType, std::string>) {
        if (!yyjson_is_str(val))
            return jacinth::errc::type_mismatch;
        field.assign(yyjson_get_str(val), yyjson_get_len(val));
    } else if constexpr (std::is_same_v<FieldType, std::string_view>) {
        if (!yyjson_is_str(val))
            return jacinth::errc::type_mismatch;
        field = {yyjson_get_str(val), yyjson_get_len(val)};
    } else if constexpr (std::is_floating_point_v<FieldType>) {
        if (!yyjson_is_num(val))
            return jacinth::errc::type_mismatch;
        field = FieldType(yyjson_get_num(val));
    } else if constexpr (std::is_same_v<FieldType, bool>) {
        if (!yyjson_is_bool(val))
            return jacinth::errc::type_mismatch;
        field = yyjson_get_bool(val);
    } else if constexpr (std::is_integral_v<FieldType>) {
        if (yyjson_is_uint(val))
            field = FieldType(yyjson_get_uint(val));
        else if (yyjson_is_int(val))
            field = FieldType(yyjson_get_int(val));
        else
            return jacinth::errc::type_mismatch;
    }
    // enums are serialized as their underlying type
    else if constexpr (std::is_enum_v<FieldType>) {
        using UnderlyingType = std::underlying_type_t<FieldType>;
        using WideType = std::make_unsigned_t<UnderlyingType>;
        if (yyjson_is_uint(val))
            field = static_cast<FieldType>(WideType(yyjson_get_uint(val)));
        else if (yyjson_is_int(val))
            field = static_cast<FieldType>(WideType(static_cast<WideType>(yyjson_get_int(val))));
        else
            return jacinth::errc::type_mismatch;
    }
    // vectors
    else if constexpr (is_vector_v<FieldType>) {
        if (!yyjson_is_arr(val))
            return jacinth::errc::type_mismatch;
        field.resize(yyjson_arr_size(val));

        size_t idx, max;
        yyjson_val *elem;
        yyjson_arr_foreach(val, idx, max, elem)
        {
            if (auto res = parseValue(elem, field[idx]); res != jacinth::errc::ok)
                return res;
        }
    }
    // fixed-size std::array
    else if constexpr (is_array_v<FieldType>) {
        if (!yyjson_is_arr(val))
            return jacinth::errc::type_mismatch;
        static constexpr const size_t N = std::tuple_size_v<FieldType>;

        size_t idx, max;
        yyjson_val *elem;
        yyjson_arr_foreach(val, idx, max, elem)
        {
            if (idx >= N)
                break;
            if (auto res = parseValue(elem, field[idx]); res != jacinth::errc::ok)
                return res;
        }
    }
    // static-extent std::span
    else if constexpr (is_span_v<FieldType>) {
        if constexpr (FieldType::extent == std::dynamic_extent) {
            static_assert(always_false<FieldType>,
                          "jacinth: cannot parse into a dynamically-sized std::span");
        } else if (!yyjson_is_arr(val))
            return jacinth::errc::type_mismatch;
        static constexpr const size_t N = FieldType::extent;

        size_t idx, max;
        yyjson_val *elem;
        yyjson_arr_foreach(val, idx, max, elem)
        {
            if (idx >= N)
                break;
            if (auto res = parseValue(elem, field[idx]); res != jacinth::errc::ok)
                return res;
        }
    }
    // maps
    else if constexpr (is_map_v<FieldType>) {
        // TODO(crueter): Really need better err handling
        if (!yyjson_is_obj(val))
            return jacinth::errc::type_mismatch;

        using MappedType = typename FieldType::mapped_type;
        field.clear();

        yyjson_obj_iter iter;
        yyjson_obj_iter_init(val, &iter);
        yyjson_val *key;
        while ((key = yyjson_obj_iter_next(&iter))) {
            auto *sub = yyjson_obj_iter_get_val(key);
            MappedType item;
            if (auto res = parseValue(sub, item); res != jacinth::errc::ok)
                return res;

            field.emplace(std::string_view(yyjson_get_str(key), yyjson_get_len(key)),
                          std::move(item));
        }
    }
    // from_json ADL
    else if constexpr (requires { from_json(jacinth::value{nullptr, nullptr}, field); }) {
        from_json(jacinth::value{nullptr, val}, field);
    }
    // nested structs, etc.
    else if constexpr (std::is_aggregate_v<FieldType>) {
        if (!yyjson_is_obj(val))
            return jacinth::errc::type_mismatch;

        jacinth::errc result{};
#ifdef JACINTH_USE_REFLECTION
        template for (constexpr auto f :
                      std::define_static_array(std::meta::nonstatic_data_members_of(
                          ^^T, std::meta::access_context::current())))
        {
            if (result != jacinth::errc::ok)
                return result;

            constexpr auto name = std::meta::identifier_of(f);
            yyjson_val *sub = yyjson_obj_getn(val, name.data(), name.size());
            using Sub = std::decay_t<decltype(field.[:f:])>;

            if constexpr (is_optional_v<Sub>) {
                if (sub)
                    result = parseValue(sub, field.[:f:]);
                else
                    field.[:f:].reset();
            } else if (!sub) {
                result = jacinth::errc::missing_value;
            } else {
                result = parseValue(sub, field.[:f:]);
            }
        }
#else
        boost::pfr::for_each_field_with_name(
            field, [val, &result](std::string_view name, auto &sub_field) {
                if (result != jacinth::errc::ok)
                    return;

                yyjson_val *sub = yyjson_obj_getn(val, name.data(), name.size());
                using Sub = std::decay_t<decltype(sub_field)>;

                if constexpr (is_optional_v<Sub>) {
                    if (sub)
                        result = parseValue(sub, sub_field);
                    else
                        sub_field.reset();
                } else if (!sub) {
                    result = jacinth::errc::missing_value;
                } else {
                    result = parseValue(sub, sub_field);
                }
            });
#endif
        return result;
    } else {
        return jacinth::errc::type_mismatch;
    }

    return {};
}

// TODO: handle variant?
template <typename T>
jacinth::errc writeValue(yyjson_mut_doc *doc, yyjson_mut_val *val, const T &field)
{
    if (!val)
        return jacinth::errc::missing_value;

    using FieldType = std::decay_t<T>;

    // copy strings into the doc pool
    if constexpr (std::is_same_v<FieldType, std::string> ||
                  std::is_same_v<FieldType, std::string_view>) {
        if (yyjson_mut_val *str = yyjson_mut_strncpy(doc, field.data(), field.size()); str) {
            val->tag = str->tag;
            val->uni = str->uni;
        } else {
            return jacinth::errc::write_error;
        }
    } else if constexpr (std::is_same_v<FieldType, char *> ||
                         std::is_same_v<FieldType, const char *>) {
        if (yyjson_mut_val *str = yyjson_mut_strcpy(doc, field); str) {
            val->tag = str->tag;
            val->uni = str->uni;
        } else {
            return jacinth::errc::write_error;
        }
    } else if constexpr (std::is_floating_point_v<FieldType>) {
        if constexpr (std::is_same_v<FieldType, float>)
            yyjson_mut_set_float(val, float(field));
        else
            yyjson_mut_set_double(val, double(field));
    } else if constexpr (std::is_same_v<FieldType, bool>) {
        yyjson_mut_set_bool(val, field);
    } else if constexpr (std::is_integral_v<FieldType>) {
        if constexpr (std::is_unsigned_v<FieldType>)
            yyjson_mut_set_uint(val, uint64_t(field));
        else
            yyjson_mut_set_int(val, int64_t(field));
    }
    // enums are serialized as their underlying type
    else if constexpr (std::is_enum_v<FieldType>) {
        using UnderlyingType = std::underlying_type_t<FieldType>;
        if constexpr (std::is_unsigned_v<UnderlyingType>)
            yyjson_mut_set_uint(val, uint64_t(static_cast<UnderlyingType>(field)));
        else
            yyjson_mut_set_int(val, int64_t(static_cast<UnderlyingType>(field)));
    }
    // vectors, arrays, spans
    else if constexpr (is_vector_v<FieldType> || is_array_v<FieldType> || is_span_v<FieldType>) {
        yyjson_mut_set_arr(val);
        for (const auto &item : field) {
            auto *elem = yyjson_mut_null(doc);
            if (!yyjson_mut_arr_append(val, elem))
                return jacinth::errc::write_error;

            if (auto res = writeValue(doc, elem, item); res != jacinth::errc::ok)
                return res;
        }
    }
    // optionals
    else if constexpr (is_optional_v<FieldType>) {
        if (field.has_value()) {
            if (auto res = writeValue(doc, val, *field); res != jacinth::errc::ok)
                return res;
        } else
            yyjson_mut_set_null(val);
    }
    // maps
    else if constexpr (is_map_v<FieldType>) {
        yyjson_mut_set_obj(val);
        for (const auto &[k, v] : field) {
            auto *sub = yyjson_mut_null(doc);
            auto *key = yyjson_mut_strncpy(doc, k.data(), k.size());
            if (!yyjson_mut_obj_put(val, key, sub))
                return jacinth::errc::write_error;

            if (auto res = writeValue(doc, sub, v); res != jacinth::errc::ok)
                return res;
        }
    }
    // to_json ADL
    else if constexpr (requires { to_json(jacinth::mutable_value{nullptr, nullptr}, field); }) {
        to_json(jacinth::mutable_value{doc, val}, field);
    }
    // nested structs, etc
    else if constexpr (std::is_aggregate_v<FieldType>) {
        yyjson_mut_set_obj(val);
        jacinth::errc result{};
#ifdef JACINTH_USE_REFLECTION
        template for (constexpr auto f :
                      std::define_static_array(std::meta::nonstatic_data_members_of(
                          ^^T, std::meta::access_context::current())))
        {
            if (result != jacinth::errc::ok)
                return result;

            constexpr auto name = std::meta::identifier_of(f);
            auto *sub = yyjson_mut_null(doc);
            auto *key = yyjson_mut_strn(doc, name.data(), name.size());

            // noesc speeds up writes
            if (unsafe_yyjson_is_str_noesc(name.data(), name.size()))
                unsafe_yyjson_set_tag(key, YYJSON_TYPE_STR, YYJSON_SUBTYPE_NOESC, name.size());

            if (!yyjson_mut_obj_add(val, key, sub))
                result = jacinth::errc::write_error;
            else if (auto res = writeValue(doc, sub, field.[:f:]); res != jacinth::errc::ok)
                result = res;
        }
#else
        boost::pfr::for_each_field_with_name(field, [&](std::string_view name, auto &sub_field) {
            if (result != jacinth::errc::ok)
                return;

            auto *sub = yyjson_mut_null(doc);
            auto *key = yyjson_mut_strn(doc, name.data(), name.size());

            // noesc speeds up writes
            if (unsafe_yyjson_is_str_noesc(name.data(), name.size()))
                unsafe_yyjson_set_tag(key, YYJSON_TYPE_STR, YYJSON_SUBTYPE_NOESC, name.size());

            if (!yyjson_mut_obj_add(val, key, sub))
                result = jacinth::errc::write_error;
            else
                result = writeValue(doc, sub, sub_field);
        });
#endif
        return result;
    } else {
        static_assert(always_false<FieldType>, "jacinth: unsupported type for JSON serialization");
    }

    return {};
}
