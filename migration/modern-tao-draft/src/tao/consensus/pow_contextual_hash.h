// TaoX experimental contextual PoW hash bridge.
// DO NOT RELEASE — NO RANDOMX ACTIVATION.
//
// Caller must hold cs_main.
// This is NOT a consensus validity decision.
// No production validation path calls this interface.

#ifndef TAOX_CONSENSUS_POW_CONTEXTUAL_HASH_H
#define TAOX_CONSENSUS_POW_CONTEXTUAL_HASH_H

#include <tao/consensus/pow_algorithm.h>
#include <tao/consensus/pow_hash_dispatch.h>

#include <chain.h>
#include <primitives/block.h>

#include <limits>
#include <optional>

namespace taox::consensus {

inline std::optional<uint256> CalculateContextualPoWHash(
    const CBlockHeader& header,
    const CBlockIndex* parent)
    EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    // This helper handles non-genesis headers only.
    if (!parent)
        return std::nullopt;

    if (parent->nHeight < 0 ||
        parent->nHeight == std::numeric_limits<int>::max())
        return std::nullopt;

    if (!parent->phashBlock)
        return std::nullopt;

    if (header.hashPrevBlock != parent->GetBlockHash())
        return std::nullopt;

    if (!parent->IsValid(BLOCK_VALID_TREE))
        return std::nullopt;

    const auto algorithm =
        SelectPoWAlgorithm(parent->nHeight + 1);

    if (!algorithm)
        return std::nullopt;

    // No RandomX seed policy has been approved.
    // Until one exists, only historical Scrypt
    // may pass through this prototype.
    if (*algorithm != PoWAlgorithm::Scrypt)
        return std::nullopt;

    return CalculatePoWHash(
        header, algorithm, std::nullopt);
}

} // namespace taox::consensus

#endif // TAOX_CONSENSUS_POW_CONTEXTUAL_HASH_H
