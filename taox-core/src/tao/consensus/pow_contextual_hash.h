// TaoX production contextual Proof-of-Work hashing.
#ifndef TAOX_CONSENSUS_POW_CONTEXTUAL_HASH_H
#define TAOX_CONSENSUS_POW_CONTEXTUAL_HASH_H

#include <tao/consensus/pow_algorithm.h>
#include <tao/consensus/pow_hash_dispatch.h>

#include <chain.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <sync.h>

#include <limits>
#include <optional>

namespace taox::consensus {

inline std::optional<uint256> CalculateContextualPoWHash(
    const CBlockHeader& header,
    const CBlockIndex* parent,
    const Consensus::Params& params)
    EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    if (!parent || !parent->phashBlock)
        return std::nullopt;

    if (parent->nHeight < 0 ||
        parent->nHeight == std::numeric_limits<int>::max())
        return std::nullopt;

    if (header.hashPrevBlock != parent->GetBlockHash())
        return std::nullopt;

    if (!parent->IsValid(BLOCK_VALID_TREE))
        return std::nullopt;

    const int height = parent->nHeight + 1;
    const auto algorithm = SelectPoWAlgorithm(height, params);

    if (!algorithm)
        return std::nullopt;

    if (*algorithm == PoWAlgorithm::Scrypt)
        return CalculatePoWHash(
            header, algorithm, std::nullopt);

    const auto seed_height = RandomXSeedHeight(height, params);
    if (!seed_height || *seed_height > parent->nHeight)
        return std::nullopt;

    const CBlockIndex* ancestor =
        parent->GetAncestor(static_cast<int>(*seed_height));

    if (!ancestor || !ancestor->phashBlock ||
        !ancestor->IsValid(BLOCK_VALID_TREE))
        return std::nullopt;

    return CalculatePoWHash(
        header, algorithm, ancestor->GetBlockHash());
}

} // namespace taox::consensus

#endif
