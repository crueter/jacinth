#include <fstream>
#include <print>
#include <utility>
#include <vector>

#include <sstream>
#include "yyjson.h"

import jacinth;

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

int main()
{
    std::ifstream file("release.json");
    std::stringstream buf;
    buf << file.rdbuf();
    auto data = buf.str();

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
        std::println("Release {}", std::string(json["name"]));
        std::println("  Tag: {}", std::string(json["tag_name"]));
        std::println("  URL: {}", std::string(json["html_url"]));
        std::println("  Assets:");

        auto assets = json["assets"];

        for (auto a : assets.as_array()) {
            std::println("    Asset: {}", std::string(a["name"]));
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
        json["hi"] = "Hello World!", json["creator"] = "Jacinth, by crueter";
        json["files"] = newAssets;

        auto dumped = json.dump();
        std::println("Dumped: {}", dumped);

        // TODO: is_type funcs
        for (auto [k, v] : json.as_object()) {
            std::println("  {}: {}", std::string(k), std::string(v));
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
        std::println("  {}", jacinth::json::dump(newAssets));
    }

    // TODO: test doc, struct write
}
