#ifndef TOAX_EXPERIMENTAL_SEED_RESOLVER_H
#define TOAX_EXPERIMENTAL_SEED_RESOLVER_H

#include <tao/consensus/pow_transition_policy_experimental.h>

#include <chain.h>
#include <primitives/block.h>
#include <sync.h>
#include <uint256.h>

#include <limits>
#include <optional>

namespace toax::test_only {

// Returns a seed from the candidate's own parent
// ancestry, never from the active-chain tip.
//
// This is an experimental reference resolver.
// BLOCK_VALID_TREE is not full chainstate validity.
inline std::optional<uint256> ResolveSeed(
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

    const int candidate_height =
        parent->nHeight + 1;

    if (Select(candidate_height, policy) !=
        Algorithm::RandomX)
        return std::nullopt;

    const auto height =
        SeedHeight(candidate_height, policy);

    if (!height ||
        *height < 0 ||
        *height > parent->nHeight)
        return std::nullopt;

    const auto* ancestor =
        parent->GetAncestor(static_cast<int>(*height));

    if (!ancestor || !ancestor->phashBlock)
        return std::nullopt;

    if (!ancestor->IsValid(BLOCK_VALID_TREE))
        return std::nullopt;

    return ancestor->GetBlockHash();
}

} // namespace toax::test_only

#endif
