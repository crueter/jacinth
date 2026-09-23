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

export module jacinth:read;

export import :err;
export import :spec;

// TODO: is_<type> funcs?

export template <typename T>
jacinth::errc parseValue(yyjson_val *val, T &field);

export namespace jacinth
{

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
        [[maybe_unused]] auto _ = parseValue(m_val, value);
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
        auto res = parseValue(m_val, value);
        if (res == jacinth::errc::ok)
            return {};

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
            if (std::size_t len = yyjson_val_write_buf(s.data(), cap, m_val, flg, &err)) {
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
        char *buf = yyjson_val_write_opts(m_val, flg, nullptr, &len, nullptr);
        if (!buf) {
            s.clear();
            return;
        }
        s.assign(buf, len);
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

    // read directly into an existing object (non-allocating)
    template <typename T>
    static void read_to(T &value, std::string_view s, yyjson_read_flag flg = 0)
    {
        read(s, flg).get_to(value);
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

} // namespace jacinth

export {
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
                field =
                    static_cast<FieldType>(WideType(static_cast<WideType>(yyjson_get_int(val))));
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
}