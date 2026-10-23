# Jacinth Usage

This contains all presently documented use cases for Jacinth.

- [Jacinth Usage](#jacinth-usage)
  - [Simple struct serialization](#simple-struct-serialization)
  - [Simple struct deserialization](#simple-struct-deserialization)
  - [Create a JSON from scratch](#create-a-json-from-scratch)
  - [On-demand JSON document parsing](#on-demand-json-document-parsing)
  - [Pretty-print](#pretty-print)
  - [Maps](#maps)
  - [ADL](#adl)
  - [Iteration](#iteration)
  - [Error Handling](#error-handling)
  - [Meta Specializations](#meta-specializations)
    - [Rename](#rename)
    - [From](#from)
    - [Exclude](#exclude)
    - [Include](#include)
    - [Flatten](#flatten)
    - [Full Example](#full-example)
  - [Other Fun Stuff](#other-fun-stuff)
    - [Accessing `jacinth::value` and `jacinth::mutable_value`](#accessing-jacinthvalue-and-jacinthmutable_value)

## Simple struct serialization

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

## Simple struct deserialization

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

## Create a JSON from scratch

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

## On-demand JSON document parsing

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

## Pretty-print

You can use the `jacinth::write_opts` struct to pretty print:

```cpp
std::println("{}", jacinth::json::dump(asset, {.pretty = true}));
```

## Maps

`std::map` serialization is supported, both in aggregate structs *and* directly. Also note here that you can directly serialize *any* supported type with `dump`, including standard types/containers.

```cpp
std::map<std::string, int> people = {
    {"James", 23},
    {"Kayla", 76},
    {"Thomas", 51}
};

std::println("{}", jacinth::json::dump(people));
```

## ADL

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

## Iteration

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

## Error Handling

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

## Meta Specializations

Sometimes, the way you write your struct may not match exactly with how you expect the JSON to be. In this case, you can use Jacinth's meta specification API to shift around how fields will be serialized and deserialized.

This example is taken from [`test/meta.cpp`](./test/meta.cpp). The types that will be used are:

```cpp
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
```

To start, define the meta specialization itself:

```cpp
template <>
inline constexpr jacinth::obj jacinth::meta<Room> = {};
```

You have now defined a blank specialization (aka, the default).

### Rename

My room JSON uses `name` to refer to the room's title. However, my room struct uses `title`, since I prefer that (or a consumer mandates it). I can use `jacinth::rename` to rename `title` to `name`, and vice versa. Inside of the meta specialization tuple, add:

```cpp
jacinth::rename("title", "name"),
```

When dumping to JSON, the room's `title` field will be written as `name`. When parsing a JSON, the `name` field will be written into the room's `title`.

### From

My room's JSON specifies `gameName` at the root. However, I store game information in a `GameInfo` struct. Inside of the meta specialization tuple, add:

```cpp
jacinth::from("gameName", [](auto &t) -> auto & { return t.game.name; }),
```

Notice the `auto &` used twice; **this MUST be a reference**! It will not compile otherwise.

When dumping to JSON, the `name` field of the room's `game` field will be written as `gameName` to the root. When parsing a JSON, the `gameName` field will be written into the `name` field of the room's `game` field.

You can also use specific indexes of member vectors/arrays, but this can be fragile, so be careful!

```cpp
jacinth::from("owner", [](auto &t) -> auto & { return t.members[0]; }),
```

### Exclude

My room JSON doesn't include the `id` or `creationTime` fields. I also don't want the `game` field to be serialized either, since I already define `gameName`. I want to exclude those:

```cpp
jacinth::exclude("id", "creationTime", "game"),
```

When dumping to JSON, those fields will not be written. When parsing a JSON, those fields of `room` will not be written to.

### Include

If you only need to read/write a few fields from a large document/struct, you may instead opt to use `jacinth::include`, which will exclude all fields *except for* the fields you specifically include. In this case, I only need my `from` fields, `members`, and `name`:

```cpp
jacinth::include("name", "members"),
```

When dumping to JSON, only `name`, `members,` and the `from` fields will be written. When parsing a JSON, only those fields will be read (with `name` getting read into `title`, as before).

> [!NOTE]
> Fields that have `jacinth::from` specializations are unaffected by `exclude`/`include`, and will always be included.
>
> Renamed fields will use the specified JSON key, *not* the member name. In this example, excluding `title` would do nothing, whereas excluding `name` would properly exclude that field.
>
> You should also avoid defining both `exclude` and `include`. Currently `exclude` will win but this is undefined behavior and subject to change at any point.

### Flatten

If a struct contains one or more child structs, and you wish to *flatten* those child structs--that is, write their fields directly to the root--use `jacinth::flatten`:

```cpp
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
// serializes to: {height, weight, eyeColor, skinColor, hairColor, name}
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
```

A `Person` struct will then output as:

```json
{
    "height": 75.5,
    "weight": 196.3,
    "eyeColor": "blue",
    "skinColor": "white",
    "hairColor": "blond",
    "name": "Marcus"
}
```

Reading this JSON back will properly fill the `characteristics` and `colors` structs as well.

### Full Example

```cpp
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
    jacinth::include("name", "members"),
    jacinth::from("gameName", [](auto &t) -> auto & { return t.game.name; }),
    jacinth::from("owner", [](auto &t) -> auto & { return t.members[0]; }),
};
```

## Other Fun Stuff

### Accessing `jacinth::value` and `jacinth::mutable_value`

The `get`/`try_get` methods and the `[]` operator will always try to convert to a reasonable type, given the variable they are being assigned to. However, you can leave this as a `jacinth::value` or `jacinth::mutable_value` by assigning to an `auto` variable:

```cpp
// these will be jacinth::value
auto url_v = doc.get("html_url");
auto asset_0_v = doc.get("assets")[0];
```
