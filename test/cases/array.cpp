// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <array>
#include <span>
#include <string>
#include <vector>

import jacinth;
import tests;

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

void test_span()
{
    std::array<double, 3> vals = {1.5, 2.5, 3.5};

    SpanWriteStruct s = {"SpanWriteStruct", vals};
    const std::string s_expect = R"({"label":"SpanWriteStruct","values":[1.5,2.5,3.5]})";
    tests::check_match("static-extent span struct", jacinth::json::dump(s), s_expect);

    std::span<const double> dynamic(vals);
    const std::string expect = R"([1.5,2.5,3.5])";
    tests::check_match("dynamic-extent span", jacinth::json::dump(dynamic), expect);
}

void test_array()
{
    const ArrayStruct s = {"ArrayStruct", {10.5, 6.7, 988.43, -10000000000}};
    const std::string expect =
        R"({"label":"ArrayStruct","values":[10.5,6.7,988.43,-10000000000.0]})";
    tests::check_match("std::array", jacinth::json::dump(s), expect);

    ArrayStruct back = jacinth::json::read(expect);
    tests::check_match("std::array roundtrip label", back.label, s.label);
    tests::check_match("std::array roundtrip values", back.values, s.values);
}

void test_initlist()
{
    jacinth::json json;
    json["name"] = "Jacinth";
    json["creator"] = "crueter";
    json["features"] = {"Modules", "Reflection", "OOP API"};

    const std::string expect =
        R"({"name":"Jacinth","creator":"crueter","features":["Modules","Reflection","OOP API"]})";
    tests::check_match("initializer list", json.dump(), expect);
}

void test_vecdump()
{
    std::vector<float> vec = {100.3, 678.25, 892349237.23, -198123424, -0};
    std::vector<float> back = jacinth::json::read(jacinth::json::dump(vec));
    tests::check_match("vector<float> roundtrip", back, vec);

    const std::vector<std::string> svec = {"Hello", "Hi", "Hey", "Heyo", "Hola", "Bonjour"};
    const std::string expect = R"(["Hello","Hi","Hey","Heyo","Hola","Bonjour"])";
    tests::check_match("string vector", jacinth::json::dump(svec), expect);

    std::vector<std::string> sback = jacinth::json::read(expect);
    tests::check_match("string vector roundtrip", sback, svec);
}

void test_nestarray()
{
    jacinth::json json;
    json["hello"]["nested"][3] = 15.0;

    const std::string expect = R"({"hello":{"nested":[null,null,null,15.0]}})";
    tests::check_match("nested array", json.dump(), expect);
    tests::check_match("nested array read", json["hello"]["nested"][3].get<double>(), 15.0);
}

int main()
{
    test_span();
    test_array();
    test_initlist();
    test_vecdump();
    test_nestarray();

    tests::exit();
}
