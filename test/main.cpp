#include <array>
#include <fstream>
#include <print>
#include <span>
#include <utility>
#include <vector>

#include <sstream>
#include <map>

import jacinth;

// TODO: make an actual unit testing suite
struct Asset {
    std::string name;
    std::size_t size;
    std::string digest;
    std::string created_at;
    std::string browser_download_url;
    std::optional<std::string> node_id;
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

struct ArrayStruct{
    std::string label;
    std::array<float, 4> values;
};

struct SpanWriteStruct {
    std::string label;
    std::span<const double, 3> values;
};

struct SpanReadStruct {
    std::span<int, 3> values;
};

struct MapStruct {
    std::string name;
    std::map<std::string, uint32_t> values;
};

enum Condition {
    New,
    LikeNew,
    Great,
    WellLoved,
    Used,
    Okay,
    Bad,
    Wrecked,
};

struct Car {
    std::string make;
    std::string model;
    std::size_t year;
    Condition condition;
};

namespace {

struct CustomStruct {
    std::string name;
};

void to_json(jacinth::mutable_value json, const CustomStruct &custom) {
    json["derived"] = std::format("Derived value from to_json: {}", custom.name);
}

void from_json(const jacinth::value &json, CustomStruct &custom) {
    custom.name = std::format("Derived value from from_json: {}", json["name"].as<std::string>());
}

}

std::string readAll(const std::string &filename) {
    std::ifstream file(filename);
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

int main()
{
    auto data = readAll("release.json");

    // Struct reflection
    {
        Release release = jacinth::doc::read(data);

        std::println("Release {}", release.name);
        std::println("  Tag: {}", release.tag_name);
        std::println("  URL: {}", release.html_url);
        std::println("  Assets:");

        std::size_t i = 0;

        for (const auto &a : std::as_const(release.assets)) {
            std::println("    Asset {}", i);
            std::println("      Name: {}", a.name);
            std::println("      Node ID: {}", a.node_id.value_or("Unknown"));
            // std::println("      Digest: {}", a.digest);
            // std::println("      URL: {}", a.browser_download_url);

            ++i;
        }
    }

    // Read from a JSON
    // TODO: fix doc
    {
        auto json = jacinth::doc::read(data);

        // TODO: std::formatter specializations
        std::println("Release {}", json["name"].as<std::string>());
        std::println("  Tag: {}", json["tag_name"].as<std::string>());
        std::println("  URL: {}", json["html_url"].as<std::string>());
        std::println("  Assets:");

        auto assets = json["assets"];

        for (auto a : assets.as_array()) {
            std::println("    Asset: {}", a["name"].as<std::string>());
        }
    }

    std::vector<Asset> newAssets = {Asset{
        .name = "Test-Asset.tar.gz",
        .size = 6791230,
        .digest = "sha256:abcdef1234567890",
        .created_at = "tomorrow",
        .browser_download_url = "https://example.com",
        .node_id = "node123256789abcdef"
    }};

    // Create a JSON from scratch
    {
        auto json = jacinth::json();
        json["name"] = "Jacinth";
        json["creator"] = "crueter";
        json["features"] = {
            "Modules",
            "Reflection",
            "OOP API"
        };

        auto dumped = json.dump();
        std::println("Dumped: {}", dumped);

        // TODO: is_type funcs
        for (auto [k, v] : json.as_object()) {
            std::println("  {}: {}", std::string(k), jacinth::json::dump(v));
        }
    }

    // Mutate existing json
    {
        auto json = jacinth::json::read(data);
        json["name"] = "Custom Name";
        json["body"] = "Release description :)";

        // TODO: remove, etc. methods
        const auto curAssets = json["assets"];
        json["assets"].remove(1, curAssets.size() - 1);

        std::println("Mutated dump: {}", json.dump());

        json["assets"].clear();

        std::println("Mutated dump, cleared assets: {}", json.dump());

        json.remove("html_url");

        std::println("Mutated dump, removed html_url: {}", json.dump());
    }

    // Direct write from a struct
    {
        std::println("Direct write from newAssets:");
        std::println("{}", jacinth::json::dump(newAssets, {.pretty = true}));
    }

    // Maps
    {
        MapStruct s = {
            "John",
            {
                {"James", 23},
                {"Kayla", 76},
                {"Thomas", 51}
            }
        };

        std::println("MapStruct:");
        std::string str;
        jacinth::json::dump_to(s, str, {.pretty = true});
        std::println("{}", str);

        // direct map serialization
        std::map<std::string, int> people = {
            {"James", 23},
            {"Kayla", 76},
            {"Thomas", 51}
        };

        std::println("direct map serialization: {}", jacinth::json::dump(people));
    }

    // Custom to/from json
    {
        CustomStruct s = {
            "CustomStruct Test"
        };

        const auto json_str = "{\"name\": \"CustomStruct JSON\"}";

        std::println("to_json: {}", jacinth::json::dump(s));
        CustomStruct newCustom = jacinth::json::read(json_str);
        std::println("from_json: {}", newCustom.name);
    }

    // test std::array
    {
        ArrayStruct s = {
            "ArrayStruct",
            {
                10.5, 6.7, 988.43, -10000000000
            }
        };

        const auto dumped = jacinth::json::dump(s);
        std::println("std::array: {}", dumped);

        ArrayStruct back = jacinth::json::read(dumped);
        std::println("  label: {}", back.label);
        std::println("  values: {} {} {} {}",
                     back.values[0], back.values[1], back.values[2], back.values[3]);
    }

    // test std::span
    {
        std::array<double, 3> vals = {1.5, 2.5, 3.5};

        SpanWriteStruct s = {"SpanWriteStruct", vals};
        std::println("static-extent span struct: {}", jacinth::json::dump(s));

        std::span<const double> dynamic(vals);
        auto dumped = jacinth::json::dump(dynamic);
        std::println("dynamic-extent span: {}", dumped);

        vals = jacinth::doc::read(dumped);
        std::println("static-extent span read: {}", vals);
    }

    // enum
    {
        auto cars_json = readAll("cars.json");

        std::vector<Car> cars = jacinth::json::read(cars_json);

        std::size_t i = 0;
        for (const auto &car : cars) {
            std::println("Car {}: {} {} {}", i, car.year, car.make, car.model);
            std::println("  Condition: {}", int(car.condition));
            ++i;
        }
    }

    // initializer list
    {
        jacinth::json json;
        json["name"] = "Jacinth";
        json["creator"] = "crueter";
        json["features"] = {
            "Modules",
            "Reflection",
            "OOP API"
        };

        std::println("Jacinth: {}", json.dump());
    }

    // directly dump an int
    {
        int hi = 67;
        std::println("Int Dump: {}", jacinth::json::dump(hi));
    }

    // directly dump a vector
    {
        std::vector<float> vec = {
            100.3,
            678.25,
            892349237.23,
            -198123424,
            -0
        };

        std::println("direct vector dump: {}", jacinth::json::dump(vec));
    }

    {
        std::vector<std::string> vec = {
            "Hello",
            "Hi",
            "Hey",
            "Heyo",
            "Hola",
            "Bonjour"
        };

        std::println("direct string vector dump: {}", jacinth::json::dump(vec));
    }
}
