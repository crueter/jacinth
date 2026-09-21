#include <array>
#include <expected>
#include <fstream>
#include <print>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

#include <map>
#include <sstream>
#include <string_view>

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

struct ArrayStruct {
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

namespace
{

struct CustomStruct {
    std::string name;
};

void to_json(jacinth::mutable_value json, const CustomStruct &custom)
{
    json["derived"] = std::format("Derived value from to_json: {}", custom.name);
}

void from_json(const jacinth::value &json, CustomStruct &custom)
{
    custom.name = std::format("Derived value from from_json: {}", json.get<std::string>("name"));
}

} // namespace

std::string readAll(const std::string &filename)
{
    std::ifstream file(filename);
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

int failures = 0;

void expect(bool cond, std::string_view what)
{
    if (!cond) {
        std::println("FAIL: {}", what);
        ++failures;
    }
}

template <typename T>
void expect_errc(const std::expected<T, std::error_code> &res, jacinth::errc want,
                 std::string_view what)
{
    if (res) {
        std::println("FAIL: {} (expected error {}, got value)", what, int(want));
        ++failures;
        return;
    }
    if (res.error().value() != int(want)) {
        std::println("FAIL: {} (error code {}, message \"{}\")", what, res.error().value(),
                     res.error().message());
        ++failures;
    } else {
        std::println("PASS: {} (got \"{}\")", what, res.error().message());
    }
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

        std::println("{}", json.dump());

        // test assignment instead of copy init
        std::string tag;

        // this should be a jacinth::value
        auto url_v = json.get("html_url");
        auto url = url_v.get<std::string>();
        tag = json.get("tag_name");

        auto asset_0_v = json.get("assets").get(0);
        std::println("Asset 0: {}", asset_0_v.get<std::string>("name"));

        // TODO: std::formatter specializations
        std::println("Release {}", json["name"].get<std::string>());
        std::println("  Tag: {}", tag);
        std::println("  URL: {}", url);
        std::println("  Assets:");

        auto assets = json["assets"];

        for (auto a : assets.as_array()) {
            std::println("    Asset: {}", a.get<std::string>("name"));
        }
    }

    std::vector<Asset> newAssets = {Asset{.name = "Test-Asset.tar.gz",
                                          .size = 6791230,
                                          .digest = "sha256:abcdef1234567890",
                                          .created_at = "tomorrow",
                                          .browser_download_url = "https://example.com",
                                          .node_id = "node123256789abcdef"}};

    // Create a JSON from scratch
    {
        auto json = jacinth::json();
        json["name"] = "Jacinth";
        json["creator"] = "crueter";
        json["features"] = {"Modules", "Reflection", "OOP API"};

        auto dumped = json.dump();
        std::println("Dumped: {}", dumped);

        // TODO: is_type funcs
        for (auto [k, v] : json.as_object()) {
            std::println("  {}: {}", std::string(k), v.dump());
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
        MapStruct s = {"John", {{"James", 23}, {"Kayla", 76}, {"Thomas", 51}}};

        std::println("MapStruct:");
        std::string str;
        jacinth::json::dump_to(s, str, {.pretty = true});
        std::println("{}", str);

        // direct map serialization
        std::map<std::string, int> people = {{"James", 23}, {"Kayla", 76}, {"Thomas", 51}};

        std::println("direct map serialization: {}", jacinth::json::dump(people));
    }

    // Custom to/from json
    {
        CustomStruct s = {"CustomStruct Test"};

        const auto json_str = "{\"name\": \"CustomStruct JSON\"}";

        std::println("to_json: {}", jacinth::json::dump(s));
        CustomStruct newCustom = jacinth::json::read(json_str);
        std::println("from_json: {}", newCustom.name);
    }

    // test std::array
    {
        ArrayStruct s = {"ArrayStruct", {10.5, 6.7, 988.43, -10000000000}};

        const auto dumped = jacinth::json::dump(s);
        std::println("std::array: {}", dumped);

        ArrayStruct back = jacinth::json::read(dumped);
        std::println("  label: {}", back.label);
        std::println("  values: {} {} {} {}", back.values[0], back.values[1], back.values[2],
                     back.values[3]);
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
        json["features"] = {"Modules", "Reflection", "OOP API"};

        std::println("Jacinth: {}", json.dump());
    }

    // directly dump an int
    {
        int hi = 67;
        std::println("Int Dump: {}", jacinth::json::dump(hi));
    }

    // directly dump a vector
    {
        std::vector<float> vec = {100.3, 678.25, 892349237.23, -198123424, -0};

        std::println("direct vector dump: {}", jacinth::json::dump(vec));
    }

    {
        std::vector<std::string> vec = {"Hello", "Hi", "Hey", "Heyo", "Hola", "Bonjour"};

        std::println("direct string vector dump: {}", jacinth::json::dump(vec));
    }

    // move semantics
    {
        jacinth::doc a = jacinth::doc::read("{\"x\":1}");
        {
            jacinth::doc b = jacinth::doc::read("{\"y\":2}");
            std::println("before: a.x={}", a.get<int>("x"));
            a = std::move(b);
        }
        std::println("after:  a.y={}", a.get<int>("y"));
    }

    // create a nested json array
    {
        jacinth::json json;
        json["hello"]["nested"][3] = 15.0;

        std::println("{}", json.dump());
    }

    // This is AI slop because I cba to write 6000 unit tests on random nonsense, enjoy

    // opt-in error handling
    {
        // whole-document parse failures
        expect_errc(jacinth::doc::try_read("not json {"), jacinth::errc::invalid_json,
                    "doc::try_read invalid text");
        expect_errc(jacinth::json::try_read("not json {"), jacinth::errc::invalid_json,
                    "json::try_read invalid text");
        expect_errc(jacinth::doc::try_read(""), jacinth::errc::invalid_json,
                    "doc::try_read empty text");

        // typed read: release.json has no Car keys
        expect_errc(jacinth::doc::try_read<Car>(data), jacinth::errc::missing_value,
                    "doc::try_read<Car> from release.json");
        expect_errc(jacinth::json::try_read<Car>(data), jacinth::errc::missing_value,
                    "json::try_read<Car> from release.json");

        // nested required field missing
        expect_errc(jacinth::doc::try_read<Asset>(R"({"name":"x"})"), jacinth::errc::missing_value,
                    "doc::try_read<Asset> missing fields");

        // wrong root type for an aggregate
        expect_errc(jacinth::doc::try_read<Car>("[]"), jacinth::errc::type_mismatch,
                    "doc::try_read<Car> from array");
        expect_errc(jacinth::doc::try_read<Car>("null"), jacinth::errc::type_mismatch,
                    "doc::try_read<Car> from null");

        // scalar conversions
        {
            auto v = jacinth::doc::read(R"({
                "s": "hello",
                "n": 42,
                "d": 3.5,
                "b": true,
                "obj": {"x": 1},
                "arr": [1, 2],
                "badarr": [1, "two"],
                "nil": null,
                "opt_good": 7,
                "opt_null": null
            })");

            expect_errc(v.try_get<std::string>("nil"), jacinth::errc::type_mismatch,
                        "null as string");
            expect_errc(v.try_get<int>("s"), jacinth::errc::type_mismatch, "string as int");
            expect_errc(v.try_get<double>("s"), jacinth::errc::type_mismatch, "string as double");
            expect_errc(v.try_get<std::string>("n"), jacinth::errc::type_mismatch,
                        "int as string");
            expect_errc(v.try_get<bool>("n"), jacinth::errc::type_mismatch, "int as bool");
            expect_errc(v.try_get<int>("b"), jacinth::errc::type_mismatch, "bool as int");
            expect_errc(v.try_get<int>("obj"), jacinth::errc::type_mismatch, "object as int");
            expect_errc(v.try_get<int>("arr"), jacinth::errc::type_mismatch, "array as int");

            // missing key / out-of-range array index
            expect_errc(v.try_get<int>("missing"), jacinth::errc::missing_value,
                        "missing key as int");
            expect_errc(v.try_get<int>("missing"), jacinth::errc::missing_value,
                        "try_get missing key");
            expect_errc(v["arr"][99].try_get<int>(), jacinth::errc::missing_value,
                        "array index out of range");

            // container element type errors
            expect_errc(v.try_get<std::vector<int>>("badarr"), jacinth::errc::type_mismatch,
                        "vector<int> from [1, \"two\"]");
            expect_errc(v.try_get<std::array<float, 2>>("badarr"), jacinth::errc::type_mismatch,
                        "array<float,2> from [1, \"two\"]");

            // optionals: present / null / missing all succeed
            auto opt_good = v.try_get<std::optional<int>>("opt_good");
            expect(opt_good.has_value() && opt_good->has_value() && **opt_good == 7,
                   "optional present value");
            auto opt_null = v.try_get<std::optional<int>>("opt_null");
            expect(opt_null.has_value() && !opt_null->has_value(), "optional null value");
            auto opt_missing = v.try_get<std::optional<int>>("nope");
            expect(opt_missing.has_value() && !opt_missing->has_value(), "optional missing key");
            expect_errc(v.try_get<std::optional<std::string>>("opt_good"),
                        jacinth::errc::type_mismatch, "optional int from string");
        }

        // map with a bad element type
        {
            auto m = jacinth::doc::read(R"({"name":"ok","values":{"a":"boom"}})");
            expect_errc(m.try_get<MapStruct>(), jacinth::errc::type_mismatch,
                        "map<string,uint32_t> from bad value");
        }

        // enum from a string
        {
            auto c = jacinth::doc::read(R"({"make":"x","model":"y","year":1,"condition":"New"})");
            expect_errc(c.try_get<Car>(), jacinth::errc::type_mismatch, "enum from string");
        }

        // error_code ergonomics: == errc and category exposure
        auto bad = jacinth::doc::try_read("nope");
        expect(!bad.has_value() && bad.error() == jacinth::errc::invalid_json,
               "error_code == errc comparison");
        expect(std::string(bad.error().category().name()) == "jacinth", "error category name");
        expect(!bad.error().message().empty(), "error message non-empty");
        auto missing = jacinth::doc::try_read<Car>(data);
        expect(missing.error() != jacinth::errc::invalid_json, "error_code != errc comparison");
        expect(missing.error() == jacinth::errc::missing_value, "missing_value comparison");
    }

    // [END AI SLOP]

    return failures ? 1 : 0;
}
