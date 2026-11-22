// TAO Revival experimental RandomX v2 hashing interface.
// DO NOT RELEASE: no hard-fork height, PoW switch or live validation is defined.
#ifndef TAO_CRYPTO_RANDOMX_EXPERIMENTAL_H
#define TAO_CRYPTO_RANDOMX_EXPERIMENTAL_H

#include <uint256.h>

#include <cstddef>
#include <memory>
#include <span>

class CBlockHeader;

namespace tao::experimental {

// RandomX v2 uses a consensus-defining variant. This code deliberately uses
// XNUVA's pinned RandomX source, not an unversioned system librandomx.
// The VM is light-mode; miners will need an independently tested fast path.
class RandomXLightHasher {
public:
    explicit RandomXLightHasher(std::span<const std::byte> seed_key);
    ~RandomXLightHasher();
    RandomXLightHasher(const RandomXLightHasher&) = delete;
    RandomXLightHasher& operator=(const RandomXLightHasher&) = delete;

    uint256 Hash(std::span<const std::byte> input) const;
    uint256 HashHeader(const CBlockHeader& header) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// Reference helper for deterministic unit vectors; inefficient for mining.
// The seed key is explicit: TAO's on-chain seed schedule is NOT yet selected.
uint256 RandomXHashV2(std::span<const std::byte> input,
                     std::span<const std::byte> seed_key);

uint256 RandomXHeaderHashV2(const CBlockHeader& header,
                           const uint256& seed_block_id);

} // namespace tao::experimental

#endif // TAO_CRYPTO_RANDOMX_EXPERIMENTAL_H
