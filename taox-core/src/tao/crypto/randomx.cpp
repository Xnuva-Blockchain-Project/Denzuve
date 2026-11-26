// TaoX experimental adapter around the pinned shared RandomX V2 foundation.
// No consensus activation, miner or production network code calls this module.
// The shared engine returns RAW bytes: preserve them without endianness reversal.
#include <tao/crypto/randomx.h>

#include <primitives/block.h>
#include <streams.h>
#include <xnuva_randomx/engine.h>

#include <cstring>
#include <memory>
#include <optional>
#include <stdexcept>

namespace tao::experimental {

struct RandomXLightHasher::Impl {
    xnuva::randomx_foundation::LightHasher engine;
    explicit Impl(std::span<const std::byte> seed_key) : engine(seed_key) {}
};

RandomXLightHasher::RandomXLightHasher(std::span<const std::byte> seed_key)
    : m_impl(std::make_unique<Impl>(seed_key))
{
}

RandomXLightHasher::~RandomXLightHasher() = default;

namespace {
uint256 FromRawDigest(const xnuva::randomx_foundation::Digest& raw)
{
    static_assert(sizeof(raw) == 32);
    uint256 digest;
    // Preserve previous adapter byte representation: uint256::GetHex()
    // applies its existing display convention; never reverse bytes here.
    std::memcpy(digest.data(), raw.data(), raw.size());
    return digest;
}
} // namespace

uint256 RandomXLightHasher::Hash(std::span<const std::byte> input) const
{
    return FromRawDigest(m_impl->engine.Hash(input));
}

uint256 RandomXLightHasher::HashHeader(const CBlockHeader& header) const
{
    DataStream stream{};
    stream << header;
    if (stream.size() != 80) {
        throw std::runtime_error(
            "TaoX RandomX block header serialization must be 80 bytes");
    }
    return FromRawDigest(m_impl->engine.Hash80ByteHeader(
        std::span<const std::byte>{stream.data(), stream.size()}));
}

uint256 RandomXHashV2(std::span<const std::byte> input,
                     std::span<const std::byte> seed_key)
{
    return RandomXLightHasher{seed_key}.Hash(input);
}

uint256 RandomXHeaderHashV2(const CBlockHeader& header,
                           const uint256& seed_block_id)
{
    // Each calling thread owns its own VM and cache.
    // Never share a mutable RandomX VM between threads.
    struct SeedHasher {
        std::optional<uint256> seed;
        std::unique_ptr<RandomXLightHasher> hasher;
    };

    thread_local SeedHasher cached;

    // Compare the complete seed identity, not only its
    // height or a shortened representation.
    if (!cached.hasher || !cached.seed ||
        *cached.seed != seed_block_id) {

        // Release the previous allocation before constructing
        // another cache. Retain at most one hasher per thread.
        cached.hasher.reset();
        cached.seed.reset();

        const std::span<const unsigned char> seed_bytes{
            seed_block_id.data(), seed_block_id.size()
        };

        auto replacement = std::make_unique<RandomXLightHasher>(
            std::as_bytes(seed_bytes));

        cached.hasher = std::move(replacement);
        cached.seed = seed_block_id;
    }

    // Only the engine allocation is reused.
    // Every header is independently serialised and hashed.
    return cached.hasher->HashHeader(header);
}


struct RandomXFullHasher::Impl {
    xnuva::randomx_foundation::FullHasher engine;

    Impl(std::span<const std::byte> seed_key,
         std::size_t worker_count)
        : engine(seed_key, worker_count)
    {
    }
};

RandomXFullHasher::RandomXFullHasher(
    std::span<const std::byte> seed_key,
    std::size_t worker_count)
    : m_impl(std::make_unique<Impl>(seed_key, worker_count))
{
}

RandomXFullHasher::~RandomXFullHasher() = default;

uint256 RandomXFullHasher::HashHeader(
    const CBlockHeader& header) const
{
    return HashHeaderForWorker(0, header);
}

uint256 RandomXFullHasher::HashHeaderForWorker(
    std::size_t worker_index,
    const CBlockHeader& header) const
{
    DataStream stream{};
    stream << header;

    if (stream.size() != 80) {
        throw std::runtime_error(
            "TaoX RandomX header must be exactly 80 bytes");
    }

    return FromRawDigest(
        m_impl->engine.Hash80ByteHeaderForWorker(
            worker_index,
            std::span<const std::byte>{
                stream.data(), stream.size()
            }));
}

} // namespace tao::experimental
