// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

// Global import
module;

#include <vector>

export module jacinth;

export import :read;
export import :write;
export import :iter;

// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=122609
export {
    using std::vector;
}