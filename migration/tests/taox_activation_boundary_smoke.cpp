// TaoX laboratory activation-boundary test.
// NOT A CONSENSUS IMPLEMENTATION.
// Fictional height 1000 must never activate on mainnet.

#include <tao/consensus/pow_algorithm.h>

#include <cassert>
#include <iostream>
#include <limits>
#include <optional>

namespace lab {

using taox::consensus::PoWAlgorithm;

struct TestPolicy {
    bool randomx_enabled;
    int activation_height;
};

std::optional<PoWAlgorithm> Select(
    int height, const TestPolicy& policy)
{
    if (height < 0) return std::nullopt;

    if (!policy.randomx_enabled)
        return PoWAlgorithm::Scrypt;

    if (policy.activation_height <= 0)
        return std::nullopt;

    if (height < policy.activation_height)
        return PoWAlgorithm::Scrypt;

    return PoWAlgorithm::RandomX;
}

} // namespace lab

int main()
{
    using taox::consensus::PoWAlgorithm;
    using taox::consensus::SelectPoWAlgorithm;

    const lab::TestPolicy disabled{false, 1000};
    const lab::TestPolicy enabled{true, 1000};
    const lab::TestPolicy invalid{true, 0};

    // Production-facing selector remains Scrypt-only.
    for (int h : {0, 1, 999, 1000, 1001, 710000}) {
        const auto actual = SelectPoWAlgorithm(h);
        if (!actual || *actual != PoWAlgorithm::Scrypt)
            return 1;

        const auto inactive = lab::Select(h, disabled);
        if (!inactive || *inactive != PoWAlgorithm::Scrypt)
            return 2;
    }

    std::cout << "PASS: default policy remains Scrypt\n";

    // Test-only activation boundary.
    if (lab::Select(999, enabled) != PoWAlgorithm::Scrypt)
        return 3;

    if (lab::Select(1000, enabled) != PoWAlgorithm::RandomX)
        return 4;

    if (lab::Select(1001, enabled) != PoWAlgorithm::RandomX)
        return 5;

    std::cout << "PASS: fictional activation boundary\n";

    // Fail closed on invalid inputs.
    if (lab::Select(-1, enabled).has_value())
        return 6;

    if (lab::Select(1, invalid).has_value())
        return 7;

    if (SelectPoWAlgorithm(-1).has_value())
        return 8;

    std::cout << "PASS: invalid configurations rejected\n";
    std::cout << "PASS: no production activation\n";

    return 0;
}
