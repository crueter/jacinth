// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <format>
#include <print>
#include <string>
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

void test_cars(std::string_view data)
{
    const std::vector<Car> cars = jacinth::json::read(data);

    tests::check_match("Cars size", cars.size(), 4UL);

    const std::string_view makes[] = {"Acura", "Chevrolet", "Ford", "Toyota"};
    const std::string_view models[] = {"MDX", "Suburban", "F-150 Raptor", "Corolla"};
    const std::size_t years[] = {2006, 1993, 2026, 2015};
    const Condition conditions[] = {LikeNew, Bad, New, Used};

    for (std::size_t i = 0; i < cars.size(); ++i) {
        const auto expect = std::format("{} {} {}", years[i], makes[i], models[i]);
        const auto actual = std::format("{} {} {}", cars[i].year, cars[i].make, cars[i].model);

        tests::check_match(std::format("Car {} name", i), actual, expect);
        tests::check_match(std::format("Car {} condition", i), int(cars[i].condition),
                           int(conditions[i]));
    }
}

int main()
{
    const std::string data = {
#embed "../data/cars.json"
    };

    test_cars(data);

    tests::exit();
}
