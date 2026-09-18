# Jacinth

aka. "JSON for Postmodern C++"

An experimental JSON library that aims to be incredibly simple and easy to use, without sacrificing speed or code size.

## TODOs

This library will essentially aim to be a wrapper around [yyjson](https://github.com/ibireme/yyjson) (one of the fastest JSON libraries in the world!). The public API and design patterns are not yet decided, but at a minimum, Jacinth will support:

- nlohmann-like in-place JSON object API
- Glaze/Reflect-like compile-time struct reflection
- JSON serialization *and* deserialization
- C++20 module support, possibly with `import std`
- Absolutely zero dependencies for consumers
  - Sadly this isn't really possible for system installs
  - But vendored targets should just be able to pull in the repository and configure it as-is, and not even need yyjson or boost-pfr

Nice to haves that are not guaranteed, but may be added in the future:

- JS-like append/remove API, and other operator overload sauce
- C++20 `std::format` specializations for JSON objects
- Deep, robust error handling
