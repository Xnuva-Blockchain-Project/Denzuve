// TaoX experimental contextual PoW batch verification.
// Development only — not release-qualified.

#ifndef TAOX_CONSENSUS_POW_BATCH_VALIDATION_H
#define TAOX_CONSENSUS_POW_BATCH_VALIDATION_H

#include <tao/consensus/pow_seed_tracker.h>
#include <tao/consensus/pow_hash_dispatch.h>

#include <pow.h>

#include <exception>
#include <span>

namespace taox::consensus {

// All-or-nothing contextual proof-of-work verification.
//
// The caller must initialise the tracker using independently
// authenticated chain ancestry.
//
// Actual network integration must additionally enforce the
// header difficulty-transition and other consensus rules.
//
// On failure, the tracker retains its original state.
inline bool ValidateContextualPoWBatch(
    RandomXSeedTracker& tracker,
    std::span<const CBlockHeader> headers,
    const Consensus::Params& params)
{
    try {
        return tracker.ValidateAndRecordBatch(
            headers,
            params,
            [&params](
                const CBlockHeader& header,
                const RandomXSeedTracker::NextHeaderContext& context)
            {
                const auto hash = CalculatePoWHash(
                    header,
                    context.algorithm,
                    context.seed);

                return hash.has_value() &&
                    CheckProofOfWork(
                        *hash,
                        header.nBits,
                        params);
            });
    } catch (const std::exception&) {
        // Never accept an unverified batch if the
        // RandomX hashing implementation fails.
        return false;
    }
}

} // namespace taox::consensus

#endif
