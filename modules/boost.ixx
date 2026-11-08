// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

// Small boost wrapper

module;

#ifndef JACINTH_USE_REFLECTION
#include "boost/pfr/core_name.hpp"
#endif

module jacinth:boost;

#ifndef JACINTH_USE_REFLECTION
export namespace pfr {

using boost::pfr::for_each_field_with_name;
using boost::pfr::names_as_array;

};
#endif
