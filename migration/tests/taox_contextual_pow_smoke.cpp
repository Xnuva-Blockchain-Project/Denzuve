// TaoX ancestry-aware PoW selection prototype.
// Laboratory simulation only. DO NOT RELEASE.
//
// IMPORTANT:
// AncestryStatus::Verified is supplied by this test.
// This code does not authenticate blockchain ancestry.

#include <tao/consensus/pow_algorithm.h>

#include <iostream>
#include <limits>

namespace lab {

using taox::consensus::PoWAlgorithm;
using taox::consensus::SelectPoWAlgorithm;

enum class AncestryStatus {
    Verified,
    Unknown,
    Invalid
};

enum class Decision {
    UseScrypt,
    Defer,
    Reject
};

Decision Select(int height, AncestryStatus ancestry)
{
    if (height < 0)
        return Decision::Reject;

    switch (ancestry) {
        case AncestryStatus::Unknown:
            return Decision::Defer;

        case AncestryStatus::Invalid:
            return Decision::Reject;

        case AncestryStatus::Verified: {
            const auto algorithm = SelectPoWAlgorithm(height);

            if (!algorithm)
                return Decision::Reject;

            if (*algorithm == PoWAlgorithm::Scrypt)
                return Decision::UseScrypt;

            return Decision::Reject;
        }
    }

    // Reject unexpected enum values.
    return Decision::Reject;
}

} // namespace lab

int main()
{
    using lab::AncestryStatus;
    using lab::Decision;
    using lab::Select;

    auto check = [](int height,
                    AncestryStatus ancestry,
                    Decision expected) {
        return Select(height, ancestry) == expected;
    };

    for (int height : {0, 1, 636, 637, 1000, 710000}) {
        if (!check(height,
                   AncestryStatus::Verified,
                   Decision::UseScrypt))
            return 1;

        if (!check(height,
                   AncestryStatus::Unknown,
                   Decision::Defer))
            return 2;

        if (!check(height,
                   AncestryStatus::Invalid,
                   Decision::Reject))
            return 3;
    }

    std::cout << "PASS: verified ancestry selects Scrypt\n";
    std::cout << "PASS: unknown ancestry is deferred\n";
    std::cout << "PASS: invalid ancestry is rejected\n";

    if (!check(-1, AncestryStatus::Verified,
               Decision::Reject))
        return 4;

    if (!check(-1, AncestryStatus::Unknown,
               Decision::Reject))
        return 5;

    if (!check(1000,
               static_cast<AncestryStatus>(999),
               Decision::Reject))
        return 6;

    if (!check(std::numeric_limits<int>::max(),
               AncestryStatus::Verified,
               Decision::UseScrypt))
        return 7;

    std::cout << "PASS: malformed context rejected\n";
    std::cout << "PASS: RandomX remains inactive\n";

    return 0;
}
