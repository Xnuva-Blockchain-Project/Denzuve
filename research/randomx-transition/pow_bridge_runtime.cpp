#include <tao/consensus/pow_bridge_experimental.h>

#include <kernel/cs_main.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

decltype(cs_main) cs_main;

using namespace toax::test_only;

struct TestNode {
    uint256 id{};
    CBlockIndex index{};
};

class TestTree {
public:
    CBlockIndex* Add(CBlockIndex* parent,
                     uint32_t branch)
    {
        auto node = std::make_unique<TestNode>();
        auto* index = &node->index;

        index->pprev = parent;
        index->nHeight =
            parent ? parent->nHeight + 1 : 0;

        const auto height =
            static_cast<uint32_t>(index->nHeight);

        for (unsigned i = 0; i < 4; ++i) {
            node->id.data()[i] =
                (height >> (8 * i)) & 0xff;
            node->id.data()[i + 4] =
                (branch >> (8 * i)) & 0xff;
        }

        index->phashBlock = &node->id;
        index->nStatus = BLOCK_VALID_TREE;

        if (parent) index->BuildSkip();

        nodes.push_back(std::move(node));
        return index;
    }

    CBlockIndex* Extend(CBlockIndex* parent,
                        int height,
                        uint32_t branch)
    {
        while (parent->nHeight < height) {
            parent = Add(parent, branch);
        }
        return parent;
    }

private:
    std::vector<std::unique_ptr<TestNode>> nodes;
};

CBlockHeader MakeCandidate(const CBlockIndex* parent)
{
    CBlockHeader header{};
    header.nVersion = 2;
    header.hashPrevBlock = parent->GetBlockHash();
    header.nTime = 1800000000;
    header.nBits = 0x1e0fffff;
    header.nNonce = 42;
    return header;
}

int main()
{
    LOCK(cs_main);

    constexpr Policy trial{1000, 16};
    constexpr Policy disabled{std::nullopt, 16};

    TestTree tree;

    auto* genesis = tree.Add(nullptr, 1);
    auto* common = tree.Extend(genesis, 1008, 1);

    auto* a = tree.Extend(common, 1015, 2);
    auto* b = tree.Extend(common, 1015, 3);

    std::cout << "PASS: constructed forked indexes\n";

    // Candidate 999 must still use historical Scrypt.
    auto* parent_998 = common->GetAncestor(998);
    auto historical = MakeCandidate(parent_998);

    auto scrypt = ResolveAndHash(
        historical, parent_998, trial);

    assert(scrypt);
    assert(*scrypt == historical.GetPoWHash());

    std::cout << "PASS: preactivation Scrypt hash\n";

    // Disabled policy must preserve Scrypt at all heights.
    auto candidate_a = MakeCandidate(a);

    auto disabled_hash = ResolveAndHash(
        candidate_a, a, disabled);

    assert(disabled_hash);
    assert(*disabled_hash == candidate_a.GetPoWHash());

    std::cout << "PASS: disabled RandomX policy\n";

    // Candidate 1016 uses ancestor 1015 as seed.
    auto randomx_a = ResolveAndHash(
        candidate_a, a, trial);

    assert(randomx_a);

    auto expected_a =
        tao::experimental::RandomXHeaderHashV2(
            candidate_a, a->GetBlockHash());

    assert(*randomx_a == expected_a);

    std::cout << "PASS: RandomX header hashing\n";
    std::cout << "PASS: correct ancestor seed\n";

    auto candidate_b = MakeCandidate(b);

    auto randomx_b = ResolveAndHash(
        candidate_b, b, trial);

    assert(randomx_b);
    assert(*randomx_a != *randomx_b);

    std::cout << "PASS: fork-specific RandomX results\n";

    auto bad_header = candidate_a;
    bad_header.hashPrevBlock.SetNull();

    assert(!ResolveAndHash(
        bad_header, a, trial));

    assert(!ResolveAndHash(
        candidate_a, nullptr, trial));

    const auto old_status = a->nStatus;
    a->nStatus = BLOCK_FAILED_VALID;

    assert(!ResolveAndHash(
        candidate_a, a, trial));

    a->nStatus = old_status;

    std::cout << "PASS: invalid contexts rejected\n";

    std::cout
        << "PASS: combined C++ PoW bridge runtime\n"
        << "NOTE: synthetic blockchain context\n"
        << "NOTE: no difficulty/target check\n"
        << "NOTE: no block accepted or mined\n"
        << "NOTE: mainnet RandomX remains disabled\n";
}
