// TaoX experimental bounded RandomX seed tracking.
// Local development only. Not release-qualified.

#ifndef TAOX_CONSENSUS_POW_SEED_TRACKER_H
#define TAOX_CONSENSUS_POW_SEED_TRACKER_H

#include <tao/consensus/pow_algorithm.h>

#include <consensus/params.h>
#include <uint256.h>
#include <primitives/block.h>
#include <limits>
#include <span>

#include <cstdint>
#include <optional>

namespace taox::consensus {

class RandomXSeedTracker {
public:
    void Reset() noexcept
    {
        m_verified_height = -1;
        m_verified_hash.reset();
        m_seed_height = -1;
        m_seed_hash.reset();
    }

    // Establish an independently authenticated chain anchor.
    // The caller must verify the anchor before invoking this method.
    void SetVerifiedAnchor(
        std::int64_t height,
        const uint256& hash) noexcept
    {
        Reset();

        if (height < 0) return;

        m_verified_height = height;
        m_verified_hash = hash;
    }

    // The caller MUST obtain both hashes from an independently
    // validated chain and confirm that the seed is an ancestor
    // of the supplied anchor. This class checks heights only.
    // On failure, no previous context is retained.
    bool SetVerifiedAnchorAndAncestorSeed(
        std::int64_t anchor_height,
        const uint256& anchor_hash,
        std::int64_t seed_height,
        const uint256& seed_hash,
        const Consensus::Params& params) noexcept
    {
        Reset();

        if (anchor_height < 0 ||
            anchor_height >= static_cast<std::int64_t>(INT32_MAX))
            return false;

        const auto expected =
            RandomXSeedHeight(
                static_cast<int>(anchor_height + 1), params);

        if (!expected ||
            *expected != seed_height ||
            seed_height < 0 ||
            seed_height > anchor_height)
            return false;

        m_verified_height = anchor_height;
        m_verified_hash = anchor_hash;
        m_seed_height = seed_height;
        m_seed_hash = seed_hash;

        return true;
    }

    // Called only after the header at this height has
    // independently passed contextual proof-of-work validation.
    // Returns false if verified headers are not sequential.
    bool RecordVerifiedHeader(
        std::int64_t height,
        const uint256& hash,
        const Consensus::Params& params) noexcept
    {
        if (m_verified_height < 0 ||
            height != m_verified_height + 1)
            return false;

        const auto next_seed_height =
            RandomXSeedHeightForNextBlock(height, params);

        m_verified_height = height;
        m_verified_hash = hash;

        if (next_seed_height &&
            *next_seed_height == height) {
            m_seed_height = height;
            m_seed_hash = hash;
        }

        return true;
    }

    struct NextHeaderContext {
        PoWAlgorithm algorithm;
        std::optional<uint256> seed;
    };

    // Read-only prevalidation context. Never credits work or
    // advances the verified chain. The caller must independently
    // check difficulty, proof of work and other consensus rules.
    std::optional<NextHeaderContext> GetNextHeaderContext(
        const CBlockHeader& header,
        const Consensus::Params& params) const noexcept
    {
        if (m_verified_height < 0 ||
            m_verified_height >= std::numeric_limits<int>::max() ||
            !m_verified_hash ||
            header.hashPrevBlock != *m_verified_hash)
            return std::nullopt;

        const int next_height =
            static_cast<int>(m_verified_height + 1);

        const auto algorithm =
            SelectPoWAlgorithm(next_height, params);

        if (!algorithm)
            return std::nullopt;

        if (*algorithm == PoWAlgorithm::Scrypt)
            return NextHeaderContext{*algorithm, std::nullopt};

        const auto required_seed =
            RandomXSeedHeight(next_height, params);

        if (!required_seed)
            return std::nullopt;

        const auto seed = GetSeed(*required_seed);

        if (!seed)
            return std::nullopt;

        return NextHeaderContext{*algorithm, seed};
    }

    // Validate against a temporary copy of session state.
    // The verifier must perform actual consensus PoW checks.
    // This class only guarantees sequential context and
    // all-or-nothing state advancement.
    template <typename Verifier>
    bool ValidateAndRecordBatch(
        std::span<const CBlockHeader> headers,
        const Consensus::Params& params,
        Verifier&& verify)
    {
        RandomXSeedTracker candidate = *this;

        for (const auto& header : headers) {
            const auto context =
                candidate.GetNextHeaderContext(header, params);

            if (!context)
                return false;

            if (!verify(header, *context))
                return false;

            const auto next_height =
                candidate.VerifiedHeight() + 1;

            if (!candidate.RecordVerifiedHeader(
                    next_height, header.GetHash(), params))
                return false;
        }

        *this = candidate;
        return true;
    }

    std::optional<uint256> GetSeed(
        std::int64_t required_height) const noexcept
    {
        if (required_height < 0 ||
            required_height > m_verified_height)
            return std::nullopt;

        if (m_seed_hash &&
            m_seed_height == required_height)
            return m_seed_hash;

        if (m_verified_hash &&
            m_verified_height == required_height)
            return m_verified_hash;

        return std::nullopt;
    }

    std::int64_t VerifiedHeight() const noexcept
    {
        return m_verified_height;
    }

private:
    static std::optional<std::int64_t>
    RandomXSeedHeightForNextBlock(
        std::int64_t height,
        const Consensus::Params& params) noexcept
    {
        if (height < 0 ||
            height >= static_cast<std::int64_t>(INT32_MAX))
            return std::nullopt;

        return RandomXSeedHeight(
            static_cast<int>(height + 1), params);
    }

    std::int64_t m_verified_height{-1};
    std::optional<uint256> m_verified_hash;

    std::int64_t m_seed_height{-1};
    std::optional<uint256> m_seed_hash;
};

} // namespace taox::consensus

#endif
