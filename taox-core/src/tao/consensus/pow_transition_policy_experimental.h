#ifndef TOAX_TEST_TRANSITION_POLICY_H
#define TOAX_TEST_TRANSITION_POLICY_H

#include <cstdint>
#include <optional>

namespace toax::test_only {

enum class Algorithm {
    Scrypt,
    RandomX
};

struct Policy {
    std::optional<int> activation_height;
    int seed_epoch_length;
};

constexpr bool ValidPolicy(const Policy& policy)
{
    return policy.seed_epoch_length > 0 &&
        (!policy.activation_height ||
         *policy.activation_height > 0);
}

constexpr std::optional<Algorithm> Select(
    int height,
    const Policy& policy)
{
    if (height < 0 || !ValidPolicy(policy))
        return std::nullopt;

    if (!policy.activation_height ||
        height < *policy.activation_height)
        return Algorithm::Scrypt;

    return Algorithm::RandomX;
}

// Height only. The actual seed block ID must come
// from verified ancestry of the candidate's parent.
constexpr std::optional<std::int64_t> SeedHeight(
    int height,
    const Policy& policy)
{
    if (!ValidPolicy(policy) ||
        !policy.activation_height ||
        height < *policy.activation_height)
        return std::nullopt;

    const std::int64_t activation =
        *policy.activation_height;

    const std::int64_t relative =
        static_cast<std::int64_t>(height) - activation;

    const std::int64_t epoch =
        relative / policy.seed_epoch_length;

    const std::int64_t seed =
        activation - 1 +
        epoch * policy.seed_epoch_length;

    if (seed < 0 ||
        seed >= static_cast<std::int64_t>(height))
        return std::nullopt;

    return seed;
}

} // namespace toax::test_only

#endif
