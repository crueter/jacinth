#include <cassert>
#include <fstream>
#include <print>
#include <sstream>

import jacinth;

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

std::string readAll(const std::string &filename)
{
    std::ifstream file(filename);
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// TODO USE A REAL TEST FRAMEWORK
void check(std::string_view test, bool condition) {
    if (condition) {
        std::println("PASS: {}", test);
    } else {
        std::println("FAIL: {}", test);
    }
}

int main()
{
    const auto data = readAll("room.json");

    // Read
    {
        const Room room = jacinth::doc::read(data);

        std::println("Room {}", room.title);
        std::println("  Game: {}", room.game.name);
        std::println("  Members:");

        for (const auto &m : room.members) {
            std::println("    {}", m.username);
        }

        check("Room title is BETA 1.7.3 AWESOME SERVER", room.title == "BETA 1.7.3 AWESOME SERVER");
        check("Room is playing minecraft", room.game.name == "Minecraft");
        check("Room has 4 members", room.members.size() == 4);
        check("Room owner is crueter", room.members[0].username == "crueter");
        check("Room has an invalid ID", room.id == 0);
        check("Room has no creation time", room.creationTime.empty());
    }

    // Write
    {
        const Room room = jacinth::doc::read(data);

        const auto dumped = jacinth::json::dump(room, {.pretty = true});

        std::println("Dumped:");
        std::println("{}", dumped);

        check("data == dumped", data == dumped);

        assert(data == dumped);
    }
}