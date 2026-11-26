// XNUVA reference vectors reused independently in the TAO adapter.
// This is an isolated cryptographic self-test, NOT a TAO consensus test.
#include <tao/crypto/randomx.h>

#include <primitives/block.h>
#include <uint256.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <thread>
#include <vector>
#include <iostream>
#include <span>
#include <string>

namespace {

uint256 UncachedHeaderHash(
    const CBlockHeader& header,
    const uint256& seed)
{
    const std::span<const unsigned char> bytes{
        seed.data(), seed.size()
    };

    tao::experimental::RandomXLightHasher reference{
        std::as_bytes(bytes)
    };

    return reference.HashHeader(header);
}

void RequireEqual(
    const uint256& actual,
    const uint256& expected,
    const char* message)
{
    if (actual != expected) {
        throw std::runtime_error(message);
    }
}

void QualifyCachedHeaderHash(
    const CBlockHeader& original,
    const uint256& seed_a)
{
    using tao::experimental::RandomXHeaderHashV2;
    using Clock = std::chrono::steady_clock;

    // Change only one byte of the original seed.
    // A shortened or incomplete cache key must not pass.
    uint256 seed_b = seed_a;
    seed_b.begin()[0] ^= 0x01;

    const std::array<uint256, 2> seeds{seed_a, seed_b};

    constexpr std::array<uint32_t, 4> nonces{
        0U, 1U, 5U, 17U
    };

    std::array<std::array<uint256, 4>, 2> expected{};

    // Calculate independent references using newly allocated
    // light hashers, not the cached function under test.
    const auto uncached_start = Clock::now();

    for (std::size_t i = 0; i < seeds.size(); ++i) {
        for (std::size_t j = 0; j < nonces.size(); ++j) {
            CBlockHeader candidate = original;
            candidate.nNonce = nonces[j];

            expected[i][j] =
                UncachedHeaderHash(candidate, seeds[i]);
        }
    }

    const auto uncached_end = Clock::now();

    // Distinct seed references must produce distinct digests
    // for this deterministic fixture.
    for (std::size_t j = 0; j < nonces.size(); ++j) {
        if (expected[0][j] == expected[1][j]) {
            throw std::runtime_error(
                "Distinct seed reference digests unexpectedly match");
        }
    }

    const auto cached_start = Clock::now();

    for (std::size_t i = 0; i < seeds.size(); ++i) {
        for (std::size_t j = 0; j < nonces.size(); ++j) {
            CBlockHeader candidate = original;
            candidate.nNonce = nonces[j];

            RequireEqual(
                RandomXHeaderHashV2(candidate, seeds[i]),
                expected[i][j],
                "Cached versus uncached digest mismatch");
        }
    }

    const auto cached_end = Clock::now();

    // Explicitly exercise returning to a previous seed.
    for (const std::size_t i : {0U, 1U, 0U}) {
        for (std::size_t j = 0; j < nonces.size(); ++j) {
            CBlockHeader candidate = original;
            candidate.nNonce = nonces[j];

            RequireEqual(
                RandomXHeaderHashV2(candidate, seeds[i]),
                expected[i][j],
                "Seed transition digest mismatch");

            RequireEqual(
                RandomXHeaderHashV2(candidate, seeds[i]),
                expected[i][j],
                "Repeated header digest mismatch");
        }
    }

    // Independently owned thread-local hashers must agree
    // with the same reference digests, including seed changes.
    std::atomic<bool> concurrent_ok{true};

    {
        std::vector<std::jthread> workers;

        for (std::size_t worker = 0; worker < 4; ++worker) {
            workers.emplace_back([&, worker] {
                try {
                    for (std::size_t round = 0;
                         round < 3; ++round) {

                        const std::size_t i =
                            (worker + round) % seeds.size();

                        for (std::size_t j = 0;
                             j < nonces.size(); ++j) {

                            CBlockHeader candidate = original;
                            candidate.nNonce = nonces[j];

                            RequireEqual(
                                RandomXHeaderHashV2(
                                    candidate, seeds[i]),
                                expected[i][j],
                                "Concurrent cached digest mismatch");
                        }
                    }
                } catch (...) {
                    concurrent_ok.store(false);
                }
            });
        }

        // jthread destructors join all workers before
        // references and shared test state are destroyed.
    }

    if (!concurrent_ok.load()) {
        throw std::runtime_error(
            "Concurrent RandomX equivalence failed");
    }

    const double uncached_ms =
        std::chrono::duration<double, std::milli>(
            uncached_end - uncached_start).count();

    const double cached_ms =
        std::chrono::duration<double, std::milli>(
            cached_end - cached_start).count();

    std::cout
        << "PASS: eight independent reference digests\n"
        << "PASS: complete seed identity comparison\n"
        << "PASS: repeated header equivalence\n"
        << "PASS: A-B-A seed transition equivalence\n"
        << "PASS: four-thread cache equivalence\n"
        << "UNCACHED_MS=" << uncached_ms << '\n'
        << "CACHED_MS=" << cached_ms << '\n';

    if (cached_ms > 0) {
        std::cout << "OBSERVED_SPEEDUP="
                  << uncached_ms / cached_ms << '\n';
    }
}

} // namespace

int main()
{
    try {
        const std::string key{"test key 000"};
        const std::string input{"This is a test"};
        const std::span<const char> key_span{key.data(), key.size()};
        const std::span<const char> input_span{input.data(), input.size()};

        const auto digest{tao::experimental::RandomXHashV2(
            std::as_bytes(input_span), std::as_bytes(key_span))};
        if (digest.GetHex() !=
            "6fdb4cfbfbb4c5c966df8309e8fcec67"
            "c91395d6baefb28636b23e1b866bec22") {
            std::cerr << "FAIL: RandomX v2 reference vector\n";
            return 1;
        }

        // A pre-existing XNUVA header fixture tests canonical 80-byte input
        // and byte-order, but cannot validate TAO hard-fork consensus.
        CBlockHeader header{};
        header.nVersion = 1;
        header.hashPrevBlock.SetNull();
        header.hashMerkleRoot = uint256{
            "fb923316be9e4f5db3467d8c2d159bb"
            "3a27fa9ba2c71703c078a0ea7ccb676e8"};
        header.nTime = 1788642000;
        header.nBits = 0x207fffff;
        header.nNonce = 0;

        const uint256 seed{
            "2d95b3b13732ae491122eaacd12098b2"
            "97ddbb0180f0f6bdfaeee555be491aae"};
        if (header.GetHash() != seed) {
            std::cerr << "FAIL: serialized header SHA256d reference\n";
            return 1;
        }
        const auto header_digest{tao::experimental::RandomXHeaderHashV2(
            header, seed)};
        if (header_digest.GetHex() !=
            "80b59afe417178f46435a111204c14604"
            "d13971b812588e612597a50f9717085") {
            std::cerr << "FAIL: RandomX v2 header reference vector\n";
            return 1;
        }

        // Qualify the production-facing cached adapter
        // before constructing the full-memory dataset.
        QualifyCachedHeaderHash(header, seed);

        // FULL-MEMORY TAOX HEADER PARITY
        // Use the existing pinned 80-byte CBlockHeader fixture.
        // This is an offline adapter test, not network qualification.
        const std::span<const unsigned char> seed_bytes{
            seed.data(), seed.size()};
        const auto seed_key = std::as_bytes(seed_bytes);

        tao::experimental::RandomXLightHasher light{seed_key};
        tao::experimental::RandomXFullHasher full{seed_key, 2};

        const uint256 full_reference = full.HashHeader(header);

        if (full_reference != header_digest) {
            std::cerr << "FAIL: full-memory reference hash mismatch\n";
            return 1;
        }

        for (const uint32_t nonce : {0U, 1U, 5U, 17U}) {
            CBlockHeader candidate = header;
            candidate.nNonce = nonce;

            const uint256 expected = light.HashHeader(candidate);

            for (std::size_t worker = 0; worker < 2; ++worker) {
                const uint256 actual =
                    full.HashHeaderForWorker(worker, candidate);

                if (actual != expected) {
                    std::cerr << "FAIL: TaoX full/light mismatch"
                              << " at nonce " << nonce
                              << " worker " << worker << '\n';
                    return 1;
                }
            }
        }

        std::cout
            << "PASS: full-memory TaoX reference hash\n"
            << "PASS: two workers match light-memory hashes"
            << " across four nonces\n";
        std::cout << "PASS: two pinned RandomX v2 reference vectors\n";
        std::cout << "NOTE: TAO consensus/mining integration NOT tested\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
