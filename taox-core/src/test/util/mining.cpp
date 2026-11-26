// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/mining.h>

#include <chain.h>
#include <tao/consensus/pow_algorithm.h>
#include <tao/consensus/pow_seed_tracker.h>
#include <stdexcept>
#include <tao/crypto/randomx.h>

#include <cstdint>
#include <limits>
#include <span>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <key_io.h>
#include <node/context.h>
#include <pow.h>
#include <primitives/transaction.h>
#include <test/util/script.h>
#include <util/check.h>
#include <validation.h>
#include <validationinterface.h>
#include <versionbits.h>

#include <algorithm>
#include <memory>

using node::BlockAssembler;
using node::NodeContext;

bool SolveContextualTestPoW(
    CBlockHeader& header,
    const CBlockIndex& parent,
    const Consensus::Params& consensus)
{
    std::optional<uint256> seed;

    {
        LOCK(cs_main);

        if (!parent.phashBlock ||
            parent.nHeight < 0 ||
            parent.nHeight == std::numeric_limits<int>::max() ||
            header.hashPrevBlock != parent.GetBlockHash() ||
            !parent.IsValid(BLOCK_VALID_TREE)) {
            return false;
        }

        const int height = parent.nHeight + 1;

        const auto algorithm =
            taox::consensus::SelectPoWAlgorithm(height, consensus);

        if (!algorithm) return false;

        if (*algorithm == taox::consensus::PoWAlgorithm::RandomX) {
            const auto seed_height =
                taox::consensus::RandomXSeedHeight(height, consensus);

            if (!seed_height ||
                *seed_height < 0 ||
                *seed_height > parent.nHeight) {
                return false;
            }

            const auto* ancestor =
                parent.GetAncestor(static_cast<int>(*seed_height));

            if (!ancestor ||
                !ancestor->phashBlock ||
                !ancestor->IsValid(BLOCK_VALID_TREE)) {
                return false;
            }

            seed = ancestor->GetBlockHash();
        }
    }

    std::unique_ptr<tao::experimental::RandomXLightHasher> hasher;

    if (seed) {
        const std::span<const unsigned char> seed_bytes{
            seed->data(), seed->size()
        };

        hasher =
            std::make_unique<tao::experimental::RandomXLightHasher>(
                std::as_bytes(seed_bytes));
    }

    constexpr uint64_t MAX_ATTEMPTS{1'000'000};

    for (uint64_t attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
        const uint256 hash = hasher
            ? hasher->HashHeader(header)
            : header.GetPoWHash();

        if (CheckProofOfWorkImpl(hash, header.nBits, consensus)) {
            return true;
        }

        if (header.nNonce == std::numeric_limits<uint32_t>::max()) {
            return false;
        }

        ++header.nNonce;
    }

    return false;
}

COutPoint generatetoaddress(const NodeContext& node, const std::string& address)
{
    const auto dest = DecodeDestination(address);
    assert(IsValidDestination(dest));
    BlockAssembler::Options assembler_options;
    assembler_options.coinbase_output_script = GetScriptForDestination(dest);
    assembler_options.include_dummy_extranonce = true;

    return MineBlock(node, assembler_options);
}

std::vector<std::shared_ptr<CBlock>> CreateBlockChain(size_t total_height, const CChainParams& params)
{
    std::vector<std::shared_ptr<CBlock>> ret{total_height};
    auto time{params.GenesisBlock().nTime};

    taox::consensus::RandomXSeedTracker tracker;
    tracker.SetVerifiedAnchor(
        0, params.GenesisBlock().GetHash());

    std::optional<uint256> active_seed;
    std::unique_ptr<tao::experimental::RandomXLightHasher> hasher;

    // NOTE: here `height` does not correspond to the block height but the block height - 1.
    for (size_t height{0}; height < total_height; ++height) {
        CBlock& block{*(ret.at(height) = std::make_shared<CBlock>())};

        CMutableTransaction coinbase_tx;
        coinbase_tx.nLockTime = static_cast<uint32_t>(height);
        coinbase_tx.vin.resize(1);
        coinbase_tx.vin[0].prevout.SetNull();
        coinbase_tx.vin[0].nSequence = CTxIn::MAX_SEQUENCE_NONFINAL; // Make sure timelock is enforced.
        coinbase_tx.vout.resize(1);
        coinbase_tx.vout[0].scriptPubKey = P2WSH_OP_TRUE;
        coinbase_tx.vout[0].nValue = GetBlockSubsidy(height + 1, params.GetConsensus());
        // Always include OP_0 as a dummy extraNonce.
        coinbase_tx.vin[0].scriptSig = CScript() << (height + 1) << OP_0;
        block.vtx = {MakeTransactionRef(std::move(coinbase_tx))};

        block.nVersion = VERSIONBITS_LAST_OLD_BLOCK_VERSION;
        block.hashPrevBlock = (height >= 1 ? *ret.at(height - 1) : params.GenesisBlock()).GetHash();
        block.hashMerkleRoot = BlockMerkleRoot(block);
        block.nTime = ++time;
        block.nBits = params.GenesisBlock().nBits;
        block.nNonce = 0;

        if (EnableFuzzDeterminism()) {
            while (!CheckProofOfWork(
                block.GetHash(), block.nBits,
                params.GetConsensus())) {
                ++block.nNonce;
                assert(block.nNonce);
            }

            continue;
        }

        const auto& consensus = params.GetConsensus();

        const auto context =
            tracker.GetNextHeaderContext(block, consensus);

        if (!context) {
            throw std::runtime_error(
                "Denzuve offline fixture lacks contextual PoW");
        }

        if (context->algorithm ==
            taox::consensus::PoWAlgorithm::RandomX) {

            if (!context->seed) {
                throw std::runtime_error(
                    "Denzuve offline fixture lacks RandomX seed");
            }

            if (!hasher || active_seed != context->seed) {
                hasher.reset();

                const auto& seed = *context->seed;

                const std::span<const unsigned char> seed_bytes{
                    seed.data(), seed.size()
                };

                hasher =
                    std::make_unique<
                        tao::experimental::RandomXLightHasher>(
                            std::as_bytes(seed_bytes));

                active_seed = context->seed;
            }
        } else {
            hasher.reset();
            active_seed.reset();
        }

        bool solved = false;

        for (uint64_t attempt = 0;
             attempt < 1'000'000;
             ++attempt) {

            const uint256 pow_hash = hasher
                ? hasher->HashHeader(block)
                : block.GetPoWHash();

            if (CheckProofOfWorkImpl(
                    pow_hash, block.nBits, consensus)) {
                solved = true;
                break;
            }

            if (block.nNonce ==
                std::numeric_limits<uint32_t>::max()) {
                break;
            }

            ++block.nNonce;
        }

        if (!solved) {
            throw std::runtime_error(
                "Denzuve offline fixture nonce search exhausted");
        }

        if (!tracker.RecordVerifiedHeader(
                tracker.VerifiedHeight() + 1,
                block.GetHash(),
                consensus)) {
            throw std::runtime_error(
                "Denzuve offline fixture ancestry advancement failed");
        }
    }
    return ret;
}

COutPoint MineBlock(const NodeContext& node, const node::BlockAssembler::Options& assembler_options)
{
    auto block = PrepareBlock(node, assembler_options);
    auto valid = MineBlock(node, block);
    assert(!valid.IsNull());
    return valid;
}

struct BlockValidationStateCatcher : public CValidationInterface {
    const uint256 m_hash;
    std::optional<BlockValidationState> m_state;

    BlockValidationStateCatcher(const uint256& hash)
        : m_hash{hash},
          m_state{} {}

protected:
    void BlockChecked(const std::shared_ptr<const CBlock>& block, const BlockValidationState& state) override
    {
        if (block->GetHash() != m_hash) return;
        m_state = state;
    }
};

COutPoint MineBlock(const NodeContext& node, std::shared_ptr<CBlock>& block)
{
    if (EnableFuzzDeterminism()) {
        while (!CheckProofOfWork(
            block->GetHash(), block->nBits, Params().GetConsensus())) {
            ++block->nNonce;
            assert(block->nNonce);
        }
    } else {
        auto& chainman = *Assert(node.chainman);
        const CBlockIndex* parent = nullptr;

        {
            LOCK(cs_main);
            parent = chainman.m_blockman.LookupBlockIndex(
                block->hashPrevBlock);
        }

        if (!parent ||
            !SolveContextualTestPoW(
                *block, *parent, chainman.GetConsensus())) {
            return {};
        }

        block->fChecked = false;
    }

    return ProcessBlock(node, block);
}

COutPoint ProcessBlock(const NodeContext& node, const std::shared_ptr<CBlock>& block)
{
    auto& chainman{*Assert(node.chainman)};
    const auto old_height = WITH_LOCK(chainman.GetMutex(), return chainman.ActiveHeight());
    bool new_block;
    BlockValidationStateCatcher bvsc{block->GetHash()};
    node.validation_signals->RegisterValidationInterface(&bvsc);
    const bool processed{chainman.ProcessNewBlock(block, true, true, &new_block)};
    const bool duplicate{!new_block && processed};
    assert(!duplicate);
    node.validation_signals->UnregisterValidationInterface(&bvsc);
    node.validation_signals->SyncWithValidationInterfaceQueue();
    const bool was_valid{bvsc.m_state && bvsc.m_state->IsValid()};
    assert(old_height + was_valid == WITH_LOCK(chainman.GetMutex(), return chainman.ActiveHeight()));

    if (was_valid) return {block->vtx[0]->GetHash(), 0};
    return {};
}

std::shared_ptr<CBlock> PrepareBlock(const NodeContext& node,
                                     const BlockAssembler::Options& assembler_options)
{
    auto block = std::make_shared<CBlock>(
        BlockAssembler{Assert(node.chainman)->ActiveChainstate(), Assert(node.mempool.get()), assembler_options}
            .CreateNewBlock()
            ->block);

    LOCK(cs_main);
    block->nTime = Assert(node.chainman)->ActiveChain().Tip()->GetMedianTimePast() + 1;
    block->hashMerkleRoot = BlockMerkleRoot(*block);

    return block;
}
std::shared_ptr<CBlock> PrepareBlock(const NodeContext& node, const CScript& coinbase_scriptPubKey)
{
    BlockAssembler::Options assembler_options;
    assembler_options.coinbase_output_script = coinbase_scriptPubKey;
    assembler_options.include_dummy_extranonce = true;
    ApplyArgsManOptions(*node.args, assembler_options);
    return PrepareBlock(node, assembler_options);
}
