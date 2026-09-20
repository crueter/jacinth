module;

#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <span>
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

template <typename T>
void parseValue(yyjson_val *val, T &field);
template <typename T>
void writeValue(yyjson_mut_doc *doc, yyjson_mut_val *val, const T &field);

export namespace jacinth
{

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
    bool pretty;
    bool escapeUnicode;
    bool escapeSlashes;
    bool allowInfAndNan;
    bool writeInfAndNanAsNull;
    bool allowInvalidUnicode;
    bool prettyTwoSpaces;
    bool endingNewline;

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

// TODO: is_<type> funcs

// An immutable JSON-ish value
class value {
    yyjson_doc *m_doc;
    yyjson_val *m_val;

public:
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

    value operator[](std::size_t i) const
    {
        return {m_doc, yyjson_arr_get(m_val, i)};
    }

    template <typename T>
    operator T() const
    {
        T t;
        parseValue(m_val, t);
        return t;
    }

    // explicit conv
    template <typename T>
    T as() const
    {
        T t = operator T();
        return t;
    }

    // Try to get a value, or the default if not
    template <typename T>
    T get(std::string_view key, T default_value = {}) const
    {
        value v = operator[](key);
        return v ? T(v) : default_value;
    }

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
    yyjson_doc *m_doc;

public:
    static doc read(std::string_view s)
    {
        return doc{yyjson_read(s.data(), s.size(), 0)};
    }

    explicit doc(yyjson_doc *d) noexcept : value(d, d ? d->root : nullptr), m_doc(d) {}

    yyjson_doc *raw() const {
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
    yyjson_mut_doc *m_doc;
    yyjson_mut_val *m_val;

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

    // assignment
    template <typename T>
    mutable_value &operator=(const T &v)
    {
        writeValue(m_doc, m_val, v);
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
            writeValue(m_doc, elem, item);
        }
        return *this;
    }

    template <typename T>
    mutable_value &assign(const T &v)
    {
        return *this = v;
    }

    template <typename T>
    operator T() const
    {
        T t;
        // parseValue can't take mutable values...
        auto *idoc = yyjson_mut_val_imut_copy(m_val, nullptr);
        parseValue(idoc->root, t);
        return t;
    }

    // explicit conv
    template <typename T>
    T as() const
    {
        T t = operator T();
        return t;
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

    mutable_value operator[](std::size_t i)
    {
        // this is only valid for arrays
        ensure_array();
        auto *v = yyjson_mut_arr_get(m_val, i);
        size_t n = yyjson_mut_arr_size(m_val);

        // fill the array with zeroes until it's big enough
        // technically this is an anti-pattern
        while (!v && n <= i) {
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
class json {
    yyjson_mut_doc *m_doc;

    explicit json(yyjson_mut_doc *d) noexcept : m_doc(d) {}

public:
    json() : m_doc(yyjson_mut_doc_new(nullptr))
    {
        yyjson_mut_doc_set_root(m_doc, yyjson_mut_null(m_doc));
    }

    explicit json(doc const &d) : json(yyjson_doc_mut_copy(d.raw(), nullptr)) {}

    template <typename T>
    json(const T &v) : json()
    {
        operator=(v);
    }

    // initializer-list construction
    template <typename T>
    json(std::initializer_list<T> values) : json()
    {
        operator=(values);
    }

    ~json()
    {
        yyjson_mut_doc_free(m_doc);
    }

    json(json &&o) noexcept : m_doc(std::exchange(o.m_doc, nullptr)) {}
    json(json const &o) : json(yyjson_mut_doc_mut_copy(o.m_doc, nullptr)) {}
    json &operator=(json &&o) noexcept
    {
        std::swap(m_doc, o.m_doc);
        return *this;
    }

    mutable_value root() const
    {
        return {m_doc, m_doc->root};
    }

    // interpret the root as type T
    template <typename T>
    operator T() const
    {
        return root().template as<T>();
    }

    // explicit conv
    template <typename T>
    T as() const
    {
        T t = operator T();
        return t;
    }

    // i/o
    static json read(std::string_view s, yyjson_read_flag flg = 0)
    {
        yyjson_doc *d = yyjson_read(s.data(), s.size(), flg);
        yyjson_mut_doc *m = yyjson_doc_mut_copy(d, nullptr);
        yyjson_doc_free(d);
        return json(m);
    }

    // allocating dump
    std::string dump(write_opts opts = {})
    {
        std::string s;
        dump_to(s, opts);
        return s;
    }

    // non-allocating dump
    void dump_to(std::string &s, write_opts opts = {})
    {
        auto *root = m_doc->root;
        std::size_t len = 0;
        char *buf = root ? yyjson_mut_val_write(root, opts.to_flags(), &len) : nullptr;
        s.assign(buf ? buf : "", buf ? len : 0);
        free(buf);
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

    static void dump_to(const mutable_value &value, std::string &s,
                        write_opts opts = {})
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

    static void dump_to(const doc &value, std::string &s,
                        write_opts opts = {})
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

    static void dump_to(const value &node, std::string &s,
                        write_opts opts = {})
    {
        node.dump_to(s, opts);
    }

    // freeze this into a read-only doc
    doc freeze() const
    {
        yyjson_doc *d = yyjson_mut_doc_imut_copy(m_doc, nullptr);
        return doc(d);
    }

    // access/mut ops
    template <typename T>
    json &operator=(const T &v)
    {
        writeValue(m_doc, m_doc->root, v);
        return *this;
    }

    // initializer-list assignment
    template <typename T>
    json &operator=(std::initializer_list<T> values)
    {
        yyjson_mut_set_arr(m_doc->root);
        for (const auto &item : values) {
            auto *elem = yyjson_mut_null(m_doc);
            yyjson_mut_arr_append(m_doc->root, elem);
            writeValue(m_doc, elem, item);
        }
        return *this;
    }

    mutable_value operator[](std::string_view key)
    {
        return root()[key];
    }

    mutable_value operator[](const char *key)
    {
        return root()[key];
    }

    mutable_value operator[](std::size_t i)
    {
        return root()[i];
    }

    bool remove(std::string_view key)
    {
        return root().remove(key);
    }

    // TODO: ranges?
    bool remove(std::size_t i)
    {
        return root().remove(i);
    }

    bool remove(std::size_t pos, std::size_t n)
    {
        return root().remove(pos, n);
    }

    bool clear()
    {
        return root().clear();
    }

    mutable_value pop_back()
    {
        return root().pop_back();
    }

    mutable_value pop_front()
    {
        return root().pop_front();
    }

    // root obj testers
    bool is_object() const
    {
        return yyjson_mut_is_obj(m_doc->root);
    }

    bool is_array() const
    {
        return yyjson_mut_is_arr(m_doc->root);
    }

    // iter
    iterable_view<mut_object_iterator> as_object();
    iterable_view<mut_array_iterator> as_array();
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

inline iterable_view<mut_object_iterator> json::as_object()
{
    return root().as_object();
}
inline iterable_view<mut_array_iterator> json::as_array()
{
    return root().as_array();
}

} // namespace jacinth

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
void parseValue(yyjson_val *val, T &field)
{
    if (!val)
        return;

    using FieldType = std::decay_t<T>;

    if constexpr (std::is_same_v<FieldType, std::string>) {
        if (yyjson_is_str(val))
            field.assign(yyjson_get_str(val), yyjson_get_len(val));
    } else if constexpr (std::is_same_v<FieldType, std::string_view>) {
        if (yyjson_is_str(val))
            field = {yyjson_get_str(val), yyjson_get_len(val)};
    } else if constexpr (std::is_floating_point_v<FieldType>) {
        if (yyjson_is_num(val))
            field = FieldType(yyjson_get_num(val));
    } else if constexpr (std::is_same_v<FieldType, bool>) {
        if (yyjson_is_bool(val))
            field = yyjson_get_bool(val);
    } else if constexpr (std::is_integral_v<FieldType>) {
        if (yyjson_is_uint(val))
            field = FieldType(yyjson_get_uint(val));
        else if (yyjson_is_int(val))
            field = FieldType(yyjson_get_int(val));
    }
    // enums are serialized as their underlying type
    else if constexpr (std::is_enum_v<FieldType>) {
        using UnderlyingType = std::underlying_type_t<FieldType>;
        using WideType = std::make_unsigned_t<UnderlyingType>;
        if (yyjson_is_uint(val))
            field = static_cast<FieldType>(WideType(yyjson_get_uint(val)));
        else if (yyjson_is_int(val))
            field = static_cast<FieldType>(WideType(static_cast<WideType>(yyjson_get_int(val))));
    }
    // vectors
    else if constexpr (is_vector_v<FieldType>) {
        if (yyjson_is_arr(val)) {
            field.resize(yyjson_arr_size(val));

            size_t idx, max;
            yyjson_val *elem;
            yyjson_arr_foreach(val, idx, max, elem)
            {
                parseValue(elem, field[idx]);
            }
        }
    }
    // fixed-size std::array
    else if constexpr (is_array_v<FieldType>) {
        if (yyjson_is_arr(val)) {
            static constexpr const size_t N = std::tuple_size_v<FieldType>;

            size_t idx, max;
            yyjson_val *elem;
            yyjson_arr_foreach(val, idx, max, elem)
            {
                if (idx >= N)
                    break;
                parseValue(elem, field[idx]);
            }
        }
    }
    // static-extent std::span
    else if constexpr (is_span_v<FieldType>) {
        if constexpr (FieldType::extent == std::dynamic_extent) {
            static_assert(always_false<FieldType>,
                          "jacinth: cannot parse into a dynamically-sized std::span");
        } else if (yyjson_is_arr(val)) {
            static constexpr const size_t N = FieldType::extent;

            size_t idx, max;
            yyjson_val *elem;
            yyjson_arr_foreach(val, idx, max, elem)
            {
                if (idx >= N)
                    break;
                parseValue(elem, field[idx]);
            }
        }
    }
    // optionals
    else if constexpr (is_optional_v<FieldType>) {
        if (val && !yyjson_is_null(val)) {
            typename FieldType::value_type item;
            parseValue(val, item);
            field = std::move(item);
        } else
            field.reset();
    }
    // maps
    else if constexpr (is_map_v<FieldType>) {
        // TODO(crueter): Really need better err handling
        if (!yyjson_is_obj(val))
            return;
        using MappedType = typename FieldType::mapped_type;
        field.clear();

        yyjson_obj_iter iter;
        yyjson_obj_iter_init(val, &iter);
        yyjson_val *key;
        while ((key = yyjson_obj_iter_next(&iter))) {
            auto *sub = yyjson_obj_iter_get_val(key);
            MappedType item;
            parseValue(sub, item);
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
        if (yyjson_is_obj(val)) {
#ifdef JACINTH_USE_REFLECTION
            yyjson_obj_iter iter;
            yyjson_obj_iter_init(val, &iter);
            yyjson_val *key;
            while ((key = yyjson_obj_iter_next(&iter))) {
                template for (constexpr auto f :
                            std::define_static_array(std::meta::nonstatic_data_members_of(
                                ^^T, std::meta::access_context::current())))
                {
                    constexpr auto name = std::meta::identifier_of(f);
                    if (yyjson_get_len(key) == name.size() && std::memcmp(yyjson_get_str(key), name.data(), name.size()) == 0) {
                        parseValue(yyjson_obj_iter_get_val(key), field.[:f:]);
                        break;
                    }
                }
            }

#else
            boost::pfr::for_each_field_with_name(
                field, [val](std::string_view name, auto &sub_field) {
                    yyjson_val *sub = yyjson_obj_getn(val, name.data(), name.size());
                    parseValue(sub, sub_field);
                });
#endif
            }
    }
}

// TODO: handle variant?
template <typename T>
void writeValue(yyjson_mut_doc *doc, yyjson_mut_val *val, const T &field)
{
    if (!val)
        return;

    using FieldType = std::decay_t<T>;

    // copy strings into the doc pool
    if constexpr (std::is_same_v<FieldType, std::string> ||
                  std::is_same_v<FieldType, std::string_view>) {
        if (yyjson_mut_val *str = yyjson_mut_strncpy(doc, field.data(), field.size()); str) {
            val->tag = str->tag;
            val->uni = str->uni;
        }
    } else if constexpr (std::is_same_v<FieldType, char *> ||
                         std::is_same_v<FieldType, const char *>) {
        if (yyjson_mut_val *str = yyjson_mut_strcpy(doc, field); str) {
            val->tag = str->tag;
            val->uni = str->uni;
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
            yyjson_mut_arr_append(val, elem);
            writeValue(doc, elem, item);
        }
    }
    // optionals
    else if constexpr (is_optional_v<FieldType>) {
        if (field.has_value())
            writeValue(doc, val, *field);
        else
            yyjson_mut_set_null(val);
    }
    // maps
    else if constexpr (is_map_v<FieldType>) {
        yyjson_mut_set_obj(val);
        for (const auto &[k, v] : field) {
            auto *sub = yyjson_mut_null(doc);
            auto *key = yyjson_mut_strncpy(doc, k.data(), k.size());
            yyjson_mut_obj_put(val, key, sub);
            writeValue(doc, sub, v);
        }
    }
    // to_json ADL
    else if constexpr (requires { to_json(jacinth::mutable_value{nullptr, nullptr}, field); }) {
        to_json(jacinth::mutable_value{doc, val}, field);
    }
    // nested structs, etc
    else if constexpr (std::is_aggregate_v<FieldType>) {
        yyjson_mut_set_obj(val);
#ifdef JACINTH_USE_REFLECTION
        template for (constexpr auto f :
                      std::define_static_array(std::meta::nonstatic_data_members_of(
                          ^^T, std::meta::access_context::current())))
        {
            constexpr auto name = std::meta::identifier_of(f);
            auto *sub = yyjson_mut_null(doc);
            auto *key = yyjson_mut_strn(doc, name.data(), name.size());
            if (unsafe_yyjson_is_str_noesc(name.data(), name.size()))
                unsafe_yyjson_set_tag(key, YYJSON_TYPE_STR, YYJSON_SUBTYPE_NOESC, name.size());
            yyjson_mut_obj_add(val, key, sub);
            writeValue(doc, sub, field.[:f:]);
        }
#else
        boost::pfr::for_each_field_with_name(field, [&](std::string_view name, auto &sub_field) {
            auto *sub = yyjson_mut_null(doc);
            auto *key = yyjson_mut_strn(doc, name.data(), name.size());
            if (unsafe_yyjson_is_str_noesc(name.data(), name.size()))
                unsafe_yyjson_set_tag(key, YYJSON_TYPE_STR, YYJSON_SUBTYPE_NOESC, name.size());
            yyjson_mut_obj_add(val, key, sub);
            writeValue(doc, sub, sub_field);
        });
#endif
    } else {
        static_assert(always_false<FieldType>, "jacinth: unsupported type for JSON serialization");
    }
}
