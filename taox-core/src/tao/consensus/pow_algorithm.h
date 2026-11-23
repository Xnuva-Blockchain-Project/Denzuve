// TaoX consensus Proof-of-Work algorithm selection.
#ifndef TAOX_CONSENSUS_POW_ALGORITHM_H
#define TAOX_CONSENSUS_POW_ALGORITHM_H

#include <consensus/params.h>

#include <cstdint>
#include <optional>

namespace taox::consensus {

enum class PoWAlgorithm {
    Scrypt,
    RandomX
};

inline std::optional<PoWAlgorithm> SelectPoWAlgorithm(
    int height, const Consensus::Params& params) noexcept
{
    if (height < 0) return std::nullopt;

    const int activation = params.randomx_activation_height;

    if (activation == -1) return PoWAlgorithm::Scrypt;

    if (activation <= 0 ||
        params.randomx_seed_epoch_length <= 0)
        return std::nullopt;

    return height < activation
        ? PoWAlgorithm::Scrypt
        : PoWAlgorithm::RandomX;
}

inline std::optional<std::int64_t> RandomXSeedHeight(
    int height, const Consensus::Params& params) noexcept
{
    if (SelectPoWAlgorithm(height, params) !=
        PoWAlgorithm::RandomX)
        return std::nullopt;

    const std::int64_t activation =
        params.randomx_activation_height;
    const std::int64_t epoch_length =
        params.randomx_seed_epoch_length;
    const std::int64_t relative =
        static_cast<std::int64_t>(height) - activation;

    const std::int64_t seed =
        activation - 1 +
        (relative / epoch_length) * epoch_length;

    if (seed < 0 || seed >= height)
        return std::nullopt;

    return seed;
}

} // namespace taox::consensus

#endif
