// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <optional>
#include <string_view>
#include <map>
#include <cstdint>
#include <array>
#include <system_error>
#include <vector>

import jacinth;
import tests;

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

struct Asset {
    std::string name;
    std::size_t size;
    std::string digest;
    std::string created_at;
    std::string browser_download_url;
    std::optional<std::string> node_id;
};

struct MapStruct {
    std::string name;
    std::map<std::string, uint32_t> values;
};

// [WARNING: MOSTLY AI SLOP]

static const std::string release_data = {
    #embed "../data/release.json"
};

void test_invalid(std::string_view data)
{
    // whole-document parse failures
    tests::check_errc("doc::try_read invalid text", jacinth::doc::try_read("not json {"),
                      jacinth::errc::invalid_json);
    tests::check_errc("json::try_read invalid text", jacinth::json::try_read("not json {"),
                      jacinth::errc::invalid_json);
    tests::check_errc("doc::try_read empty text", jacinth::doc::try_read(""),
                      jacinth::errc::invalid_json);

    // typed read: release.json has no Car keys
    tests::check_errc("doc::try_read<Car> from release.json", jacinth::doc::try_read<Car>(data),
                      jacinth::errc::missing_value);
    tests::check_errc("json::try_read<Car> from release.json", jacinth::json::try_read<Car>(data),
                      jacinth::errc::missing_value);

    // nested required field missing
    tests::check_errc("doc::try_read<Asset> missing fields",
                      jacinth::doc::try_read<Asset>(R"({"name":"x"})"),
                      jacinth::errc::missing_value);

    // wrong root type for an aggregate
    tests::check_errc("doc::try_read<Car> from array", jacinth::doc::try_read<Car>("[]"),
                      jacinth::errc::type_mismatch);
    tests::check_errc("doc::try_read<Car> from null", jacinth::doc::try_read<Car>("null"),
                      jacinth::errc::type_mismatch);

    // map with a bad element type
    {
        auto m = jacinth::doc::read(R"({"name":"ok","values":{"a":"boom"}})");
        tests::check_errc("map<string,uint32_t> from bad value", m.try_get<MapStruct>(),
                          jacinth::errc::type_mismatch);
    }

    // enum from a string
    {
        auto c = jacinth::doc::read(R"({"make":"x","model":"y","year":1,"condition":"New"})");
        tests::check_errc("enum from string", c.try_get<Car>(), jacinth::errc::type_mismatch);
    }
}

void test_scalar()
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

    tests::check_errc("null as string", v.try_get<std::string>("nil"),
                      jacinth::errc::type_mismatch);
    tests::check_errc("string as int", v.try_get<int>("s"), jacinth::errc::type_mismatch);
    tests::check_errc("string as double", v.try_get<double>("s"), jacinth::errc::type_mismatch);
    tests::check_errc("int as string", v.try_get<std::string>("n"), jacinth::errc::type_mismatch);
    tests::check_errc("int as bool", v.try_get<bool>("n"), jacinth::errc::type_mismatch);
    tests::check_errc("bool as int", v.try_get<int>("b"), jacinth::errc::type_mismatch);
    tests::check_errc("object as int", v.try_get<int>("obj"), jacinth::errc::type_mismatch);
    tests::check_errc("array as int", v.try_get<int>("arr"), jacinth::errc::type_mismatch);

    // missing key / out-of-range array index
    tests::check_errc("missing key as int", v.try_get<int>("missing"),
                      jacinth::errc::missing_value);
    tests::check_errc("try_get missing key", v.try_get<int>("missing"),
                      jacinth::errc::missing_value);
    tests::check_errc("array index out of range", v["arr"][99].try_get<int>(),
                      jacinth::errc::missing_value);

    // container element type errors
    tests::check_errc("vector<int> from [1, \"two\"]", v.try_get<std::vector<int>>("badarr"),
                      jacinth::errc::type_mismatch);
    tests::check_errc("array<float,2> from [1, \"two\"]",
                      v.try_get<std::array<float, 2>>("badarr"), jacinth::errc::type_mismatch);

    // optionals: present / null / missing all succeed
    auto opt_good = v.try_get<std::optional<int>>("opt_good");
    tests::check("optional present value",
                 opt_good.has_value() && opt_good->has_value() && **opt_good == 7);
    auto opt_null = v.try_get<std::optional<int>>("opt_null");
    tests::check("optional null value", opt_null.has_value() && !opt_null->has_value());
    auto opt_missing = v.try_get<std::optional<int>>("nope");
    tests::check("optional missing key", opt_missing.has_value() && !opt_missing->has_value());
    tests::check_errc("optional int from string",
                      v.try_get<std::optional<std::string>>("opt_good"),
                      jacinth::errc::type_mismatch);
}

void test_errc()
{
    // error_code ergonomics: == errc and category exposure
    auto bad = jacinth::doc::try_read("nope");
    tests::check("error_code == errc comparison",
                 !bad.has_value() && bad.error() == jacinth::errc::invalid_json);
    tests::check_match("error category name",
                       std::string(bad.error().category().name()), std::string("jacinth"));
    tests::check("error message non-empty", !bad.error().message().empty());
    auto missing = jacinth::doc::try_read<Car>(release_data);
    tests::check("error_code != errc comparison", missing.error() != jacinth::errc::invalid_json);
    tests::check("missing_value comparison", missing.error() == jacinth::errc::missing_value);
}

int main()
{
    test_invalid(release_data);
    test_scalar();
    test_errc();

    tests::exit();
}
