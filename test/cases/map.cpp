// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <map>
#include <string>

import jacinth;
import tests;

struct MapStruct {
    std::string name;
    std::map<std::string, uint32_t> values;
};

void test_map()
{
    MapStruct s = {"John", {{"James", 23}, {"Kayla", 76}, {"Thomas", 51}}};

    const auto expect = R"({"name":"John","values":{"James":23,"Kayla":76,"Thomas":51}})";

    std::string str;
    jacinth::json::dump_to(s, str);

    tests::check_match("MapStruct", str, expect);

    // direct map serialization
    std::map<std::string, int> people = {{"James", 23}, {"Kayla", 76}, {"Thomas", 51}};

    const auto map_expect = R"({"James":23,"Kayla":76,"Thomas":51})";

    tests::check_match("Direct map serialization", jacinth::json::dump(people), map_expect);
}

int main()
{
    test_map();

    tests::exit();
}
