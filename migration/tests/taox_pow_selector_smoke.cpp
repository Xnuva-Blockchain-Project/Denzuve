#include <tao/consensus/pow_algorithm.h>

#include <array>
#include <cassert>
#include <iostream>
#include <limits>

int main()
{
    using taox::consensus::PoWAlgorithm;
    using taox::consensus::SelectPoWAlgorithm;

    constexpr std::array<int, 8> heights{
        0, 1, 2, 636, 637, 710000,
        1000000, std::numeric_limits<int>::max()
    };

    for (int height : heights) {
        const auto result = SelectPoWAlgorithm(height);

        assert(result.has_value());
        assert(*result == PoWAlgorithm::Scrypt);

        if (!result.has_value() ||
            *result != PoWAlgorithm::Scrypt) {
            return 1;
        }
    }

    constexpr auto negative = SelectPoWAlgorithm(-1);

    static_assert(!negative.has_value());

    if (SelectPoWAlgorithm(-1).has_value()) {
        return 1;
    }

    std::cout
        << "PASS: all tested valid heights use Scrypt\n"
        << "PASS: negative height rejected\n"
        << "PASS: RandomX activation remains disabled\n";

    return 0;
}
