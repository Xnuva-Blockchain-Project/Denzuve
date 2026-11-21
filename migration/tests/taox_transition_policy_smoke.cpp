// TaoX experimental RandomX transition policy.
// LABORATORY ONLY — DO NOT RELEASE.
//
// Predecessor state in this test is simulated.
// No real block-index validation is performed.
// Difficulty and chainwork consensus are not implemented.

#include <tao/consensus/pow_algorithm.h>
#include <arith_uint256.h>
#include <pow.h>
#include <uint256.h>

#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>

namespace lab {

using taox::consensus::PoWAlgorithm;
using taox::consensus::SelectPoWAlgorithm;

enum class ParentStatus {
    Verified,
    Unknown,
    Invalid
};

enum class Decision {
    UseScrypt,
    UseRandomX,
    Defer,
    Reject
};

struct ActivationPolicy {
    int activation_height;
    uint32_t initial_randomx_bits;
};

struct Result {
    Decision decision;
    std::optional<uint32_t> initial_bits;
};

Result Select(
    int parent_height,
    ParentStatus parent,
    std::optional<ActivationPolicy> policy)
{
    if (parent_height < 0 ||
        parent_height == std::numeric_limits<int>::max()) {
        return {Decision::Reject, std::nullopt};
    }

    if (parent == ParentStatus::Invalid) {
        return {Decision::Reject, std::nullopt};
    }

    if (parent == ParentStatus::Unknown) {
        return {Decision::Defer, std::nullopt};
    }

    if (parent != ParentStatus::Verified) {
        return {Decision::Reject, std::nullopt};
    }

    const int candidate_height = parent_height + 1;

    // No experimental activation configured.
    if (!policy) {
        auto algorithm = SelectPoWAlgorithm(candidate_height);

        if (algorithm &&
            *algorithm == PoWAlgorithm::Scrypt) {
            return {Decision::UseScrypt, std::nullopt};
        }

        return {Decision::Reject, std::nullopt};
    }

    // Fail closed on malformed laboratory policy.
    if (policy->activation_height <= 0) {
        return {Decision::Reject, std::nullopt};
    }

    const uint256 pow_limit{
        "00000ffffffffffffffffffffffffffffffff"
        "fffffffffffffffffffffffffff"
    };

    if (!DeriveTarget(
            policy->initial_randomx_bits, pow_limit)) {
        return {Decision::Reject, std::nullopt};
    }

    if (candidate_height < policy->activation_height) {
        return {Decision::UseScrypt, std::nullopt};
    }

    if (candidate_height == policy->activation_height) {
        return {
            Decision::UseRandomX,
            policy->initial_randomx_bits
        };
    }

    // Post-activation retargeting is not defined.
    return {Decision::Reject, std::nullopt};
}

} // namespace lab

int main()
{
    using namespace lab;

    constexpr ActivationPolicy test_policy{
        1000,
        0x1e0fffff
    };

    auto check = [](
        int parent_height,
        ParentStatus status,
        std::optional<ActivationPolicy> policy,
        Decision expected)
    {
        return Select(
            parent_height, status, policy
        ).decision == expected;
    };

    // Historical / default behaviour.
    if (!check(635, ParentStatus::Verified,
               std::nullopt, Decision::UseScrypt))
        return 1;

    if (!check(999, ParentStatus::Verified,
               std::nullopt, Decision::UseScrypt))
        return 2;

    std::cout << "PASS: default Scrypt preserved\n";

    // Laboratory activation boundary.
    if (!check(998, ParentStatus::Verified,
               test_policy, Decision::UseScrypt))
        return 3;

    const auto boundary =
        Select(999, ParentStatus::Verified, test_policy);

    if (boundary.decision != Decision::UseRandomX ||
        boundary.initial_bits != test_policy.initial_randomx_bits)
        return 4;

    std::cout << "PASS: explicit test boundary selected\n";

    // Future retargeting remains undefined.
    if (!check(1000, ParentStatus::Verified,
               test_policy, Decision::Reject))
        return 5;

    std::cout << "PASS: undefined retarget rejected\n";

    // Unknown and invalid ancestry.
    if (!check(999, ParentStatus::Unknown,
               test_policy, Decision::Defer))
        return 6;

    if (!check(999, ParentStatus::Invalid,
               test_policy, Decision::Reject))
        return 7;

    std::cout << "PASS: ancestry decisions enforced\n";

    // Malformed policy and heights.
    if (!check(999, ParentStatus::Verified,
               ActivationPolicy{0, 0x1e0fffff},
               Decision::Reject))
        return 8;

    if (!check(999, ParentStatus::Verified,
               ActivationPolicy{1000, 0},
               Decision::Reject))
        return 9;

    if (!check(-1, ParentStatus::Verified,
               test_policy, Decision::Reject))
        return 10;

    if (!check(std::numeric_limits<int>::max(),
               ParentStatus::Verified,
               test_policy, Decision::Reject))
        return 11;

    if (!check(999,
               static_cast<ParentStatus>(999),
               test_policy, Decision::Reject))
        return 12;

    for (uint32_t bad_bits : {
        uint32_t{0},
        uint32_t{0x1e8fffff},
        uint32_t{0x2300ffff},
        uint32_t{0x1f7fffff}
    }) {
        if (!check(
                999,
                ParentStatus::Verified,
                ActivationPolicy{1000, bad_bits},
                Decision::Reject))
            return 13;
    }

    std::cout << "PASS: invalid compact targets rejected\n";
    std::cout << "PASS: malformed inputs rejected\n";
    std::cout << "PASS: no mainnet activation\n";
    std::cout << "PASS: no chainwork policy invented\n";

    return 0;
}
