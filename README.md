# Jacinth

aka. "JSON for Postmodern C++"

Jacinth is a simple, low-overhead JSON library for C++23 and C++26 that provides a clean, easy-to-use, flexible API, without bloating the library or removing features you expect from your favorite JSON libraries.

## Features

Jacinth is (currently) a relatively thin OOP wrapper around the very fast [yyjson](https://github.com/ibireme/yyjson). It features:

- Compile-time struct reflection, enabling serialization and deserialization of arbitrarily complex struct types with low overhead
  - GCC gets access to the extremely powerful (and fast) [P2996 Reflection API](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r13.html)
  - Clang and MSVC instead will use [Boost.PFR](https://www.boost.org/doc/libs/latest/doc/html/boost_pfr.html), which isn't as fast as C++26 reflection, but is very close.
    - The included PFR is version 1.92, but anything newer than 1.80 or so should work.
- A mutable nlohmann-like JSON object API (`jacinth::json`, `jacinth::mutable_value`)
- A fast, lazy, on-demand parser for immutable objects (`jacinth::doc`, `jacinth::value`)
- Custom type parsing through nlohmann-like ADL
  - `to_json(jacinth::mutable_value json, const T& field)`
  - `from_json(const jacinth::value &json, T& field)`
- C++20 module support
- Extremely low executable size overhead
  - A basic `std::println("Hello World");` program takes up 131K with `-O3`
  - Jacinth's test executable takes up just 200-250K with `-O3`
- Low compilation speed overhead
  - On a Zen 4 desktop CPU, Jacinth's test executable compiles and links in 4 seconds
  - A similar program made with Glaze takes around 5.5 seconds
  - This is particularly helped by Jacinth supporting C++20 modules, which makes compilation significantly faster
- Super easy integration into existing projects
  - Projects that already use Glaze or nlohmann can get a near-identical API surface for quick migration
  - See [#Integration](#integration)
- Flexible custom meta specializations

## Limitations

Jacinth is intentionally designed in a limited manner that makes it as easy to use as possible. Thus, it lacks certain features and characteristics that you may otherwise expect:

- Robust error handling (for now)
- Specialized SIMD paths
- Support for binary or other non-JSON formats
  - Technically, JSONC and JSON5 are supported through yyjson, but this isn't really exposed in the C++ API right now.
- JavaScript/Python-like array/object manipulation
  - This might be added at some point
- `std::format` specializations (for now)
  - For the time being you can just `jacinth::json::dump`
- Absolutely *zero* overhead
  - Jacinth's safety semantics, error handling, and other goodies generally make this impossible.
  - However, Jacinth is still plenty fast; writing is slightly slower, but reading is *faster* than yyjson! See [#Performance](#performance).
- Singular API pattern
  - Jacinth intentionally implements several different API surfaces and patterns so you can choose which way works best. It's generally designed such that it can serve as a near drop-in replacement for nlohmann, while adding extra features to make your life even easier.

## Compiler Support

To use Jacinth, your C++ compiler and standard library *must* support, *at a minimum*:

- [P1103 C++20 Modules](https://en.cppreference.com/cpp/language/modules)
- [P0323 `std::expected`](https://en.cppreference.com/cpp/header/expected)

The in-repo test suite has a few more requirements:

- [P2286 Formatting Ranges](https://en.cppreference.com/cpp/utility/format/range_formatter)
- [P2093 `std::print`](https://en.cppreference.com/cpp/header/print)

Using C++26 compile-reflection instead of Boost.PFR also requires:

- [P2996 Reflection](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r13.html)
- [P3491 `define_static_array`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3491r3.html)
- [P1306 Expansion Statements (`template for`)](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p1306r5.html)

Thus, the following compilers are supported:

- GCC >=16 (+ reflection)
  - Versions below 16.2 may be unstable with reflection enabled. Some distributions, such as Gentoo, are unaffected, but Arch Linux and Ubuntu are known to be affected.
- Clang >= 19
  - This does NOT mean that Debian 13 is supported! Your distribution must have libstdc++15 or newer; Debian 13 is stuck on 14.
- MSVC >= 2026, or clang-cl with MSVC >= 2022
  - Technically, 2022 supports all necessary features, but is unable to compile Boost.PFR due to ICE bugs.
  - If you need to stay on 2022, use clang-cl.

Clang and MSVC do not support reflection at the time of writing.

## Licensing

This is licensed under the MIT. You are free to use and modify this as you wish, but you ***must*** include the contents of the [`LICENSE`](./LICENSE) file somewhere in your application and source tree. Attribution that your program or library uses Jacinth is highly encouraged but not legally required.

## Performance

Jacinth isn't aiming to be the fastest library out there, but it still ends up being very fast, as yyjson itself is already one of the fastest. In fact, in terms of read performance, Jacinth tends to serialize structs equal to or *faster* than Glaze or yyjson!

Writing will generally be on par with yyjson, but slower than Glaze since Jacinth currently lacks a dedicated fast struct writer like Glaze has.

In this test, `Jacinth (Parse)` refers to parsing the JSON into DOM and extracting a single value from it. These numbers and benchmarks were created with a modified version of [Stephen Berry's JSON benchmarks](https://github.com/stephenberry/json_performance).

| Library | Roundtrip Time (s) | Write (MB/s) | Read (MB/s) |
| ------- | ------------------ | ------------ | ----------- |
| [**Glaze**](https://github.com/stephenberry/glaze) | **0.90** | **1421** | **1562** |
| [**Jacinth (Struct)**](https://github.com/crueter/jacinth) | **1.02** | **1188** | **1773** |
| [**Jacinth (Parse)**](https://github.com/crueter/jacinth) | **N/A** | **N/A** | **2125** |
| [**simdjson (on demand)**](https://github.com/simdjson/simdjson) | **N/A** | **N/A** | **1962** |
| [**yyjson**](https://github.com/ibireme/yyjson) | **1.10** | **1203** | **1460** |
| [**reflect_cpp**](https://github.com/getml/reflect-cpp) | **2.35** | **778** | **448** |
| [**daw_json_link**](https://github.com/beached/daw_json_link) | **2.23** | **526** | **755** |
| [**RapidJSON**](https://github.com/Tencent/rapidjson) | **2.26** | **462** | **855** |
| [**json_struct**](https://github.com/jorgen/json_struct) | **3.68** | **373** | **356** |
| [**Boost.JSON**](https://boost.org/libs/json) | **3.98** | **283** | **436** |
| [**nlohmann**](https://github.com/nlohmann/json) | **10.14** | **150** | **111** |

In the Out-Of-Sequence test, Jacinth does see a performance penalty, but still reads very fast, unlike simdjson; keys don't necessarily need to be in the same order as the struct:

| Library | Read (MB/s) |
| ------- | ----------- |
| [**Glaze**](https://github.com/stephenberry/glaze) | **1472** |
| [**simdjson (on demand)**](https://github.com/simdjson/simdjson) | **127** |
| [**Jacinth**](https://github.com/crueter/jacinth) | **1120** |

### Maximizing Performance

Jacinth will already be fast no matter what you do, but if you need to squeeze out as much as you possibly can, you have a few options:

- If you don't need to modify a JSON document, use `jacinth::doc`, which is usually faster
- Use `std::string_view` in your structs if possible to avoid slow string allocations
  - This is technically unsafe if you don't store the owning doc (as the underlying data will be freed once you exit scope).
  - To get around this, store the doc object in the same lifecycle

```cpp
{
  Release r;
  auto doc = jacinth::doc::read(data);
  d.get_to(r);

  // use release fields...
}

// strings in Release are no longer valid
```

- Use on-demand parsing (`doc.get("value")`) if you only need a few fields from a large document
- Use the non-allocating `jacinth::json::dump_to(value, out)` instead of `dump`
- Use the non-allocating `jacinth::{doc,json}::read_to(value, data)` functions instead of `read`

## Integration

Jacinth can be used both as a system and a vendored target. It can be integrated simply via `find_package(jacinth)` or `add_subdirectory(jacinth)`, respectively.

Jacinth creates a `jacinth::jacinth` target, so you can link to Jacinth via `target_link_libraries(MyApp PRIVATE jacinth::jacinth)`. You usually shouldn't need Jacinth within your header files, so internal targets generally won't need `PUBLIC` propagation. Both the system and vendored targets provide a `JACINTH_USE_REFLECTION` option. Enabling this will add `-freflection` to your compile options, force the C++26 standard, and define `JACINTH_USE_REFLECTION` in the module.

In order for Jacinth to properly compile and for `import jacinth;` to work, you MUST turn on `CMAKE_CXX_SCAN_FOR_MODULES`, and use a supported generator. Ninja is recommended. For `clang-cl`, CMake 4.4 or newer is required and the Visual Studio generator is not supported. Obviously you'll need a modern compiler too; GCC >=16, Clang >=19, and MSVC >=2022 should all work fine.

### Adding Jacinth to your project

You can use `FetchContent`, [CPMUtil](https://github.com/crueter/CPMUtil), a Git submodule, or any other dependency vendoring solution of your choice. You should probably use `find_package(jacinth)` as well (CPMUtil will handle this for you).

### Caveats

As with all modules, you *must* ensure that the target you're importing the module into has more or less the same compiler flags as Jacinth. If you set a bunch of additional flags e.g. `-fwrapv` or `-fno-rtti` to your source files, Jacinth must also have these applied.

Linking to `jacinth::jacinth` will automatically enable C++26 and reflection features if you have `JACINTH_USE_REFLECTION` on, but you may wish to enable this for your entire project if using Jacinth's reflection features.

## Usage

Abstract:

- Use `jacinth::doc` for *immutable* JSON documents, aka those which you don't intend to edit/mutate.
- Use `jacinth::json` for *mutable* JSON documents, aka those which you intend to edit OR create from scratch. It's slower than `jacinth::doc`, so if your application is speed-conscious you should use that when possible.
- Parse into a JSON or struct object with `jacinth::{doc,json}::read(string)`
- Dump any JSON value with `jacinth::{doc,json}::dump(object)`, or `object.dump` on a Jacinth JSON value (doc, json, value, mutable_value)
- Convert JSON documents to any type you wish
  - Implicitly: `MyStruct my_object = jacinth::doc::read(data);`
  - Explicitly: `auto my_object = jacinth::doc::read<MyStruct>(data);`
  - Explicitly, with `std::expected` error handling: `auto my_object = jacinth::doc::try_read<MyStruct>(data);`
- Convert JSON values to any type you wish
  - Implicitly: `uint64_t number = doc["number"];` or `doc.get("number");`
  - Explicitly: `auto number = doc["number"].get<uint64_t>();` or `doc.get<uint64_t>("number");`
  - Explicitly, with error handling: `auto number = doc["number"].try_get<uint64_t>();` or `doc.try_get<uint64_t>("number");`
- Read, write, and create arrays and objects
  - Read arrays: `auto number = doc["numbers"].get<uint64_t>(5);`
  - Write arrays: `json["numbers"][4] = 80000;`
  - Read nested objects and arrays: `auto name = doc["meetings"][3]["people"][0].get<std::string>("name");`
  - Write nested objects and arrays: `json["meetings"][3]["people"][0]["name"] = "Bella";`

Below are a few usage examples and tricks. Most of these can be found in the [test suite](./test).

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
    std::println("      Digest: {}", a.digest.value_or("Not Present"));
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

### Advanced Usage

A bunch more documented use cases can be found in [USAGE](./USAGE.md).
