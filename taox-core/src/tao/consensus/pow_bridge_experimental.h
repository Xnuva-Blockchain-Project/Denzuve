#ifndef TOAX_TEST_POW_BRIDGE_H
#define TOAX_TEST_POW_BRIDGE_H

#include <tao/consensus/pow_hash_dispatch.h>
#include <tao/consensus/pow_seed_resolver_experimental.h>

#include <chain.h>
#include <primitives/block.h>
#include <sync.h>

#include <limits>
#include <optional>

namespace toax::test_only {

// EXPERIMENTAL REFERENCE ONLY.
//
// This computes a PoW hash. It does not decide
// block acceptance or validate the difficulty.
//
// The policy is supplied explicitly by test code.
// It is not a mainnet activation parameter.
inline std::optional<uint256> ResolveAndHash(
    const CBlockHeader& candidate,
    const CBlockIndex* parent,
    const Policy& policy)
    EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    if (!parent || !parent->phashBlock)
        return std::nullopt;

    if (parent->nHeight < 0 ||
        parent->nHeight ==
            std::numeric_limits<int>::max())
        return std::nullopt;

    if (!parent->IsValid(BLOCK_VALID_TREE))
        return std::nullopt;

    if (candidate.hashPrevBlock !=
        parent->GetBlockHash())
        return std::nullopt;

    const int height = parent->nHeight + 1;
    const auto algorithm = Select(height, policy);

    if (!algorithm)
        return std::nullopt;

    if (*algorithm == Algorithm::Scrypt) {
        return taox::consensus::CalculatePoWHash(
            candidate,
            taox::consensus::PoWAlgorithm::Scrypt,
            std::nullopt);
    }

    if (*algorithm == Algorithm::RandomX) {
        const auto seed =
            ResolveSeed(candidate, parent, policy);

        if (!seed)
            return std::nullopt;

        return taox::consensus::CalculatePoWHash(
            candidate,
            taox::consensus::PoWAlgorithm::RandomX,
            seed);
    }

    return std::nullopt;
}

} // namespace toax::test_only

#endif
