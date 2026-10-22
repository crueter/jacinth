// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <array>
#include <print>
#include <span>
#include <string_view>
#include <vector>

import jacinth;
import tests;

// [WARNING: MOSTLY AI SLOP]

// numeric vectors
void test_numvec()
{
    auto vi = jacinth::doc::try_read<std::vector<int>>(R"([10, 20, -30, 40])");
    tests::check("fast path vector<int>", vi && vi->size() == 4 && (*vi)[0] == 10 &&
                                              (*vi)[1] == 20 && (*vi)[2] == -30 && (*vi)[3] == 40);

    auto vu = jacinth::doc::try_read<std::vector<unsigned>>(R"([1, 2, 3])");
    tests::check("fast path vector<unsigned>",
                 vu && vu->size() == 3 && (*vu)[0] == 1u && (*vu)[2] == 3u);

    auto vd = jacinth::doc::try_read<std::vector<double>>(R"([1, 2, 3])");
    tests::check("fast path vector<double> from integer JSON",
                 vd && vd->size() == 3 && (*vd)[0] == 1.0 && (*vd)[2] == 3.0);

    auto vf = jacinth::doc::try_read<std::vector<float>>(R"([1.5, 2.5, 3.5])");
    tests::check("fast path vector<float>",
                 vf && vf->size() == 3 && (*vf)[0] == 1.5f && (*vf)[2] == 3.5f);

    auto ve = jacinth::doc::try_read<std::vector<int>>(R"([])");
    tests::check("fast path empty vector<int>", ve && ve->size() == 0);

    // mixed-type array must report type_mismatch
    tests::check_errc("fast path vector<int> from [1,\"two\"]",
                      jacinth::doc::try_read<std::vector<int>>(R"([1, "two"])"),
                      jacinth::errc::type_mismatch);

    // std::array uses the same merged path: exactly N, capped when longer
    auto ai = jacinth::doc::try_read<std::array<int, 3>>(R"([10, 20, 30])");
    tests::check("fast path array<int,3>", ai && (*ai)[0] == 10 && (*ai)[2] == 30);

    auto ac = jacinth::doc::try_read<std::array<int, 3>>(R"([1, 2, 3, 4, 5])");
    tests::check("fast path array<int,3> truncates extras",
                 ac && (*ac)[0] == 1 && (*ac)[1] == 2 && (*ac)[2] == 3);

    auto ad = jacinth::doc::try_read<std::array<double, 2>>(R"([1, 2])");
    tests::check("fast path array<double,2> from integer JSON",
                 ad && (*ad)[0] == 1.0 && (*ad)[1] == 2.0);

    // static-extent span reads into caller-provided storage
    std::array<int, 3> store{};
    std::span<int, 3> sp{store};
    auto sr = jacinth::doc::try_read_to(sp, R"([4, 5, 6])");
    tests::check("fast path span<int,3> read_to",
                 sr && store[0] == 4 && store[1] == 5 && store[2] == 6);
}

int main() {
    test_numvec();

    tests::exit();
}