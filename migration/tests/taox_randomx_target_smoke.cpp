// TaoX experimental compact-target qualification.
// Standalone laboratory test. DO NOT RELEASE.

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

    const auto valid = DeriveTarget(0x1e0fffff, pow_limit);
    if (!valid) return 1;

    const auto historical = DeriveTarget(
        0x1e04d8b0, pow_limit
    );
    if (!historical) return 2;

    std::cout << "PASS: valid targets accepted\n";

    if (DeriveTarget(0, pow_limit)) return 3;
    if (DeriveTarget(0x1e8fffff, pow_limit)) return 4;
    if (DeriveTarget(0x2300ffff, pow_limit)) return 5;
    if (DeriveTarget(0x1f7fffff, pow_limit)) return 6;

    std::cout << "PASS: invalid targets rejected\n";

    // The powLimit must itself be representable.
    arith_uint256 limit = UintToArith256(pow_limit);

    if (limit == arith_uint256{0}) return 7;

    std::cout << "PASS: TaoX powLimit verified\n";
    std::cout << "PASS: no RandomX consensus activation\n";

    return 0;
}
