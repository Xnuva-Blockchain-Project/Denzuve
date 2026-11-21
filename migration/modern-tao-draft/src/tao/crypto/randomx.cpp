// TAO Revival experimental RandomX v2 adapter.
// Development-only. Never route production block validation through this file
// before the hard-fork specification, tests and network upgrade are complete.
#include <tao/crypto/randomx.h>

#include <primitives/block.h>
#include <streams.h>

#include <randomx.h>

#include <memory>
#include <mutex>
#include <stdexcept>

namespace tao::experimental {

struct RandomXLightHasher::Impl {
    randomx_cache* cache{nullptr};
    randomx_vm* vm{nullptr};
    mutable std::mutex mutex;

    explicit Impl(std::span<const std::byte> seed_key)
    {
        if (seed_key.empty()) {
            throw std::invalid_argument("TAO RandomX seed key must not be empty");
        }

        // Portable light-mode verification (256 MiB). No JIT or large pages.
        cache = randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
        if (cache == nullptr) {
            throw std::runtime_error("TAO RandomX light cache allocation failed");
        }
        randomx_init_cache(cache, seed_key.data(), seed_key.size());

        // XNUVA-specific RandomX v2, not a silent substitution with v1.
        vm = randomx_create_vm(RANDOMX_FLAG_V2, cache, nullptr);
        if (vm == nullptr) {
            randomx_release_cache(cache);
            cache = nullptr;
            throw std::runtime_error("TAO RandomX v2 VM creation failed");
        }
    }

    ~Impl()
    {
        if (vm != nullptr) randomx_destroy_vm(vm);
        if (cache != nullptr) randomx_release_cache(cache);
    }
};

RandomXLightHasher::RandomXLightHasher(
    std::span<const std::byte> seed_key)
    : m_impl(std::make_unique<Impl>(seed_key))
{
}

RandomXLightHasher::~RandomXLightHasher() = default;

uint256 RandomXLightHasher::Hash(std::span<const std::byte> input) const
{
    if (input.empty()) {
        throw std::invalid_argument("TAO RandomX input must not be empty");
    }
    // RandomX VM state is mutable: a single VM must not be used concurrently.
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    uint256 digest;
    randomx_calculate_hash(
        m_impl->vm, input.data(), input.size(), digest.data());
    return digest;
}

uint256 RandomXLightHasher::HashHeader(const CBlockHeader& header) const
{
    DataStream stream{};
    stream << header;
    if (stream.size() != 80) {
        throw std::runtime_error(
            "TAO RandomX block header serialization must be 80 bytes");
    }
    return Hash(std::span<const std::byte>{stream.data(), stream.size()});
}

uint256 RandomXHashV2(std::span<const std::byte> input,
                     std::span<const std::byte> seed_key)
{
    return RandomXLightHasher{seed_key}.Hash(input);
}

uint256 RandomXHeaderHashV2(const CBlockHeader& header,
                           const uint256& seed_block_id)
{
    const std::span<const unsigned char> seed_bytes{
        seed_block_id.data(), seed_block_id.size()
    };
    return RandomXLightHasher{std::as_bytes(seed_bytes)}.HashHeader(header);
}

} // namespace tao::experimental
