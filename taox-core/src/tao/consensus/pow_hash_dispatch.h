// TaoX experimental PoW hash dispatch.
// DO NOT RELEASE — NO RANDOMX ACTIVATION.
//
// Algorithm and seed must come from independently
// verified consensus context, not untrusted input.
//
// This interface only calculates a hash.
// It does not verify difficulty or accept blocks.

#ifndef TAOX_CONSENSUS_POW_HASH_DISPATCH_H
#define TAOX_CONSENSUS_POW_HASH_DISPATCH_H

#include <tao/consensus/pow_algorithm.h>
#include <tao/crypto/randomx.h>

#include <primitives/block.h>
#include <uint256.h>

#include <optional>

namespace taox::consensus {

inline std::optional<uint256> CalculatePoWHash(
    const CBlockHeader& header,
    std::optional<PoWAlgorithm> algorithm,
    const std::optional<uint256>& seed_block_id)
{
    if (!algorithm.has_value()) {
        return std::nullopt;
    }

    switch (*algorithm) {
    case PoWAlgorithm::Scrypt:
        // Preserve the existing historical function.
        return header.GetPoWHash();

    case PoWAlgorithm::RandomX:
        // Fail closed if consensus seed is absent.
        if (!seed_block_id.has_value()) {
            return std::nullopt;
        }

        // Reuse pinned modular RandomX V2 adapter.
        // Exceptions propagate; never fall back.
        return tao::experimental::RandomXHeaderHashV2(
            header, *seed_block_id);
    }

    return std::nullopt;
}

} // namespace taox::consensus

#endif // TAOX_CONSENSUS_POW_HASH_DISPATCH_H
