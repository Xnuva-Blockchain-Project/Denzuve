// TaoX experimental RandomX two-phase header-sync regression.
// Genuine regtest block fixtures. Not a production release test.

#include <arith_uint256.h>
#include <chain.h>
#include <headerssync.h>
#include <kernel/chainparams.h>
#include <pow.h>
#include <validation.h>
#include <tao/consensus/pow_seed_tracker.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Require(bool condition, const char* name)
{
    std::cout << (condition ? "PASS: " : "FAIL: ")
              << name << '\n';

    if (!condition) throw std::runtime_error(name);
}

unsigned char Digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;

    throw std::runtime_error("Invalid hexadecimal digit");
}

CBlockHeader Decode(const std::string& hex)
{
    if (hex.size() != 160)
        throw std::runtime_error("Expected 80-byte header");

    std::vector<unsigned char> bytes;
    bytes.reserve(80);

    for (size_t i = 0; i < hex.size(); i += 2) {
        bytes.push_back(
            (Digit(hex[i]) << 4) | Digit(hex[i + 1]));
    }

    auto le32 = [&](size_t p) -> uint32_t {
        return uint32_t(bytes[p]) |
               (uint32_t(bytes[p + 1]) << 8) |
               (uint32_t(bytes[p + 2]) << 16) |
               (uint32_t(bytes[p + 3]) << 24);
    };

    CBlockHeader h;

    h.nVersion = static_cast<int32_t>(le32(0));

    std::copy_n(bytes.begin() + 4, 32,
                h.hashPrevBlock.begin());

    std::copy_n(bytes.begin() + 36, 32,
                h.hashMerkleRoot.begin());

    h.nTime = le32(68);
    h.nBits = le32(72);
    h.nNonce = le32(76);

    return h;
}

void RunTwoPhaseRegression()
{
    const CBlockHeader genesis = Decode(
        "010000000000000000000000000000000000000000000000000000000000000000000000621cfbc1985872aa3745237d99063212f9e658322f00648165386ecfac588dcb804e2269ffff7f2002000000"
    );

    const std::vector<CBlockHeader> blocks{
        Decode(
            "00000020ecf19aa8183998c254687574e264fa9c4bff6cb038774e809f558a842dcdee3fdd1c2edf8ac9c01a8c170cbd5d0be7cbe14a9c91f8ecfd0f2f442756fb438a4b4b06046bffff7f2000000000"
        ),
        Decode(
            "000000206a5a6193a90478e6921249bfa01eb1306132af6854e69c62b7e901cf1b939fcfef0b60d8984e0549cec4af78b4a3c9c070735c0adbf6eb3968bc61ba0b210196f806046bffff7f2003000000"
        ),
        Decode(
            "000000207849ca5a2243915aad6fd919a22499294628d7d87a80d7e3aa738c2cfc545fda1384bfcfb0e3783878421db81e4fc8db8d89a73cd89bdcb4e7a2d571043361b3700c046bffff7f2004000000"
        )
    };

    Require(
        genesis.GetHash().ToString() ==
        "3feecd2d848a559f804e7738b06cff4b9cfa64e274756854c2983918a89af1ec",
        "Authentic genesis block ID");

    Require(
        blocks[0].GetHash().ToString() ==
        "cf9f931bcf01e9b7629ce65468af326130b11ea0bf491292e67804a993615a6a",
        "Historical Scrypt block ID");

    Require(
        blocks[1].GetHash().ToString() ==
        "da5f54fc2c8c73aae3d7807ad8d72846299924a219d96fad5a9143225aca4978",
        "First RandomX block ID");

    Require(
        blocks[2].GetHash().ToString() ==
        "8f4e677c48ecd46fbc15f7aad498eda3c0794c580dabc765457e77f564282d9d",
        "Second RandomX block ID");

    Require(
        blocks[0].hashPrevBlock == genesis.GetHash() &&
        blocks[1].hashPrevBlock == blocks[0].GetHash() &&
        blocks[2].hashPrevBlock == blocks[1].GetHash(),
        "Continuous genuine blockchain headers");

    Consensus::Params consensus{};

    consensus.randomx_activation_height = 2;
    consensus.randomx_seed_epoch_length = 16;

    consensus.powLimit = uint256{
        "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
    };

    consensus.fPowAllowMinDifficultyBlocks = true;
    consensus.fPowNoRetargeting = true;

    const HeadersSyncParams settings{
        .commitment_period = 1,
        .redownload_buffer_size = 3
    };

    LOCK(cs_main);

    CBlockIndex anchor(genesis);
    const uint256 genesis_hash = genesis.GetHash();

    anchor.phashBlock = &genesis_hash;
    anchor.nHeight = 0;
    anchor.nChainWork = GetBlockProof(genesis);

    Require(
        anchor.RaiseValidity(BLOCK_VALID_TREE),
        "Authenticated test anchor");

    arith_uint256 threshold = anchor.nChainWork;

    for (const auto& block : blocks)
        threshold += GetBlockProof(block);

    HeadersSyncState sync(
        0, consensus, settings, anchor, threshold);

    std::cout << "\n=== PRESYNC ===\n";

    for (const auto& block : blocks) {
        auto result = sync.ProcessNextHeaders(
            std::span<const CBlockHeader>(&block, 1),
            true);

        Require(result.success, "PRESYNC header accepted");
        Require(result.request_more, "Next headers requested");
    }

    Require(
        sync.GetState() == HeadersSyncState::State::REDOWNLOAD,
        "PRESYNC transitioned to REDOWNLOAD");

    std::cout << "\n=== REDOWNLOAD ===\n";

    std::vector<CBlockHeader> released;

    for (size_t i = 0; i < blocks.size(); ++i) {
        auto result = sync.ProcessNextHeaders(
            std::span<const CBlockHeader>(&blocks[i], 1),
            i + 1 != blocks.size());

        Require(result.success, "REDOWNLOAD header accepted");

        released.insert(
            released.end(),
            result.pow_validated_headers.begin(),
            result.pow_validated_headers.end());
    }

    Require(released.size() == blocks.size(),
            "All genuine headers released");

    for (size_t i = 0; i < blocks.size(); ++i)
        Require(released[i].GetHash() == blocks[i].GetHash(),
                "Released header matches original");

    Require(
        sync.GetState() == HeadersSyncState::State::FINAL,
        "Session finalised correctly");
}


void RunInvalidPresyncRegression()
{
    std::cout << "\n=== INVALID RANDOMX PRESYNC ===\n";

    const CBlockHeader genesis = Decode(
        "010000000000000000000000000000000000000000000000000000000000000000000000621cfbc1985872aa3745237d99063212f9e658322f00648165386ecfac588dcb804e2269ffff7f2002000000"
    );

    const CBlockHeader block1 = Decode(
        "00000020ecf19aa8183998c254687574e264fa9c4bff6cb038774e809f558a842dcdee3fdd1c2edf8ac9c01a8c170cbd5d0be7cbe14a9c91f8ecfd0f2f442756fb438a4b4b06046bffff7f2000000000"
    );

    CBlockHeader invalid = Decode(
        "000000206a5a6193a90478e6921249bfa01eb1306132af6854e69c62b7e901cf1b939fcfef0b60d8984e0549cec4af78b4a3c9c070735c0adbf6eb3968bc61ba0b210196f806046bffff7f2003000000"
    );

    // Previously confirmed invalid by the actual RandomX
    // verifier and independently rejected by regtest Node C.
    invalid.nNonce = 5;

    Require(
        invalid.hashPrevBlock == block1.GetHash(),
        "Invalid header still connects to block 1");

    Consensus::Params consensus{};
    consensus.randomx_activation_height = 2;
    consensus.randomx_seed_epoch_length = 16;
    consensus.powLimit = uint256{
        "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
    };
    consensus.fPowAllowMinDifficultyBlocks = true;
    consensus.fPowNoRetargeting = true;

    const HeadersSyncParams settings{
        .commitment_period = 1,
        .redownload_buffer_size = 3
    };

    LOCK(cs_main);

    CBlockIndex anchor(genesis);
    const uint256 genesis_hash = genesis.GetHash();

    anchor.phashBlock = &genesis_hash;
    anchor.nHeight = 0;
    anchor.nChainWork = GetBlockProof(genesis);

    Require(
        anchor.RaiseValidity(BLOCK_VALID_TREE),
        "Negative-test anchor authenticated");

    const arith_uint256 threshold =
        anchor.nChainWork +
        GetBlockProof(block1) +
        GetBlockProof(invalid);

    HeadersSyncState sync(
        1, consensus, settings, anchor, threshold);

    const auto first = sync.ProcessNextHeaders(
        std::span<const CBlockHeader>(&block1, 1),
        true);

    Require(
        first.success && first.request_more,
        "Valid Scrypt first batch accepted");

    const auto rejected = sync.ProcessNextHeaders(
        std::span<const CBlockHeader>(&invalid, 1),
        true);

    Require(
        !rejected.success,
        "Invalid RandomX continuation rejected");

    Require(
        rejected.pow_validated_headers.empty(),
        "No unverified headers released");

    Require(
        sync.GetState() == HeadersSyncState::State::FINAL,
        "Failed presync session finalised");
}



void RunInvalidRedownloadRegression()
{
    std::cout << "\n=== INVALID RANDOMX REDOWNLOAD ===\n";

    const CBlockHeader genesis = Decode(
        "010000000000000000000000000000000000000000000000000000000000000000000000621cfbc1985872aa3745237d99063212f9e658322f00648165386ecfac588dcb804e2269ffff7f2002000000");

    const std::vector<CBlockHeader> blocks{
        Decode("00000020ecf19aa8183998c254687574e264fa9c4bff6cb038774e809f558a842dcdee3fdd1c2edf8ac9c01a8c170cbd5d0be7cbe14a9c91f8ecfd0f2f442756fb438a4b4b06046bffff7f2000000000"),
        Decode("000000206a5a6193a90478e6921249bfa01eb1306132af6854e69c62b7e901cf1b939fcfef0b60d8984e0549cec4af78b4a3c9c070735c0adbf6eb3968bc61ba0b210196f806046bffff7f2003000000"),
        Decode("000000207849ca5a2243915aad6fd919a22499294628d7d87a80d7e3aa738c2cfc545fda1384bfcfb0e3783878421db81e4fc8db8d89a73cd89bdcb4e7a2d571043361b3700c046bffff7f2004000000")
    };

    Consensus::Params consensus{};
    consensus.randomx_activation_height = 2;
    consensus.randomx_seed_epoch_length = 16;
    consensus.powLimit = uint256{
        "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"};
    consensus.fPowAllowMinDifficultyBlocks = true;
    consensus.fPowNoRetargeting = true;

    const HeadersSyncParams settings{
        .commitment_period = 1,
        .redownload_buffer_size = 3
    };

    CBlockHeader invalid = blocks[1];
    invalid.nNonce = 5;

    Require(
        invalid.hashPrevBlock == blocks[0].GetHash(),
        "Invalid RandomX header connects to valid block 1");

    LOCK(cs_main);

    CBlockIndex anchor(genesis);
    const uint256 genesis_hash = genesis.GetHash();

    anchor.phashBlock = &genesis_hash;
    anchor.nHeight = 0;
    anchor.nChainWork = GetBlockProof(genesis);

    Require(
        anchor.RaiseValidity(BLOCK_VALID_TREE),
        "REDOWNLOAD test anchor authenticated");

    arith_uint256 threshold = anchor.nChainWork;

    for (const auto& block : blocks) {
        threshold += GetBlockProof(block);
    }

    HeadersSyncState sync(
        26, consensus, settings, anchor, threshold);

    for (const auto& block : blocks) {
        const auto result = sync.ProcessNextHeaders(
            std::span<const CBlockHeader>(&block, 1),
            true);

        Require(
            result.success,
            "Valid PRESYNC header accepted");
    }

    Require(
        sync.GetState() == HeadersSyncState::State::REDOWNLOAD,
        "Valid PRESYNC reached REDOWNLOAD");

    const auto first = sync.ProcessNextHeaders(
        std::span<const CBlockHeader>(&blocks[0], 1),
        true);

    Require(
        first.success && first.request_more,
        "Valid Scrypt REDOWNLOAD header accepted");

    Require(
        first.pow_validated_headers.empty(),
        "No headers prematurely released");

    const auto rejected = sync.ProcessNextHeaders(
        std::span<const CBlockHeader>(&invalid, 1),
        true);

    Require(
        !rejected.success,
        "Invalid RandomX REDOWNLOAD header rejected");

    Require(
        rejected.pow_validated_headers.empty(),
        "Invalid REDOWNLOAD released no headers");

    Require(
        sync.GetState() == HeadersSyncState::State::FINAL,
        "Invalid REDOWNLOAD session finalised");
}

void RunInterruptedPresyncRegression()
{
    std::cout << "\n=== INTERRUPTED PRESYNC ===\n";

    const CBlockHeader genesis = Decode(
        "010000000000000000000000000000000000000000000000000000000000000000000000621cfbc1985872aa3745237d99063212f9e658322f00648165386ecfac588dcb804e2269ffff7f2002000000"
    );

    const CBlockHeader block1 = Decode(
        "00000020ecf19aa8183998c254687574e264fa9c4bff6cb038774e809f558a842dcdee3fdd1c2edf8ac9c01a8c170cbd5d0be7cbe14a9c91f8ecfd0f2f442756fb438a4b4b06046bffff7f2000000000"
    );

    Consensus::Params consensus{};
    consensus.randomx_activation_height = 2;
    consensus.randomx_seed_epoch_length = 16;
    consensus.powLimit = uint256{
        "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
    };
    consensus.fPowAllowMinDifficultyBlocks = true;
    consensus.fPowNoRetargeting = true;

    const HeadersSyncParams settings{
        .commitment_period = 1,
        .redownload_buffer_size = 3
    };

    LOCK(cs_main);

    CBlockIndex anchor(genesis);
    const uint256 genesis_hash = genesis.GetHash();

    anchor.phashBlock = &genesis_hash;
    anchor.nHeight = 0;
    anchor.nChainWork = GetBlockProof(genesis);

    Require(
        anchor.RaiseValidity(BLOCK_VALID_TREE),
        "Interrupted-test anchor authenticated");

    const arith_uint256 threshold =
        anchor.nChainWork +
        GetBlockProof(block1) +
        GetBlockProof(block1);

    HeadersSyncState sync(
        2, consensus, settings, anchor, threshold);

    Require(
        sync.GetState() == HeadersSyncState::State::PRESYNC,
        "Session starts in PRESYNC");

    // A non-full response means the peer has no more headers.
    // Its chain is valid so far but below the work threshold.
    const auto result = sync.ProcessNextHeaders(
        std::span<const CBlockHeader>(&block1, 1),
        false);

    Require(
        result.success,
        "Valid partial response processed");

    Require(
        !result.request_more,
        "No further headers requested");

    Require(
        result.pow_validated_headers.empty(),
        "No low-work headers released");

    Require(
        sync.GetState() == HeadersSyncState::State::FINAL,
        "Incomplete session finalised safely");
}


void RunCommitmentMismatchRegression()
{
    std::cout << "\n=== VALID SCRYPT COMMITMENT MISMATCH ===\n";

    const CBlockHeader genesis = Decode(
        "010000000000000000000000000000000000000000000000000000000000000000000000621cfbc1985872aa3745237d99063212f9e658322f00648165386ecfac588dcb804e2269ffff7f2002000000"
    );

    const std::vector<CBlockHeader> original{
        Decode(
            "00000020ecf19aa8183998c254687574e264fa9c4bff6cb038774e809f558a842dcdee3fdd1c2edf8ac9c01a8c170cbd5d0be7cbe14a9c91f8ecfd0f2f442756fb438a4b4b06046bffff7f2000000000"
        ),
        Decode(
            "000000206a5a6193a90478e6921249bfa01eb1306132af6854e69c62b7e901cf1b939fcfef0b60d8984e0549cec4af78b4a3c9c070735c0adbf6eb3968bc61ba0b210196f806046bffff7f2003000000"
        ),
        Decode(
            "000000207849ca5a2243915aad6fd919a22499294628d7d87a80d7e3aa738c2cfc545fda1384bfcfb0e3783878421db81e4fc8db8d89a73cd89bdcb4e7a2d571043361b3700c046bffff7f2004000000"
        )
    };

    Consensus::Params consensus{};
    consensus.randomx_activation_height = 2;
    consensus.randomx_seed_epoch_length = 16;
    consensus.powLimit = uint256{
        "7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
    };
    consensus.fPowAllowMinDifficultyBlocks = true;
    consensus.fPowNoRetargeting = true;

    const HeadersSyncParams settings{
        .commitment_period = 1,
        .redownload_buffer_size = 3
    };

    // Search for a genuine Scrypt-valid alternative block 1.
    CBlockHeader alternative = original[0];
    bool found_valid = false;

    for (uint32_t nonce = 1; nonce < 4096; ++nonce) {
        alternative.nNonce = nonce;

        if (alternative.GetHash() == original[0].GetHash())
            continue;

        if (CheckProofOfWork(
                alternative.GetPoWHash(),
                alternative.nBits,
                consensus)) {
            found_valid = true;
            break;
        }
    }

    Require(found_valid, "Alternative Scrypt proof found");

    Require(
        alternative.hashPrevBlock == genesis.GetHash(),
        "Alternative connects to genesis");

    Require(
        alternative.GetHash() != original[0].GetHash(),
        "Alternative block ID differs");

    Require(
        CheckProofOfWork(
            alternative.GetPoWHash(),
            alternative.nBits,
            consensus),
        "Alternative has valid Scrypt proof");

    Require(
        PermittedDifficultyTransition(
            consensus, 1, genesis.nBits, alternative.nBits),
        "Alternative difficulty transition permitted");

    LOCK(cs_main);

    CBlockIndex anchor(genesis);
    const uint256 genesis_hash = genesis.GetHash();

    anchor.phashBlock = &genesis_hash;
    anchor.nHeight = 0;
    anchor.nChainWork = GetBlockProof(genesis);

    Require(
        anchor.RaiseValidity(BLOCK_VALID_TREE),
        "Commitment-test anchor authenticated");

    arith_uint256 threshold = anchor.nChainWork;
    for (const auto& block : original)
        threshold += GetBlockProof(block);

    // First independently establish that the alternative
    // is acceptable during PRESYNC, not merely a header
    // that happens to pass one standalone PoW check.
    {
        HeadersSyncState alternative_sync(
            30, consensus, settings, anchor, threshold);

        const auto result = alternative_sync.ProcessNextHeaders(
            std::span<const CBlockHeader>(&alternative, 1),
            true);

        Require(
            result.success &&
            alternative_sync.GetState() ==
                HeadersSyncState::State::PRESYNC,
            "Alternative accepted independently in PRESYNC");
    }

    // The salted one-bit commitment may collide.
    // Use fresh sessions and require a real rejection.
    bool mismatch_detected = false;
    int attempts = 0;

    for (int i = 0; i < 32; ++i) {
        HeadersSyncState sync(
            40 + i, consensus, settings, anchor, threshold);

        const auto presync = sync.ProcessNextHeaders(
            std::span<const CBlockHeader>(
                original.data(), original.size()),
            true);

        Require(
            presync.success &&
            sync.GetState() ==
                HeadersSyncState::State::REDOWNLOAD,
            "Original chain reached REDOWNLOAD");

        const auto redownload = sync.ProcessNextHeaders(
            std::span<const CBlockHeader>(&alternative, 1),
            true);

        ++attempts;

        if (!redownload.success) {
            Require(
                redownload.pow_validated_headers.empty(),
                "Rejected alternative released no headers");

            Require(
                sync.GetState() ==
                    HeadersSyncState::State::FINAL,
                "Mismatched session finalised");

            mismatch_detected = true;
            break;
        }

        Require(
            redownload.pow_validated_headers.empty(),
            "Matching commitment remains buffered");

        Require(
            sync.GetState() ==
                HeadersSyncState::State::REDOWNLOAD,
            "Matching commitment keeps session active");
    }

    std::cout << "Commitment trials: "
              << attempts << '\n';

    Require(
        mismatch_detected,
        "Valid alternative chain rejected by commitment");
}


void RunSeedBoundaryRegression()
{
    using taox::consensus::RandomXSeedTracker;
    using taox::consensus::PoWAlgorithm;

    std::cout << "\n=== RANDOMX SEED BOUNDARIES ===\n";

    Consensus::Params consensus{};
    consensus.randomx_activation_height = 2;
    consensus.randomx_seed_epoch_length = 16;

    const CBlockHeader genesis = Decode(
        "010000000000000000000000000000000000000000000000000000000000000000000000621cfbc1985872aa3745237d99063212f9e658322f00648165386ecfac588dcb804e2269ffff7f2002000000"
    );

    RandomXSeedTracker tracker;
    tracker.SetVerifiedAnchor(0, genesis.GetHash());

    uint256 previous = genesis.GetHash();
    uint256 seed1;
    uint256 seed17;
    uint256 seed33;

    for (int height = 1; height <= 34; ++height) {
        CBlockHeader header{};
        header.nVersion = 1;
        header.hashPrevBlock = previous;
        header.nTime = 1700000000U + height;
        header.nBits = 0x207fffff;
        header.nNonce = height;

        const auto context =
            tracker.GetNextHeaderContext(header, consensus);

        Require(
            context.has_value(),
            "Sequential header has seed context");

        if (height == 1) {
            Require(
                context->algorithm == PoWAlgorithm::Scrypt,
                "Block 1 selects historical Scrypt");
        } else {
            Require(
                context->algorithm == PoWAlgorithm::RandomX,
                "Post-activation block selects RandomX");
        }

        if (height == 2 || height == 17) {
            Require(
                context->seed.has_value() &&
                *context->seed == seed1,
                "First epoch uses block 1 seed");
        }

        if (height == 18 || height == 33) {
            Require(
                context->seed.has_value() &&
                *context->seed == seed17,
                "Second epoch uses block 17 seed");
        }

        if (height == 34) {
            Require(
                context->seed.has_value() &&
                *context->seed == seed33,
                "Third epoch uses block 33 seed");
        }

        // Synthetic headers are used solely to test the
        // seed tracker's sequential state bookkeeping.
        // This is NOT proof-of-work validation.
        Require(
            tracker.RecordVerifiedHeader(
                height, header.GetHash(), consensus),
            "Synthetic sequential state recorded");

        previous = header.GetHash();

        if (height == 1) seed1 = previous;
        if (height == 17) seed17 = previous;
        if (height == 33) seed33 = previous;
    }

    Require(
        tracker.VerifiedHeight() == 34,
        "Tracker reached height 34");

    Require(
        tracker.GetSeed(33).has_value() &&
        *tracker.GetSeed(33) == seed33,
        "Active epoch seed retained");

    std::cout << "\n=== REJECTED BATCH AT BOUNDARY ===\n";

    RandomXSeedTracker rejected_tracker;
    rejected_tracker.SetVerifiedAnchor(0, genesis.GetHash());

    previous = genesis.GetHash();

    for (int height = 1; height <= 17; ++height) {
        CBlockHeader header{};
        header.nVersion = 1;
        header.hashPrevBlock = previous;
        header.nTime = 1700000000U + height;
        header.nBits = 0x207fffff;
        header.nNonce = height;

        Require(
            rejected_tracker.RecordVerifiedHeader(
                height, header.GetHash(), consensus),
            "Boundary setup recorded");

        previous = header.GetHash();
    }

    CBlockHeader rejected_header{};
    rejected_header.nVersion = 1;
    rejected_header.hashPrevBlock = previous;
    rejected_header.nTime = 1700000018U;
    rejected_header.nBits = 0x207fffff;
    rejected_header.nNonce = 18;

    const auto before =
        rejected_tracker.GetSeed(17);

    Require(
        before.has_value() && *before == previous,
        "Block 17 seed available before rejection");

    const bool accepted =
        rejected_tracker.ValidateAndRecordBatch(
            std::span<const CBlockHeader>(
                &rejected_header, 1),
            consensus,
            [](const CBlockHeader&,
               const RandomXSeedTracker::NextHeaderContext&) {
                return false;
            });

    Require(
        !accepted,
        "Rejected verification callback aborts batch");

    Require(
        rejected_tracker.VerifiedHeight() == 17,
        "Rejected batch cannot advance height");

    Require(
        rejected_tracker.GetSeed(17) == before,
        "Rejected batch cannot alter seed");
}

} // namespace

int main()
{
    try {
        RunTwoPhaseRegression();
        RunInvalidPresyncRegression();
        RunInvalidRedownloadRegression();
        RunInterruptedPresyncRegression();
        RunCommitmentMismatchRegression();
        RunSeedBoundaryRegression();
        std::cout << "\nALL REGRESSION TESTS PASSED\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "REGRESSION FAILURE: "
                  << e.what() << '\n';
        return 1;
    }
}
