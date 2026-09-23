// This stores type specializations for parseValue and writeValue
module;

#include <array>
#include <map>
#include <optional>
#include <span>
#include <vector>

#include "yyjson.h"

export module jacinth:spec;

export {
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
};

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

} // namespace jacinth