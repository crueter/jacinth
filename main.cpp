#include <fstream>
#include <print>
#include <utility>

#include "yyjson.h"

import boost.pfr;

/*

{
  "name": "Eden-Linux-amd64-gcc-standard.AppImage.zsync",
  "size": 247218,
  "digest":
"sha256:c405659a163190fe34fd23d9a1c04e4f57a107ff45c58a132560aeca80b39055",
  "created_at": "2026-09-17T21:44:30+00:00",
  "browser_download_url":
"https://nightly.eden-emu.dev/v1789678126.0ce29be608/Eden-Linux-amd64-gcc-standard.AppImage.zsync"
}

*/

// vector specializations
template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

struct Asset {
    std::string name;
    std::size_t size;
    std::string digest;
    std::string created_at;
    std::string browser_download_url;
};

struct Release {
    std::string tag_name;
    std::string name;
    std::string html_url;
    std::string body;
    std::string created_at;
    std::string published_at;
    std::vector<Asset> assets;
};

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
                    yyjson_val *sub_val = yyjson_obj_getn(val, name.data(), name.size());
                    parseValue(sub_val, sub_field);
                });
    }
}

int main()
{
    std::ifstream file("release.json");
    std::stringstream buf;
    buf << file.rdbuf();
    auto json = buf.str();

    yyjson_doc *doc = yyjson_read(json.c_str(), json.size(), 0);
    if (!doc)
        return 1;

    yyjson_val *root = yyjson_doc_get_root(doc);
    Release release;

    parseValue(root, release);

    yyjson_doc_free(doc);

    std::println("Release {}", release.name);
    std::println("  Tag: {}", release.tag_name);
    std::println("  URL: {}", release.html_url);
    std::println("  Assets:");

    std::size_t i = 0;

    for (const auto &a : std::as_const(release.assets)) {
        std::println("    Asset {}", i);

        boost::pfr::for_each_field_with_name(a, [](const auto &name, const auto &field) {
            std::println("      {}: {}", name, field);
        });

        ++i;
    }
}
