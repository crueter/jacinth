module;

#include <expected>
#include <system_error>
#include <utility>

// force yyjson to have external linkage
#define yyjson_api_inline yyjson_inline
#include "yyjson.h"

#ifdef JACINTH_USE_REFLECTION
#include <meta>
#else
#include "boost/pfr.hpp"
#endif

export module jacinth:write;

export import :err;
export import :spec;
export import :read;

export template <typename T>
jacinth::errc writeValue(yyjson_mut_doc *doc, yyjson_mut_val *val, const T &field);

export namespace jacinth
{
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
        return get<T>();
    }

    // explicit conv
    template <typename T>
    T get() const
    {
        T t{};
        get_to(t);
        return t;
    }

    // explicit conv into an existing value (non-allocating)
    template <typename T>
        requires json_convertible<T>
    void get_to(T &value) const
    {
        [[maybe_unused]] auto _ = try_get_to(value);
    }

    // explicit conv, with error handling
    template <typename T>
    std::expected<T, std::error_code> try_get() const
    {
        T t{};
        if (auto res = try_get_to(t); !res)
            return std::unexpected(res.error());

        return t;
    }

    // explicit conv into an existing value, with error handling (non-allocating)
    template <typename T>
    std::expected<void, std::error_code> try_get_to(T &value) const
    {
        if (!m_val)
            return std::unexpected(jacinth::make_error_code(jacinth::errc::missing_value));

        // TODO: this sucks, find a way to optimize
        auto *idoc = yyjson_mut_val_imut_copy(m_val, nullptr);
        auto res = parseValue(idoc->root, value);
        yyjson_doc_free(idoc);

        if (res == jacinth::errc::ok)
            return {};

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

    // TODO: this is identical to value
    // non-allocating dump into an existing string
    void dump_to(std::string &s, write_opts opts = {}) const
    {
        if (!m_val) {
            s.clear();
            return;
        }

        const auto flg = opts.to_flags();

        // 512 byte prealloc is a reasonable sweet spot for most docs
        std::size_t cap = s.capacity();
        if (cap < 512)
            cap = 512;

        while (true) {
            // write will overwrite the data anyways, just allocate
            s.resize_and_overwrite(cap, [cap](char *, std::size_t) { return cap; });

            // nonzero return means the write succeeded
            yyjson_write_err err{};
            if (std::size_t len = yyjson_mut_val_write_buf(s.data(), cap, m_val, flg, &err)) {
                s.resize(len);
                return;
            }

            // memory alloc errors mean we just need to allocate more
            // otherwise, or if >1mb, fall back to a direct write
            if (err.code != YYJSON_WRITE_ERROR_MEMORY_ALLOCATION || cap >= (std::size_t{1} << 20))
                break;
            cap *= 2;
        }

        std::size_t len = 0;
        char *buf = yyjson_mut_val_write_opts(m_val, flg, nullptr, &len, nullptr);
        if (!buf) {
            s.clear();
            return;
        }
        s.assign(buf, len);
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

    // read directly into an existing object (non-allocating)
    template <typename T>
    static void read_to(T &value, std::string_view s, yyjson_read_flag flg = 0)
    {
        read(s, flg).get_to(value);
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

    // try reading directly into a type
    template <typename T>
    static std::expected<T, std::error_code> try_read(std::string_view s, yyjson_read_flag flg = 0)
    {
        auto d = try_read(s, flg);
        if (!d)
            return std::unexpected(d.error());

        return d.value().template try_get<T>();
    }

    // try reading directly into an existing object (non-allocating)
    template <typename T>
    static std::expected<void, std::error_code> try_read_to(T &value, std::string_view s,
                                                            yyjson_read_flag flg = 0)
    {
        auto d = try_read(s, flg);
        if (!d)
            return std::unexpected(d.error());

        return d.value().try_get_to(value);
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

} // namespace jacinth

// TODO: handle variant?
export template <typename T>
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
        // num vectors can be converted very easily
        using Elem = std::decay_t<typename FieldType::value_type>;
        const std::size_t n = field.size();
        yyjson_mut_val *arr = nullptr;

        if (n) [[likely]] {
            if constexpr (std::is_same_v<Elem, float>)
                arr = yyjson_mut_arr_with_float(doc, field.data(), n);
            if constexpr (std::is_same_v<Elem, double>)
                arr = yyjson_mut_arr_with_double(doc, field.data(), n);
            if constexpr (std::is_same_v<Elem, uint64_t>)
                arr = yyjson_mut_arr_with_uint64(doc, field.data(), n);
            if constexpr (std::is_same_v<Elem, int64_t>)
                arr = yyjson_mut_arr_with_sint64(doc, field.data(), n);
            if constexpr (std::is_same_v<Elem, uint32_t>)
                arr = yyjson_mut_arr_with_uint32(doc, field.data(), n);
            if constexpr (std::is_same_v<Elem, int32_t>)
                arr = yyjson_mut_arr_with_sint32(doc, field.data(), n);
            if constexpr (std::is_same_v<Elem, uint16_t>)
                arr = yyjson_mut_arr_with_uint16(doc, field.data(), n);
            if constexpr (std::is_same_v<Elem, int16_t>)
                arr = yyjson_mut_arr_with_sint16(doc, field.data(), n);
            if constexpr (std::is_same_v<Elem, uint8_t>)
                arr = yyjson_mut_arr_with_uint8(doc, field.data(), n);
            if constexpr (std::is_same_v<Elem, int8_t>)
                arr = yyjson_mut_arr_with_sint8(doc, field.data(), n);
        }

        if (arr) {
            val->tag = arr->tag;
            val->uni = arr->uni;
            return {};
        }

        // fallback for aggregates/vectors/etc
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

        auto process_field = [val, doc, &result]
#if defined(__GNUG__) || defined(__clang__)
            [[gnu::always_inline]]
#elif defined(_MSC_VER)
            [[msvc::forceinline]]
#endif
            (std::string_view name, auto &sub_field) {
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
            };

#ifdef JACINTH_USE_REFLECTION
        template for (constexpr auto f :
                      std::define_static_array(std::meta::nonstatic_data_members_of(
                          ^^T, std::meta::access_context::current())))
        {
            constexpr auto name = std::meta::identifier_of(f);
            process_field(name, field.[:f:]);
            if (result != jacinth::errc::ok)
                return result;
        }
#else
        boost::pfr::for_each_field_with_name(field, process_field);
#endif

        return result;
    } else {
        static_assert(always_false<FieldType>, "jacinth: unsupported type for JSON serialization");
    }

    return {};
}
