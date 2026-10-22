// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <format>
#include <print>
#include <string>

import jacinth;
import tests;

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

void test_adl()
{
    CustomStruct s = {"CustomStruct Test"};

    const auto expect = R"({"derived":"Derived value from to_json: CustomStruct Test"})";
    tests::check_match("CustomStruct to_json", jacinth::json::dump(s), expect);

    const auto json_str = "{\"name\": \"CustomStruct JSON\"}";
    const auto from_expect = R"(Derived value from from_json: CustomStruct JSON)";
    CustomStruct newCustom = jacinth::json::read(json_str);
    tests::check_match("CustomStruct from_json", newCustom.name, from_expect);
}

int main()
{
    test_adl();

    tests::exit();
}
