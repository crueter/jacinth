# Jacinth

aka. "JSON for Postmodern C++"

Jacinth is a simple, low-overhead JSON library for C++23 and C++26 that provides a clean, easy-to-use, flexible API, without bloating the library or removing features you expect from your favorite JSON libraries.

## Features

Jacinth is (currently) a relatively thin OOP wrapper around the very fast [yyjson](https://github.com/ibireme/yyjson). It features:

- Compile-time struct reflection, enabling serialization and deserialization of arbitrarily complex struct types with low overhead
  - GCC 16 and up get access to the extremely powerful (and fast) [P2996 Reflection API](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r13.html)
  - Clang, MSVC, and GCC <=15 instead will use [Boost.PFR](https://www.boost.org/doc/libs/latest/doc/html/boost_pfr.html), which isn't as fast as C++26 reflection, but is very close.
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
  - However, Jacinth is still plenty fast, usually between 80-90% of yyjson. See [#Performance](#performance).
  - Generally, parse/read is nearly as fast as yyjson, but writing is usually around 20% slower due to memory safety shenanigans.
- Singular API pattern
  - Jacinth intentionally implements several different API surfaces and patterns so you can choose which way works best. It's generally designed such that it can serve as a near drop-in replacement for nlohmann, while adding extra features to make your life even easier.

## Licensing

I don't know yet. It will *probably* be MIT. I want to make it LGPL, but that has a lot of issues when it comes to C++20 modules. For now assume MIT until licensing is finished.

## Performance

Jacinth isn't aiming to be the fastest library out there, but it still ends up being very fast, as yyjson itself is already one of the fastest. In fact, in terms of read performance, Jacinth tends to serialize structs equal to or *faster* than Glaze or yyjson!

Writing has lots of extra bounds and safeguards that currently slow it down, but a faster JSON serializer for structs will be written in the future.

In this test, `Jacinth (Parse)` refers to parsing the JSON into DOM and extracting a single value from it. These numbers and benchmarks were created with a modified version of [Stephen Berry's JSON benchmarks](https://github.com/stephenberry/json_performance).

| Library | Roundtrip Time (s) | Write (MB/s) | Read (MB/s) |
| ------- | ------------------ | ------------ | ----------- |
| [**Glaze**](https://github.com/stephenberry/glaze) | **0.85** | **1449** | **1609** |
| [**Jacinth (Struct)**](https://github.com/crueter/jacinth) | **1.06** | **1147** | **1622** |
| [**Jacinth (Parse)**](https://github.com/crueter/jacinth) | **N/A** | **N/A** | **2161** |
| [**simdjson (on demand)**](https://github.com/simdjson/simdjson) | **N/A** | **N/A** | **1996** |
| [**yyjson**](https://github.com/ibireme/yyjson) | **1.09** | **1237** | **1486** |
| [**reflect_cpp**](https://github.com/getml/reflect-cpp) | **2.35** | **778** | **448** |
| [**daw_json_link**](https://github.com/beached/daw_json_link) | **2.23** | **526** | **755** |
| [**RapidJSON**](https://github.com/Tencent/rapidjson) | **2.26** | **462** | **855** |
| [**json_struct**](https://github.com/jorgen/json_struct) | **3.68** | **373** | **356** |
| [**Boost.JSON**](https://boost.org/libs/json) | **3.98** | **283** | **436** |
| [**nlohmann**](https://github.com/nlohmann/json) | **10.14** | **150** | **111** |

In the Out-Of-Sequence test, Jacinth does see a performance penalty, but still reads very fast, unlike simdjson:

| Library | Read (MB/s) |
| ------- | ----------- |
| [**Glaze**](https://github.com/stephenberry/glaze) | **1440** |
| [**simdjson (on demand)**](https://github.com/simdjson/simdjson) | **135** |
| [**Jacinth**](https://github.com/crueter/jacinth) | **1035** |

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

In order for Jacinth to properly compile and for `import jacinth;` to work, you MUST turn on `CMAKE_CXX_SCAN_FOR_MODULES`, and use a supported generator (e.g. Ninja or Visual Studio). Obviously you'll need a modern compiler too; GCC >=15, Clang >=19, and MSVC >=2022 should all work fine.

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

Below are a few usage examples and tricks. Most of these can be found in [`test/main.cpp`](test/main.cpp).

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

You can also create arrays from scratch. If you access a currently out-of-bound index, the array will be filled with nulls until it reaches your desired index:

```cpp
jacinth::json json;
json["hello"]["nested"][3] = 15.0;

std::println("{}", json.dump());
```

Outputs:

```json
{"hello":{"nested":[null,null,null,15.0]}}
```

### On-demand JSON document parsing

Jacinth supports high-speed lazy DOM parsing, which is useful if you only need a few fields in a large JSON document.

Note the `as_array` in this example; see more of that in [#Iteration](#iteration).

```cpp
auto doc = jacinth::doc::read(data);

std::println("Release {}", doc.get<std::string>("name"));
std::println("  Tag: {}", doc.get<std::string>("tag_name"));
std::println("  URL: {}", doc.get<std::string>("html_url"));
std::println("  Assets:");

auto assets = doc["assets"];

for (auto a : assets.as_array()) {
    std::println("    Asset: {}", a.get<std::string>("name"));
}
```

Also note the `get<std::string>("key")` here.

If `doc["name"]` were instead assigned to a variable, `std::string name = doc["name"]` would automatically convert to an `std::string`, so you wouldn't need the template operator. In the future a `std::formatter` specialization will be added for JSON types so that won't be necessary for print/format either.

More docs on conversions like this will come at a later date. Fun fact in the meantime: you can interpret an entire JSON document as any type you want, and this works with `jacinth::json::dump`!

### Pretty-print

You can use the `jacinth::write_opts` struct to pretty print:

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
    custom.name = std::format("Derived value from from_json: {}", json.get<std::string>("name"));
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
    std::println("{}: {}", std::string(k), v.dump());
}
```

This will output:

```txt
name: Jacinth
creator: crueter
features: ["Modules","Reflection","OOP API"]
```

Also notice that `dump`/`dump_to` directly support Jacinth's JSON value types. This is useful because it allows you to print any arbitrary JSON value without the need for explicit type conversions (in this example you would have to check conversion to `std::vector`).

### Error Handling

Jacinth allows consumers to opt-in to error handling for invalid JSONs, missing values, mismatched types, etc. The default `read`/`get` methods will just ignore these errors and give a blank/default-constructed value. This is useful in cases where you know the JSON will be exactly in the schema you expect, and file and I/O errors are already accounted for.

However, you may choose to opt into `std::expected`-based error handling. The primary differences are:

- `read` -> `try_read`
- `get<T>` -> `try_get<T>`
- Implicit type conversion from `json["key"]` -> `json["key"].try_get<T>`, or `json.try_get<T>("key")`

Currently, write operations don't have error handling. This is simply due to the sheer amount of overloads this would require. The library internals *do* support this so I'll get to adding it eventually.

These examples use a `auto ... = read<T>` pattern as opposed to the previous example's `T ... = read` pattern. `try_` methods are better off using explicit templating, so the compiler can automatically deduce `std::expected<T, std::error_code>` (and so you don't have to type that out each time).

`try_read`:

```cpp
auto release_data = /* read release.json */;
auto maybe_car = jacinth::json::try_read<Car>(release_data);

if (maybe_car) {
    std::println("Got car: {}", jacinth::json::dump(maybe_car.value()));
} else {
    std::println("Expected car: {}", maybe_car.error().message());
}
```

`try_get`:

```cpp
auto maybe_release = jacinth::json::try_read(release_data);

if (!maybe_release) {
    // ...
} else {
    auto release = maybe_release.value();
    auto maybe_name = release["created_at"].try_get<std::string>();
    // or...
    auto maybe_name = release.try_get<std::string>("created_at");
    if (!maybe_name) {
        std::println("created_at in unexpected format: expected string, "
          "got {}", jacinth::json::dump(release["created_at"]));
    }
}
```

### Other Fun Stuff

#### Accessing `jacinth::value` and `jacinth::mutable_value`

The `get`/`try_get` methods and the `[]` operator will always try to convert to a reasonable type, given the variable they are being assigned to. However, you can leave this as a `jacinth::value` or `jacinth::mutable_value` by assigning to an `auto` variable:

```cpp
// these will be jacinth::value
auto url_v = doc.get("html_url");
auto asset_0_v = doc.get("assets")[0];
```
