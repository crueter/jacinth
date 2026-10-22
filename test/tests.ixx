module;

#include <concepts>
#include <string_view>
#include <string>
#include <print>
#include <expected>
#include <system_error>
#include <type_traits>

export module tests;

import jacinth;

export namespace tests {

inline std::size_t failed = 0;
inline std::size_t passed = 0;

inline std::string normalize_newlines(std::string_view text)
{
    std::string normalized{text};
    std::erase(normalized, '\r');
    return normalized;
}

// TODO USE A REAL TEST FRAMEWORK
inline bool check(std::string_view test, bool condition)
{
    if (condition) {
        std::println("PASS: {}", test);
        ++passed;
    } else {
        std::println("FAIL: {}", test);
        ++failed;
    }

    return condition;
}

template <typename T, typename J>
    requires std::equality_comparable_with<T, J>
void check_match(std::string_view test, T actual, J expect) {
    if (!check(std::format("{} == {}", test, expect), actual == expect)) {
        std::println("Got: {}", actual);
    }
}

template <typename T>
void check_errc(std::string_view test, std::expected<T, std::error_code> actual, jacinth::errc expect) {
    jacinth::errc err = actual ? jacinth::errc::ok
                               : jacinth::errc(actual.error().value());

    if (!check(test, err == expect)) {
        std::println("  expected {}, got {}", int(expect), int(err));
    }
}

[[noreturn]] inline void exit() {
    std::println("-----------------------------------");
    std::println("TESTS RAN: {} PASSED: {} FAILED: {}", passed + failed, passed, failed);
    std::println("-----------------------------------");

    std::exit(int(failed));
}

} // namespace tests
