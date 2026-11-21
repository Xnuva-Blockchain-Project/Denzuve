// TaoX branch-specific ancestry and validity regression.
// LAB ONLY — synthetic objects and manually assigned statuses.
// No activation, consensus seed schedule or PoW validation.

#include <chain.h>
#include <kernel/cs_main.h>

#include <array>
#include <iostream>
#include <optional>

RecursiveMutex cs_main;

enum class Result { Ready, Defer, Reject };

struct LookupResult {
    Result result;
    std::optional<uint256> identifier;
};

// Prototype only. The requested ancestor height is
// supplied by the test, NOT a consensus seed schedule.
LookupResult LookupCheckedAncestor(
    const CBlockIndex* parent,
    int requested_height)
{
    if (!parent || requested_height < 0 ||
        requested_height > parent->nHeight)
        return {Result::Defer, std::nullopt};

    const CBlockIndex* seed =
        parent->GetAncestor(requested_height);

    if (!seed)
        return {Result::Defer, std::nullopt};

    // Inspect the candidate's own indexed ancestry.
    // This does not establish actual PoW validity.
    for (const CBlockIndex* p = parent;
         p != nullptr; p = p->pprev) {
        if (p->nStatus & BLOCK_FAILED_VALID)
            return {Result::Reject, std::nullopt};

        if (!p->IsValid(BLOCK_VALID_TREE))
            return {Result::Defer, std::nullopt};
    }

    if (!seed->phashBlock)
        return {Result::Defer, std::nullopt};

    return {Result::Ready, seed->GetBlockHash()};
}

int main()
{
    LOCK(cs_main);

    std::array<CBlockIndex, 7> blocks{};
    std::array<uint256, 7> ids{};

    for (int i = 0; i < 7; ++i) {
        ids[i].data()[0] =
            static_cast<unsigned char>(i + 1);
        blocks[i].phashBlock = &ids[i];
        if (!blocks[i].RaiseValidity(BLOCK_VALID_TREE))
            return 1;
    }

    // Main branch: 0-1-2-3-4
    for (int i = 0; i <= 4; ++i) {
        blocks[i].nHeight = i;
        if (i > 0)
            blocks[i].pprev = &blocks[i - 1];
        blocks[i].BuildSkip();
    }

    // Competing branch: 0-1-2-3'-4'
    blocks[5].nHeight = 3;
    blocks[5].pprev = &blocks[2];
    blocks[5].BuildSkip();

    blocks[6].nHeight = 4;
    blocks[6].pprev = &blocks[5];
    blocks[6].BuildSkip();

    // Height 3 is arbitrary laboratory input.
    auto main = LookupCheckedAncestor(&blocks[4], 3);
    auto fork = LookupCheckedAncestor(&blocks[6], 3);

    if (main.result != Result::Ready ||
        fork.result != Result::Ready ||
        !main.identifier || !fork.identifier ||
        *main.identifier != ids[3] ||
        *fork.identifier != ids[5] ||
        *main.identifier == *fork.identifier)
        return 2;

    std::cout << "PASS: branch-specific ancestry\n";

    if (LookupCheckedAncestor(nullptr, 3).result
            != Result::Defer ||
        LookupCheckedAncestor(&blocks[4], -1).result
            != Result::Defer ||
        LookupCheckedAncestor(&blocks[4], 5).result
            != Result::Defer)
        return 3;

    std::cout << "PASS: unavailable ancestry deferred\n";

    // An index lacking TREE validity must not
    // produce an apparently verified seed.
    blocks[5].nStatus = BLOCK_VALID_UNKNOWN;

    if (LookupCheckedAncestor(&blocks[6], 3).result
        != Result::Defer)
        return 4;

    blocks[5].RaiseValidity(BLOCK_VALID_TREE);

    std::cout << "PASS: insufficient validity deferred\n";

    // A failed shared ancestor invalidates
    // lookup on both competing branches.
    blocks[2].nStatus |= BLOCK_FAILED_VALID;

    if (LookupCheckedAncestor(&blocks[4], 3).result
            != Result::Reject ||
        LookupCheckedAncestor(&blocks[6], 3).result
            != Result::Reject)
        return 5;

    blocks[2].nStatus &= ~BLOCK_FAILED_VALID;

    std::cout << "PASS: failed ancestor rejected\n";

    blocks[5].phashBlock = nullptr;

    if (LookupCheckedAncestor(&blocks[6], 3).result
        != Result::Defer)
        return 6;

    std::cout << "PASS: missing seed identifier deferred\n";

    std::cout
        << "NOTE: statuses artificially assigned\n"
        << "NOTE: no actual block or PoW authenticated\n"
        << "NOTE: no consensus seed schedule defined\n"
        << "NOTE: no RandomX activation\n";

    return 0;
}
