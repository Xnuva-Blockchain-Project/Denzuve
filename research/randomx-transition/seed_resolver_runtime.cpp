#include <tao/consensus/pow_seed_resolver_experimental.h>

#include <kernel/cs_main.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

// Standalone test process only; no node is started.
decltype(cs_main) cs_main;

using namespace toax::test_only;

struct TestNode {
    uint256 hash{};
    CBlockIndex index{};
};

class TestTree {
public:
    CBlockIndex* Add(
        CBlockIndex* parent,
        uint32_t branch)
    {
        auto node = std::make_unique<TestNode>();
        auto* p = &node->index;

        p->pprev = parent;
        p->nHeight =
            parent ? parent->nHeight + 1 : 0;

        // Synthetic, unique identifiers for test nodes.
        const uint32_t height =
            static_cast<uint32_t>(p->nHeight);

        for (int i = 0; i < 4; ++i) {
            node->hash.data()[i] =
                (height >> (8 * i)) & 0xff;
            node->hash.data()[i + 4] =
                (branch >> (8 * i)) & 0xff;
        }

        p->phashBlock = &node->hash;
        p->nStatus = BLOCK_VALID_TREE;

        if (parent)
            p->BuildSkip();

        nodes.push_back(std::move(node));
        return p;
    }

    CBlockIndex* Extend(
        CBlockIndex* parent,
        int tip,
        uint32_t branch)
    {
        while (parent->nHeight < tip)
            parent = Add(parent, branch);
        return parent;
    }

private:
    std::vector<std::unique_ptr<TestNode>> nodes;
};

CBlockHeader Candidate(const CBlockIndex* parent)
{
    CBlockHeader header{};
    header.hashPrevBlock = parent->GetBlockHash();
    return header;
}

int main()
{
    LOCK(cs_main);

    constexpr Policy trial{1000, 16};
    constexpr Policy disabled{std::nullopt, 16};

    TestTree tree;

    auto* genesis = tree.Add(nullptr, 1);
    auto* common = tree.Extend(
        genesis, 1008, 1);

    auto* branch_a = tree.Extend(
        common, 1032, 2);
    auto* branch_b = tree.Extend(
        common, 1032, 3);

    assert(branch_a->GetBlockHash() !=
           branch_b->GetBlockHash());

    std::cout
        << "PASS: genuine CBlockIndex fork trees\n";

    auto* parent_999 =
        common->GetAncestor(999);

    auto seed_first = ResolveSeed(
        Candidate(parent_999),
        parent_999,
        trial);

    assert(seed_first);
    assert(*seed_first ==
           parent_999->GetBlockHash());

    std::cout
        << "PASS: preactivation ancestor seed\n";

    auto* a_1015 =
        branch_a->GetAncestor(1015);
    auto* b_1015 =
        branch_b->GetAncestor(1015);

    auto seed_a = ResolveSeed(
        Candidate(a_1015), a_1015, trial);
    auto seed_b = ResolveSeed(
        Candidate(b_1015), b_1015, trial);

    assert(seed_a && seed_b);
    assert(*seed_a == a_1015->GetBlockHash());
    assert(*seed_b == b_1015->GetBlockHash());
    assert(*seed_a != *seed_b);

    std::cout
        << "PASS: fork-specific epoch seeds\n";

    auto* a_1031 =
        branch_a->GetAncestor(1031);
    auto* b_1031 =
        branch_b->GetAncestor(1031);

    auto next_a = ResolveSeed(
        Candidate(a_1031), a_1031, trial);
    auto next_b = ResolveSeed(
        Candidate(b_1031), b_1031, trial);

    assert(next_a && next_b);
    assert(*next_a == a_1031->GetBlockHash());
    assert(*next_b == b_1031->GetBlockHash());
    assert(*next_a != *next_b);

    std::cout
        << "PASS: subsequent epoch seeds\n";

    assert(!ResolveSeed(
        Candidate(a_1015), nullptr, trial));

    auto wrong_header = Candidate(a_1015);
    wrong_header.hashPrevBlock.SetNull();

    assert(!ResolveSeed(
        wrong_header, a_1015, trial));

    assert(!ResolveSeed(
        Candidate(a_1015), a_1015, disabled));

    std::cout
        << "PASS: missing/wrong parent rejected\n";
    std::cout
        << "PASS: disabled RandomX policy rejected\n";

    auto* invalid_ancestor =
        branch_a->GetAncestor(1015);

    const auto original_status =
        invalid_ancestor->nStatus;

    invalid_ancestor->nStatus = BLOCK_FAILED_VALID;

    assert(!ResolveSeed(
        Candidate(invalid_ancestor),
        invalid_ancestor,
        trial));

    invalid_ancestor->nStatus = original_status;

    std::cout
        << "PASS: failed parent rejected\n";

    // Epoch one: candidate 1015 uses seed 999.
    auto* parent_1014 =
        branch_a->GetAncestor(1014);
    auto* seed_999 =
        branch_a->GetAncestor(999);

    const auto original_999 = seed_999->nStatus;

    seed_999->nStatus = BLOCK_FAILED_VALID;

    assert(parent_1014->IsValid(BLOCK_VALID_TREE));

    assert(!ResolveSeed(
        Candidate(parent_1014),
        parent_1014,
        trial));

    seed_999->nStatus = original_999;

    assert(ResolveSeed(
        Candidate(parent_1014),
        parent_1014,
        trial));

    std::cout
        << "PASS: failed epoch-one seed rejected\n";

    // Epoch two: candidate 1031 uses seed 1015.
    auto* parent_1030 =
        branch_a->GetAncestor(1030);
    auto* seed_1015 =
        branch_a->GetAncestor(1015);

    const auto original_1015 =
        seed_1015->nStatus;

    seed_1015->nStatus = BLOCK_FAILED_VALID;

    assert(parent_1030->IsValid(BLOCK_VALID_TREE));

    assert(!ResolveSeed(
        Candidate(parent_1030),
        parent_1030,
        trial));

    seed_1015->nStatus = original_1015;

    assert(ResolveSeed(
        Candidate(parent_1030),
        parent_1030,
        trial));

    std::cout
        << "PASS: failed epoch-two seed rejected\n";

    std::cout
        << "PASS: real C++ ancestry runtime tests\n"
        << "NOTE: fabricated CBlockIndex records\n"
        << "NOTE: no real RandomX PoW verified\n"
        << "NOTE: mainnet activation disabled\n";
}
