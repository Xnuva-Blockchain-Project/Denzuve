#include <tao/consensus/pow_transition_policy_experimental.h>

#include <cassert>
#include <iostream>
#include <limits>

using namespace toax::test_only;

constexpr Policy disabled{std::nullopt, 16};
constexpr Policy trial{1000, 16};

static_assert(Select(0, disabled) ==
              Algorithm::Scrypt);
static_assert(Select(999, trial) ==
              Algorithm::Scrypt);
static_assert(Select(1000, trial) ==
              Algorithm::RandomX);
static_assert(SeedHeight(1000, trial) == 999);
static_assert(SeedHeight(1015, trial) == 999);
static_assert(SeedHeight(1016, trial) == 1015);

int main()
{
    assert(!Select(-1, disabled));
    assert(!SeedHeight(1000, disabled));

    std::cout << "PASS: disabled policy\n";

    assert(Select(999, trial) ==
           Algorithm::Scrypt);
    assert(Select(1000, trial) ==
           Algorithm::RandomX);

    std::cout << "PASS: activation boundary\n";

    assert(!SeedHeight(999, trial));
    assert(SeedHeight(1000, trial) == 999);
    assert(SeedHeight(1015, trial) == 999);
    assert(SeedHeight(1016, trial) == 1015);
    assert(SeedHeight(1031, trial) == 1015);
    assert(SeedHeight(1032, trial) == 1031);

    std::cout << "PASS: seed epoch boundaries\n";

    assert(!Select(1000, Policy{0, 16}));
    assert(!Select(1000, Policy{1000, 0}));
    assert(!Select(1000, Policy{1000, -1}));
    assert(!SeedHeight(1000, Policy{0, 16}));
    assert(!SeedHeight(1000, Policy{1000, 0}));

    std::cout << "PASS: invalid policies rejected\n";

    constexpr int maximum =
        std::numeric_limits<int>::max();

    constexpr Policy extreme{maximum, 16};

    assert(Select(maximum, extreme) ==
           Algorithm::RandomX);
    assert(SeedHeight(maximum, extreme) ==
           static_cast<std::int64_t>(maximum) - 1);

    assert(SeedHeight(maximum, trial).has_value());
    assert(*SeedHeight(maximum, trial) < maximum);

    std::cout << "PASS: integer boundaries\n";

    std::cout
        << "PASS: C++ policy prototype\n"
        << "NOTE: illustrative activation=1000\n"
        << "NOTE: not connected to node consensus\n"
        << "NOTE: difficulty transition pending\n"
        << "NOTE: mainnet RandomX disabled\n";
}
