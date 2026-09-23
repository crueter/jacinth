// All iterator definitions for jacinth
module;

// force yyjson to have external linkage
#define yyjson_api_inline yyjson_inline
#include "yyjson.h"

#include <string_view>

export module jacinth:iter;

import :read;
import :write;

export namespace jacinth
{

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