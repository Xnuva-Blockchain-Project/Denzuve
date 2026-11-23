// Copyright (c) 2009-present The Bitcoin Core developers
// Copyright (c) 2017-present The TaoCoin developers
// TAO Revival migration draft: UNTESTED / DO NOT RELEASE.

#include <kernel/chainparams.h>

#include <consensus/amount.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <pow.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <vector>

namespace {

static CBlock CreateTAOGenesisBlock()
{
    const char* pszTimestamp =
        "Betoota Advocate 21Dec2017 Bloke That Got Caught Up Talking Bitcoin";
    const CScript genesisOutputScript =
        CScript() << ParseHex("04defb2ebff8d065d863eeb265f549bf0d89a6f21475a117b85380802bf0480bd6ab7685e84aadfa3316b803ec74d251a175b37713232e5ec48ed7b0b486d3e3bc")
                  << OP_CHECKSIG;

    CMutableTransaction txNew;
    txNew.version = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig =
        CScript() << 486604799 << CScriptNum(4)
                  << std::vector<unsigned char>(
                         reinterpret_cast<const unsigned char*>(pszTimestamp),
                         reinterpret_cast<const unsigned char*>(pszTimestamp) + std::strlen(pszTimestamp));
    txNew.vout[0].nValue = 50 * COIN;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nVersion = 1;
    genesis.nTime = 1513853104;
    genesis.nBits = 0x1e0ffff0;
    genesis.nNonce = 1349876;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static void DisableAllVersionBits(Consensus::Params& consensus)
{
    for (auto& deployment : consensus.vDeployments) {
        deployment.nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        deployment.nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        deployment.min_activation_height = 0;
        deployment.threshold = 0;
        deployment.period = 0;
    }
}

class CMainParams final : public CChainParams
{
public:
    CMainParams()
    {
        m_chain_type = ChainType::MAIN;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();

        consensus.nSubsidyHalvingInterval = 210000;

        // TAO Revival only introduces the serialized-height/version-2 rule
        // at height 710000. Later Bitcoin softforks did not exist in Revival.
        consensus.BIP34Height = 710000;
        consensus.BIP34Hash = uint256{}; // intentionally unknown; keeps BIP30 checks enabled
        consensus.BIP65Height = std::numeric_limits<int>::max();
        consensus.BIP66Height = std::numeric_limits<int>::max();
        consensus.CSVHeight = std::numeric_limits<int>::max();
        consensus.SegwitHeight = std::numeric_limits<int>::max();
        consensus.MinBIP9WarningHeight = std::numeric_limits<int>::max();

        DisableAllVersionBits(consensus);

        consensus.powLimit =
            uint256{"00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"};
        // Mainnet activation requires a separately approved height.
        consensus.randomx_activation_height = -1;
        consensus.randomx_seed_epoch_length = 0;
        consensus.nPowTargetTimespan = 10 * 60;
        consensus.nPowTargetSpacing = 5 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.enforce_BIP94 = false;
        consensus.fPowNoRetargeting = false;

        // No assumevalid shortcut until the Revival history has been revalidated.
        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

        pchMessageStart = {0xb3, 0x3d, 0xdc, 0x92};
        nDefaultPort = 31137;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;

        genesis = CreateTAOGenesisBlock();
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock ==
               uint256{"41de2a4b7018115eae4a3e7e82189b551aaf871af66929fa28a49ee852abb67e"});
        assert(genesis.hashMerkleRoot ==
               uint256{"cb8d58accf6e38658164002f3258e6f9123206997d234537aa725898c1fb1c62"});

        // Revival source has only explicit genesis/height-1 checkpoints and
        // no authoritative modern-format seed set in this migration phase.
        vSeeds.clear();
        vFixedSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = {65};
        base58Prefixes[SCRIPT_ADDRESS] = {5};
        base58Prefixes[SECRET_KEY] = {193};

        // Revival predates HD extended-key and Bech32 formats. These extended
        // key prefixes are temporary modern-wallet plumbing, not historical
        // TAO consensus identity, and are release-gated pending wallet review.
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};
        bech32_hrp = "tao-disabled";

        fDefaultConsistencyChecks = false;
        m_is_mockable_chain = false;
        m_assumeutxo_data.clear();
        chainTxData = {1513853104, 1, 0.0};

        // Non-consensus header-sync memory tuning; calibrate during testing.
        m_headers_sync_params = {.commitment_period = 673, .redownload_buffer_size = 14460};
    }
};


class CRegTestParams final : public CChainParams
{
public:
    CRegTestParams()
    {
        m_chain_type = ChainType::REGTEST;

        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210000;

        consensus.BIP34Height = std::numeric_limits<int>::max();
        consensus.BIP34Hash.SetNull();
        consensus.BIP65Height = std::numeric_limits<int>::max();
        consensus.BIP66Height = std::numeric_limits<int>::max();
        consensus.CSVHeight = std::numeric_limits<int>::max();
        consensus.SegwitHeight = std::numeric_limits<int>::max();
        consensus.MinBIP9WarningHeight = std::numeric_limits<int>::max();
        DisableAllVersionBits(consensus);

        consensus.powLimit =
            uint256{"7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"};
        consensus.nPowTargetTimespan = 600;
        consensus.nPowTargetSpacing = 300;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.enforce_BIP94 = false;

        // Regtest ONLY: one historical Scrypt block followed by RandomX.
        consensus.randomx_activation_height = 2;
        consensus.randomx_seed_epoch_length = 16;

        consensus.nMinimumChainWork.SetNull();
        consensus.defaultAssumeValid.SetNull();

        // Distinct from TaoX mainnet.
        pchMessageStart = {0x74, 0x61, 0x78, 0x52};
        nDefaultPort = 31337;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;

        genesis = CreateTAOGenesisBlock();

        // Regtest-specific header. Preserve the genesis transaction
        // construction but never reuse the mainnet genesis hash.
        genesis.nTime = 1763856000;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 0;

        // Deterministically find a valid Scrypt regtest genesis nonce.
        while (!CheckProofOfWork(genesis.GetPoWHash(),
                                 genesis.nBits, consensus)) {
            if (genesis.nNonce == std::numeric_limits<uint32_t>::max())
                throw std::runtime_error("TaoX regtest genesis nonce exhausted");
            ++genesis.nNonce;
        }

        consensus.hashGenesisBlock = genesis.GetHash();

        // Pinned TaoX RandomX regtest genesis, independently observed
        // through the running node's getblockchaininfo and getblockhash RPCs.
        const uint256 expected_regtest_genesis{
            "3feecd2d848a559f804e7738b06cff4b9cfa64e274756854c2983918a89af1ec"
        };

        if (consensus.hashGenesisBlock != expected_regtest_genesis)
            throw std::runtime_error(
                "TaoX regtest genesis identity mismatch");

        if (consensus.hashGenesisBlock ==
            CChainParams::Main()->GetConsensus().hashGenesisBlock)
            throw std::runtime_error(
                "TaoX regtest genesis matches mainnet");

        vSeeds.clear();
        vFixedSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = {111};
        base58Prefixes[SCRIPT_ADDRESS] = {196};
        base58Prefixes[SECRET_KEY] = {239};
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        bech32_hrp = "taoxrt";

        fDefaultConsistencyChecks = true;
        m_is_mockable_chain = true;
        m_assumeutxo_data.clear();
        chainTxData = {1763856000, 1, 0.0};
        m_headers_sync_params = {
            .commitment_period = 673,
            .redownload_buffer_size = 14460
        };
    }
};

} // namespace

std::unique_ptr<const CChainParams> CChainParams::Main()
{
    return std::make_unique<const CMainParams>();
}

std::unique_ptr<const CChainParams> CChainParams::TestNet()
{
    throw std::runtime_error(
        "TAO Revival testnet is unresolved: authoritative source contains a blank genesis placeholder");
}

std::unique_ptr<const CChainParams> CChainParams::TestNet4()
{
    throw std::runtime_error("Bitcoin testnet4 is not a TAO Revival network");
}

std::unique_ptr<const CChainParams> CChainParams::SigNet(const SigNetOptions&)
{
    throw std::runtime_error("Signet is not a TAO Revival network");
}

std::unique_ptr<const CChainParams> CChainParams::RegTest(const RegTestOptions&)
{
    return std::make_unique<const CRegTestParams>();
}

std::vector<int> CChainParams::GetAvailableSnapshotHeights() const
{
    return {};
}

std::optional<ChainType> GetNetworkForMagic(const MessageStartChars& message)
{
    const auto mainnet = CChainParams::Main()->MessageStart();
    if (std::ranges::equal(message, mainnet)) return ChainType::MAIN;
    const auto regtest = CChainParams::RegTest({})->MessageStart();
    if (std::ranges::equal(message, regtest)) return ChainType::REGTEST;
    return std::nullopt;
}
