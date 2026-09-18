#include <filesystem>
#include <fstream>
#include <print>
#include <utility>
#include <vector>

#include "yyjson.h"
#include <sstream>

import jacinth;

struct Asset {
    std::string name;
    std::size_t size;
    std::string digest;
    std::string created_at;
    std::string browser_download_url;
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
    auto json = buf.str();

    yyjson_doc *doc = yyjson_read(json.c_str(), json.size(), 0);
    if (!doc)
        return 1;

    yyjson_val *root = yyjson_doc_get_root(doc);
    Release release;

    jacinth::parseValue(root, release);

    yyjson_doc_free(doc);

    std::println("Release {}", release.name);
    std::println("  Tag: {}", release.tag_name);
    std::println("  URL: {}", release.html_url);
    std::println("  Assets:");

    std::size_t i = 0;

    for (const auto &a : std::as_const(release.assets)) {
        std::println("    Asset {}", i);
        std::println("      Name: {}", a.name);
        std::println("      Size: {}", a.size);
        std::println("      Digest: {}", a.digest);
        std::println("      URL: {}", a.browser_download_url);

        ++i;
    }
}
