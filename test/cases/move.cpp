// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <print>
#include <utility>

import jacinth;
import tests;

void test_move()
{
    jacinth::doc a = jacinth::doc::read("{\"x\":1}");
    {
        jacinth::doc b = jacinth::doc::read("{\"y\":2}");
        tests::check_match("before: a.x == 1", a.get<int>("x"), 1);
        a = std::move(b);
    }
    tests::check_match("after : a.y == 2", a.get<int>("y"), 2);
}

int main()
{
    test_move();

    tests::exit();
}
