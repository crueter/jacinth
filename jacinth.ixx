module;

#include <utility>
#include <vector>

#include "yyjson.h"
#include "boost/pfr.hpp"

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
            boost::pfr::for_each_field_with_name(
                field, [val](std::string_view name, auto &sub_field) {
                    yyjson_val *sub = yyjson_obj_getn(val, name.data(), name.size());
                    parseValue(sub, sub_field);
                });
    }
}

// TODO: OOP yyjson API, similar to nlohmann, etc

} // namespace jacinth