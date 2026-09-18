module;

#include <cstdint>
#include <utility>
#include <vector>

#include "yyjson.h"
#include "yyjson/src/yyjson.h"

#ifdef JACINTH_USE_REFLECTION
#include <meta>
#else
#include "boost/pfr.hpp"
#endif

export module jacinth;

export namespace jacinth
{

// vector specializations
template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

template <typename...>
inline constexpr bool always_false = false;

template <typename T>
void parseValue(yyjson_val *val, T &field)
{
    if (!val)
        return;

    using FieldType = std::decay_t<T>;

    if constexpr (std::is_same_v<FieldType, std::string>) {
        if (yyjson_is_str(val))
            field.assign(yyjson_get_str(val), yyjson_get_len(val));
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
        // vectors
    } else if constexpr (is_vector_v<FieldType>) {
        if (yyjson_is_arr(val)) {
            using ElemType = typename FieldType::value_type;
            field.clear();
            field.reserve(yyjson_arr_size(val));

            size_t idx, max;
            yyjson_val *elem;
            yyjson_arr_foreach(val, idx, max, elem)
            {
                ElemType item{};
                parseValue(elem, item);
                field.push_back(std::move(item));
            }
        }
    }
    // nested structs, etc.
    else if constexpr (std::is_aggregate_v<FieldType>) {
        if (yyjson_is_obj(val))
#ifdef JACINTH_USE_REFLECTION
            template for (constexpr auto f :
                          std::define_static_array(std::meta::nonstatic_data_members_of(
                              ^^T, std::meta::access_context::current())))
            {
                constexpr auto name = std::meta::identifier_of(f);
                yyjson_val *sub = yyjson_obj_getn(val, name.data(), name.size());
                if (sub) {
                    parseValue(sub, field.[:f:]);
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

template <typename T>
void writeValue(yyjson_mut_doc *doc, yyjson_mut_val *val, const T &field)
{
    if (!val)
        return;

    using FieldType = std::decay_t<T>;

    if constexpr (std::is_same_v<FieldType, std::string> ||
                  std::is_same_v<FieldType, std::string_view>) {
        auto *str = yyjson_mut_strncpy(doc, field.data(), field.size());
        yyjson_mut_set_strn(val, yyjson_mut_get_str(str), yyjson_mut_get_len(str));
    } else if constexpr (std::is_same_v<FieldType, char *> ||
                         std::is_same_v<FieldType, const char *>) {
        auto *str = yyjson_mut_strcpy(doc, field);
        yyjson_mut_set_strn(val, yyjson_mut_get_str(str), yyjson_mut_get_len(str));
    } else if constexpr (std::is_floating_point_v<FieldType>) {
        yyjson_mut_set_double(val, double(field));
    } else if constexpr (std::is_same_v<FieldType, bool>) {
        yyjson_mut_set_bool(val, field);
    } else if constexpr (std::is_integral_v<FieldType>) {
        if constexpr (std::is_unsigned_v<FieldType>)
            yyjson_mut_set_uint(val, uint64_t(field));
        else
            yyjson_mut_set_int(val, int64_t(field));
        // vectors
    } else if constexpr (is_vector_v<FieldType>) {
        yyjson_mut_set_arr(val);
        for (const auto &item : field) {
            auto *elem = yyjson_mut_null(doc);
            yyjson_mut_arr_append(val, elem);
            writeValue(doc, elem, item);
        }
    } else if constexpr (std::is_aggregate_v<FieldType>) {
#ifdef JACINTH_USE_REFLECTION
        template for (constexpr auto f :
                      std::define_static_array(std::meta::nonstatic_data_members_of(
                          ^^T, std::meta::access_context::current())))
        {
            constexpr auto name = std::meta::identifier_of(f);
            auto *sub = yyjson_mut_null(doc);
            auto *key = yyjson_mut_strncpy(doc, name.data(), name.size());
            yyjson_mut_obj_put(val, key, sub);
            writeValue(doc, sub, field.[:f:]);
        }
#else
        boost::pfr::for_each_field_with_name(field, [&](std::string_view name, auto &sub_field) {
            auto *sub = yyjson_mut_null(doc);
            auto *key = yyjson_mut_strncpy(doc, name.data(), name.size());
            yyjson_mut_obj_put(val, key, sub);
            writeValue(doc, sub, sub_field);
        });
#endif
    } else {
        static_assert(always_false<FieldType>,
                      "jacinth: unsupported type for JSON serialization");
    }
}

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

    operator bool() const
    {
        return !is_null();
    }

    // access ops
    value operator[](std::string_view key) const
    {
        return {m_doc, yyjson_obj_getn(m_val, key.data(), key.size())};
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
};

// Read-only JSON tree
class doc {
    yyjson_doc *m_doc;

public:
    static doc read(std::string_view s)
    {
        return doc{yyjson_read(s.data(), s.size(), 0)};
    }

    explicit doc(yyjson_doc *d) noexcept : m_doc(d) {}

    // interpret the root as type T
    template <typename T>
    operator T() const
    {
        T t;
        parseValue(root(), t);
        return t;
    }

    value root() const
    {
        return {m_doc, m_doc->root};
    }

    doc(doc &&o) noexcept : m_doc(std::exchange(o.m_doc, nullptr)) {}
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

    template <typename T>
    mutable_value &assign(const T &v)
    {
        return *this = v;
    }

    template <typename T>
    explicit operator T() const
    {
        T t;
        // parseValue can't take mutable values...
        auto *idoc = yyjson_mut_val_imut_copy(m_val, nullptr);
        parseValue(idoc->root, t);
        return t;
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
};

// Read-write JSON tree
class json {
    yyjson_mut_doc *m_doc;

    explicit json(yyjson_mut_doc *d) noexcept : m_doc(d) {}

public:
    json() : m_doc(yyjson_mut_doc_new(nullptr)) {
        yyjson_mut_doc_set_root(m_doc, yyjson_mut_null(m_doc));
    }

    explicit json(doc const &d) : json(yyjson_doc_mut_copy(d, nullptr)) {}
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
    T to() const
    {
        T t;
        parseValue(root(), t);
        return t;
    }

    // i/o
    static json read(std::string_view s, yyjson_read_flag flg = 0)
    {
        yyjson_doc *d = yyjson_read(s.data(), s.size(), flg);
        yyjson_mut_doc *m = yyjson_doc_mut_copy(d, nullptr);
        yyjson_doc_free(d);
        return json{m};
    }

    // TODO: make these flags a struct or something
    std::string dump(yyjson_write_flag flag = 0)
    {
        auto *root = yyjson_mut_doc_get_root(m_doc);
        std::size_t len = 0;
        char *buf = root ? yyjson_mut_val_write(root, flag, &len) : nullptr;
        std::string s(buf ? buf : "", buf ? len : 0);
        free(buf);
        return s;
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

    mutable_value operator[](std::string_view key)
    {
        return root()[key];
    }

    mutable_value operator[](std::size_t i)
    {
        return root()[i];
    }
};

// TODO: OOP yyjson API, similar to nlohmann, etc

} // namespace jacinth