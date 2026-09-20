# Jacinth

aka. "JSON for Postmodern C++"

Jacinth is a simple, low-overhead JSON library for C++23 and C++26 that provides a clean, easy-to-use, flexible API, without bloating the library or removing features you expect from your favorite JSON libraries.

## What it has

Jacinth is (currently) a relatively thin OOP wrapper around the very fast [yyjson](https://github.com/ibireme/yyjson). It features:

- Compile-time struct reflection, enabling serialization and deserialization of arbitrarily complex struct types with low overhead
  - GCC 16 and up get access to the extremely powerful (and fast) [P2996 Reflection API](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r13.html)
  - Clang, MSVC, and GCC <=15 instead will use [Boost.PFR](https://www.boost.org/doc/libs/latest/doc/html/boost_pfr.html), which isn't as fast as C++26 reflection, but is very close.
- A mutable nlohmann-like JSON object API (`jacinth::json`, `jacinth::mutable_value`)
- A fast, lazy, on-demand parser for immutable objects (`jacinth::doc`, `jacinth::value`)
- Custom type parsing through nlohmann-like ADL
  - `to_json(jacinth::mutable_value json, const T& field)`
  - `from_json(const jacinth::value &json, T& field)`
- C++20 module support
- Extremely low executable size overhead
  - A basic `std::println("Hello World");` program takes up 131K with `-O3`
  - Jacinth's test executable takes up just 175K with `-O3`
    - Clang-built executables *do* take up a bit more space, but not by much (250K)
- Low compilation speed overhead
  - On a Zen 4 desktop CPU, Jacinth's test executable compiles and links in 3.5 seconds
  - A similar program made with Glaze takes around 5.5 seconds
  - This is particularly helped by Jacinth supporting C++20 modules, which makes compilation significantly faster
- Super easy integration into existing projects
  - Projects that already use Glaze or nlohmann can get a near-identical API surface for quick migration
  - See [#Integration](#integration)

## What it doesn't have

Jacinth is intentionally designed in a limited manner that makes it as easy to use as possible. Thus, it lacks certain features and characteristics that you may otherwise expect:

- Robust error handling (for now)
- Specialized SIMD paths
- Support for binary or other non-JSON formats
  - Technically, cJSON and JSON5 are supported through yyjson, but this isn't super supported right now
- std::variant support (for now)
- JavaScript/Python-like array/object manipulation
  - This might be added at some point
- `std::format` specializations (for now)
  - For the time being you can just `jacinth::json::dump`

## Performance

Jacinth isn't aiming to be the fastest library out there, but it still ends up being relatively fast because yyjson itself is already one of the fastest. C++ safety features and containers do end up adding measurable overhead, but generally, Jacinth will always be around 80-90% as fast as yyjson (and *faster* than yyjson in certain on-demand parsing cases).

In this test, `Jacinth (Parse)` refers to parsing the JSON into DOM and extracting a single value from it. These numbers and benchmarks were created with a modified version of [Stephen Berry's JSON benchmarks](https://github.com/stephenberry/json_performance).

| Library | Roundtrip Time (s) | Write (MB/s) | Read (MB/s) |
| ------- | ------------------ | ------------ | ----------- |
| [**Glaze**](https://github.com/stephenberry/glaze) | **0.87** | **1425** | **1653** |
| [**simdjson (on demand)**](https://github.com/simdjson/simdjson) | **N/A** | **N/A** | **1995** |
| [**yyjson**](https://github.com/ibireme/yyjson) | **1.15** | **1125** | **1429** |
| [**Jacinth (Struct)**](https://github.com/crueter/jacinth) | **1.42** | **988** | **1293** |
| [**Jacinth (Parse)**](https://github.com/crueter/jacinth) | **N/A** | **N/A** | **2133** |
| [**reflect_cpp**](https://github.com/getml/reflect-cpp) | **2.35** | **778** | **448** |
| [**daw_json_link**](https://github.com/beached/daw_json_link) | **2.23** | **526** | **755** |
| [**RapidJSON**](https://github.com/Tencent/rapidjson) | **2.26** | **462** | **855** |
| [**json_struct**](https://github.com/jorgen/json_struct) | **3.68** | **373** | **356** |
| [**Boost.JSON**](https://boost.org/libs/json) | **3.98** | **283** | **436** |
| [**nlohmann**](https://github.com/nlohmann/json) | **10.14** | **150** | **111** |

In the Out-Of-Sequence test, Jacinth still performs very well (~80% as fast as the standard benchmark). Unlike simdjson, it doesn't *need* keys to be in the same order as expected; reflection can handle this case just fine.

| Library | Read (MB/s) |
| ------- | ----------- |
| [**Glaze**](https://github.com/stephenberry/glaze) | **1456** |
| [**simdjson (on demand)**](https://github.com/simdjson/simdjson) | **134** |
| [**Jacinth**](https://github.com/crueter/jacinth) | **1033** |

### Maximizing Performance

Jacinth will already be fast no matter what you do, but if you need to squeeze out as much as you possibly can, you have a few options:

- Try to prefer `std::string_view`, *static-extent* `std::span`, or `std::array` in your structs if possible. This means the data will only be valid while the document object is alive, but if you only access/use the data within one scope, this is fine.
  - It isn't possible to parse into dynamic-extent `std::span` containers, so use `std::vector` if you don't know what the size will be. If the size is a constant, static-extent `std::span` will work fine.
- Use on-demand parsing (`doc["value"]`) if you only need a few fields from the document
- Use the non-allocating `jacinth::json::dump_to` instead of `dump`

## Integration

Jacinth can be used both as a system and a vendored target. It can be integrated simply via  `find_package(jacinth)` or `add_subdirectory(jacinth)`, respectively.

Jacinth creates a `jacinth::jacinth` target, so you can link to Jacinth via `target_link_libraries(MyApp PRIVATE jacinth::jacinth)`. You usually shouldn't need Jacinth within your header files, so internal targets generally won't need `PUBLIC` propagation. Both the system and vendored targets provide a `JACINTH_USE_REFLECTION` option. Enabling this will add `-freflection` to your compile options, force the C++26 standard, and define `JACINTH_USE_REFLECTION` in the module.

In order for Jacinth to properly compile and for `import jacinth;` to work, you MUST turn on `CMAKE_CXX_SCAN_FOR_MODULES`, and use a supported generator (e.g. Ninja or Visual Studio). Obviously you'll need a modern compiler too; GCC >=15, Clang >=19, and MSVC >=2022 should all work fine.

### Caveats

As with all modules, you *must* ensure that the target you're importing the module into has more or less the same compiler flags as Jacinth. If you set a bunch of additional flags e.g. `-fwrapv` or `-fno-rtti` to your source files, Jacinth must also have these applied. Linking to `jacinth::jacinth` will automatically enable C++26 and reflection features if you have `JACINTH_USE_REFLECTION` on, but you may wish to enable this for your entire project if using Jacinth's reflection features anyways.

## Usage

A few usage examples and tricks. Most of these can be found in [`test/main.cpp`](test/main.cpp).

### Simple struct serialization

TL;DR: `jacinth::doc::read`.

Jacinth natively supports most standard types, including vector containers. For instance, a release containing assets can be cleanly parsed via `std::vector<Asset>`. Also note the usage of `std::optional`; if you're not entirely sure a value will be present within a document, you can use this to deterministically check for *existence* of a value, rather than relying on default/empty values.

```cpp
struct Asset {
    std::string name;
    std::size_t size;
    std::optional<std::string> digest;
    std::string browser_download_url;
};

struct Release {
    std::string tag_name;
    std::string name;
    std::string html_url;
    std::string body;
    std::vector<Asset> assets;
};

// ...
Release release = jacinth::doc::read(data);

std::println("Release {}", release.name);
std::println("  Tag: {}", release.tag_name);
std::println("  URL: {}", release.html_url);
std::println("  Assets:");

for (const auto &a : std::as_const(release.assets)) {
    std::println("    Asset {}", a.name);
    std::println("      Name: {}", a.name);
    std::println("      Digest: {}", a.digets.value_or("Not Present"));
}
```

### Simple struct deserialization

TL;DR: `jacinth::json::dump`.

```cpp
Asset asset = {
    .name = "Test-Asset.tar.gz",
    .size = 1000,
    .digest = "sha256:abcdef1234567890",
    .browser_download_url = "https://example.com"
};

std::println("Asset: {}", jacinth::json::dump(asset));
```

You can also use the non-allocating `dump_to`:

```cpp
std::string dumped;
jacinth::json::dump_to(asset, dumped);
```

### Create a JSON from scratch

You can create JSON documents and values from scratch via `jacinth::json`. You can also call `dump` or `dump_to` directly on a `jacinth::json`:

```cpp
jacinth::json json;
json["name"] = "Jacinth";
json["creator"] = "crueter";
json["features"] = {
    "Modules",
    "Reflection",
    "OOP API"
};

std::println("Jacinth: {}", json.dump());
```

### On-demand JSON document parsing

Jacinth supports high-speed lazy DOM parsing, which is useful if you only need a few fields in a large JSON document.

Note the `as_array` in this example; see more of that in [#Iteration](#iteration).

```cpp
auto doc = jacinth::doc::read(data);

std::println("Release {}", json["name"].as<std::string>());
std::println("  Tag: {}", json["tag_name"].as<std::string>());
std::println("  URL: {}", json["html_url"].as<std::string>());
std::println("  Assets:");

auto assets = json["assets"];

for (auto a : assets.as_array()) {
    std::println("    Asset: {}", a["name"].as<std::string>());
}
```

Also note the `as<std::string>()` here. Generally speaking, this is only necessary for `std::string` or other cases where an assignment may be ambiguous. In this case, the compiler isn't able to automatically discern whether to use `std::string_view`, `const char*`, or any of the other implicit conversions into `std::string`. For most primitives, it should just work without any hitches.

### Pretty-print

You can use `jacinth::write_opts` to pretty print:

```cpp
std::println("{}", jacinth::json::dump(asset, {.pretty = true}));
```

### Maps

`std::map` serialization is supported, both in aggregate structs *and* directly. Also note here that you can directly serialize *any* supported type with `dump`, including standard types/containers.

```cpp
std::map<std::string, int> people = {
    {"James", 23},
    {"Kayla", 76},
    {"Thomas", 51}
};

std::println("{}", jacinth::json::dump(people));
```

### ADL

Jacinth supports nlohmann-like ADL `from_json`/`to_json` for custom serialization. These must be in the same namespace as the type you are writing custom de/serializers for.

```cpp
namespace {

struct CustomStruct {
    std::string name;
};

void to_json(jacinth::mutable_value json, const CustomStruct &custom) {
    json["derived"] = std::format("Derived value from to_json: {}", custom.name);
}

void from_json(const jacinth::value &json, CustomStruct &custom) {
    custom.name = std::format("Derived value from from_json: {}", json["name"].as<std::string>());
}

}

// ...

CustomStruct s = {"CustomStruct Test"};
std::println("to_json: {}", jacinth::json::dump(s));

const auto json_str = "{\"name\": \"CustomStruct JSON\"}";
CustomStruct newCustom = jacinth::json::read(json_str);
std::println("from_json: {}", newCustom.name);

/* Output:
to_json: {"derived":"Derived value from to_json: CustomStruct Test"}
from_json: Derived value from from_json: CustomStruct JSON
*/
```

### Iteration

Jacinth supports iteration for array and object values. These are achieved through `as_array()` or `as_object()` on a document or value. Take the earlier [JSON from scratch](#create-a-json-from-scratch) example:

```cpp
jacinth::json json;
json["name"] = "Jacinth";
json["creator"] = "crueter";
json["features"] = {
    "Modules",
    "Reflection",
    "OOP API"
};

for (auto [k, v] : json.as_object()) {
    std::println("{}: {}", std::string(k), jacinth::json::dump(v));
}
```

This will output:

```txt
name: Jacinth
creator: crueter
features: ["Modules","Reflection","OOP API"]
```

Also notice that `dump`/`dump_to` directly support Jacinth's JSON value types. This is useful because it allows you to print any arbitrary JSON value without the need for explicit type conversions (in this example you would have to check conversion to `std::vector`).
