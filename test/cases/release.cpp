// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <cassert>
#include <print>
#include <utility>

import jacinth;
import tests;

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

// Struct reflection
void test_refl(std::string_view data)
{
    Release release = jacinth::doc::read(data);

    tests::check_match("Release name", release.name, std::string("Eden Nightly - Sep 17 2026"));
    tests::check_match("Release tag", release.tag_name, std::string("v1789678126.0ce29be608"));
    tests::check_match(
        "Release URL", release.html_url,
        std::string(
            "https://git.eden-emu.dev/eden-ci/nightly/releases/tag/v1789678126.0ce29be608"));

    tests::check_match("Release assets size", release.assets.size(), 32UL);
    tests::check("Release asset 0 Node ID missing", !release.assets[0].node_id);
    tests::check_match("Release asset 1 Node ID", release.assets[1].node_id.value(),
                       std::string("node1234"));
    tests::check_match("Release asset 2 name", release.assets[2].name,
                       std::string("Eden-Linux-0ce29be608-legacy-gcc-standard.AppImage"));
}

// Mutate existing json
void test_mutate(std::string_view data)
{
    auto json = jacinth::json::read(data);
    const std::string newName = "Custom Name";
    const std::string newBody = "Release description :)";
    json["name"] = newName;
    json["body"] = newBody;

    tests::check_match("Mutated release name", json.get<std::string>("name"), newName);
    tests::check_match("Mutated release body", json.get<std::string>("body"), newBody);

    json["assets"].remove(1);
    tests::check_match("Mutated release assets (1 removed) size", json["assets"].size(), 31UL);

    json["assets"].clear();
    tests::check_match("Mutated release assets (cleared)", json["assets"].size(), 0UL);

    json.remove("html_url");
    tests::check_match("Mutated release html_url is null", json.get("html_url").is_null(), true);
}

// Direct write from a struct
void test_direct()
{
    std::vector<Asset> assets = {Asset{.name = "Test-Asset.tar.gz",
                                          .size = 6791230,
                                          .digest = "sha256:abcdef1234567890",
                                          .created_at = "tomorrow",
                                          .browser_download_url = "https://example.com",
                                          .node_id = "node123256789abcdef"}};

    const std::string expect =
        R"([{"name":"Test-Asset.tar.gz","size":6791230,"digest":"sha256:abcdef1234567890","created_at":"tomorrow","browser_download_url":"https://example.com","node_id":"node123256789abcdef"}])";

    tests::check_match("Direct asset write", jacinth::json::dump(assets), expect);
}

int main()
{
    const std::string data = {
#embed "../data/release.json"
    };

    test_refl(data);
    test_mutate(data);
    test_direct();

    tests::exit();
}