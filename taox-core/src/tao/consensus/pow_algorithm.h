// TaoX experimental Proof-of-Work algorithm selector.
// DO NOT RELEASE — NO RANDOMX CONSENSUS ACTIVATION.

#ifndef TAOX_CONSENSUS_POW_ALGORITHM_H
#define TAOX_CONSENSUS_POW_ALGORITHM_H

#include <optional>

namespace taox::consensus {

enum class PoWAlgorithm {
    Scrypt,
    RandomX
};

// Initial consensus-safe policy:
// - Negative heights are invalid.
// - Every nonnegative height uses historical Scrypt.
// - RandomX must not activate in this revision.
constexpr std::optional<PoWAlgorithm>
SelectPoWAlgorithm(int height) noexcept
{
    if (height < 0) {
        return std::nullopt;
    }

    return PoWAlgorithm::Scrypt;
}

} // namespace taox::consensus

#endif
