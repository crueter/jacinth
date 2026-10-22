// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <print>

import jacinth;
import tests;

struct Member {
    std::string username;
    uint64_t id;
};

struct GameInfo {
    std::string name;
    uint64_t id;
};

struct Room {
    GameInfo game;
    std::string title;
    std::string creationTime;
    uint64_t id = 0;
    std::vector<Member> members;
};

// clang-format off

// Serialize into:
// { gameName, name, members, owner }
template <>
inline constexpr jacinth::obj jacinth::meta<Room> = {
    jacinth::rename("title", "name"),
    // This will exclude id, creationTime, and game, and only include
    // from-keys, name, and members
    jacinth::exclude("id", "creationTime", "game"),
    // OR
    // This will include name (renamed from title) and members (default)
    // It will also include from-keys (owner, gameName)
    // jacinth::include("name", "members"),
    jacinth::from("gameName", [](auto &t) -> auto & { return t.game.name; }),
    jacinth::from("owner", [](auto &t) -> auto & { return t.members[0]; }),
};

// clang-format on

void test_read(std::string_view data)
{
    const Room room = jacinth::doc::read(data);

    std::println("Room {}", room.title);
    std::println("  Game: {}", room.game.name);
    std::println("  Members:");

    for (const auto &m : room.members) {
        std::println("    {}", m.username);
    }

    tests::check_match("Room title", room.title, std::string("BETA 1.7.3 AWESOME SERVER"));
    tests::check_match("Room game", room.game.name, std::string("Minecraft"));
    tests::check_match("Room member size", room.members.size(), std::size_t(4));
    tests::check_match("Room owner", room.members[0].username, std::string("crueter"));
    tests::check_match("Room ID", room.id, uint64_t(0));
    tests::check("Room has no creation time", room.creationTime.empty());
}

void test_write(std::string_view data)
{
    const Room room = jacinth::doc::read(data);

    const auto dumped = jacinth::json::dump(room, {.pretty = true});

    tests::check_match("data", tests::normalize_newlines(data), tests::normalize_newlines(dumped));
}

// ========================= //
// FLATTEN                   //
// ========================= //

struct PhysicalCharacteristics {
    double height;
    double weight;
};

struct Colors {
    std::string eyes;
    std::string skin;
    std::string hair;
};

struct Person {
    PhysicalCharacteristics characteristics;
    Colors colors;
    std::string name;
};

// this will flatten the "characteristics" and "colors" subfields into Person
// serializes to: {height, weight, eyeColor, skinColor, hairColor}
template <>
inline constexpr jacinth::obj jacinth::meta<Person> = {
    jacinth::flatten("characteristics", "colors"),
};

// meta specs applied to the flattened structs will still apply!
template <>
inline constexpr jacinth::obj jacinth::meta<Colors> = {
    jacinth::rename("eyes", "eyeColor"),
    jacinth::rename("skin", "skinColor"),
    jacinth::rename("hair", "hairColor"),
};

void test_flatten(std::string_view data)
{
    const Person person = jacinth::doc::read(data);

    tests::check_match("Name", person.name, "Marcus");
    tests::check_match("Skin", person.colors.skin, "white");
    tests::check_match("Eyes", person.colors.eyes, "blue");
    tests::check_match("Hair", person.colors.hair, "blond");
    tests::check_match("Height", person.characteristics.height, 75.5);
    tests::check_match("Weight", person.characteristics.weight, 196.3);

    const auto dumped = jacinth::json::dump(person, {.pretty = true});
    tests::check_match("data", tests::normalize_newlines(data), tests::normalize_newlines(dumped));
}

int main()
{
    const std::string room = {
#include "room_json.inc"
    };

    const std::string person = {
#include "person_json.inc"
    };

    // Read
    test_read(room);

    // Write
    test_write(room);

    // Flatten
    test_flatten(person);

    tests::exit();
}