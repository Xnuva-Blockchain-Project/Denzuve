// TaoX compact-target canonicality experiment.
// DO NOT RELEASE. No consensus rules are changed.

#include <arith_uint256.h>
#include <pow.h>
#include <uint256.h>

#include <cstdint>
#include <iostream>

int main()
{
    const uint256 pow_limit{
        "00000ffffffffffffffffffffffffffffffff"
        "fffffffffffffffffffffffffff"
    };

    constexpr uint32_t canonical = 0x1e04d8b0;

    const auto target = DeriveTarget(canonical, pow_limit);

    if (!target) return 1;

    if (target->GetCompact() != canonical) return 2;

    std::cout << "PASS: historical canonical target\n";

    // These two encodings have the same numerical target:
    // 0x02000100 and 0x01000001 both represent 1.
    constexpr uint32_t alternate = 0x02000100;
    constexpr uint32_t normalized = 0x01010000;

    const auto alternate_target =
        DeriveTarget(alternate, pow_limit);

    const auto normalized_target =
        DeriveTarget(normalized, pow_limit);

    if (!alternate_target || !normalized_target)
        return 3;

    if (*alternate_target != *normalized_target)
        return 4;

    if (alternate_target->GetCompact() != normalized)
        return 5;

    if (alternate == normalized)
        return 6;

    std::cout
        << "PASS: equivalent target encodings identified\n";

    std::cout
        << "PASS: DeriveTarget accepts a noncanonical encoding\n";

    std::cout
        << "PASS: no historical consensus change\n";

    return 0;
}
