// TaoX experimental contextual PoW verification contract.
//
// LAB ONLY — NO MAINNET ACTIVATION.
// This determines readiness to verify a PoW hash.
// It does not validate ancestry, calculate hashes,
// select an activation height or accept blocks.

#include <tao/consensus/pow_algorithm.h>

#include <iostream>
#include <optional>

using taox::consensus::PoWAlgorithm;
using taox::consensus::SelectPoWAlgorithm;

enum class Context {
    Verified,
    Unknown,
    Invalid
};

enum class Seed {
    Verified,
    Unknown,
    Invalid
};

enum class Decision {
    VerifyScrypt,
    VerifyRandomX,
    Defer,
    Reject
};

// The algorithm is supplied by a hypothetical,
// externally authenticated consensus policy.
// This function does NOT determine that policy.
Decision DecideVerification(
    Context context,
    std::optional<PoWAlgorithm> algorithm,
    Seed seed)
{
    if (context == Context::Invalid ||
        seed == Seed::Invalid) {
        return Decision::Reject;
    }

    if (context == Context::Unknown ||
        !algorithm.has_value()) {
        return Decision::Defer;
    }

    switch (*algorithm) {
    case PoWAlgorithm::Scrypt:
        // Historical Scrypt does not use a RandomX seed.
        return Decision::VerifyScrypt;

    case PoWAlgorithm::RandomX:
        if (seed != Seed::Verified) {
            return Decision::Defer;
        }
        return Decision::VerifyRandomX;
    }

    return Decision::Reject;
}

int main()
{
    // The actual production-facing selector must
    // remain strictly Scrypt-only.
    if (SelectPoWAlgorithm(-1).has_value())
        return 1;

    for (int height : {0, 1, 636, 637, 1000}) {
        auto algorithm = SelectPoWAlgorithm(height);

        if (!algorithm ||
            *algorithm != PoWAlgorithm::Scrypt) {
            return 2;
        }
    }

    std::cout
        << "PASS: Scrypt-only selector preserved\n";

    // Authenticated historical Scrypt context.
    if (DecideVerification(
            Context::Verified,
            PoWAlgorithm::Scrypt,
            Seed::Unknown) != Decision::VerifyScrypt) {
        return 3;
    }

    std::cout
        << "PASS: historical Scrypt verification permitted\n";

    // This is a hypothetical laboratory input,
    // NOT an activation of RandomX.
    if (DecideVerification(
            Context::Verified,
            PoWAlgorithm::RandomX,
            Seed::Verified) != Decision::VerifyRandomX) {
        return 4;
    }

    std::cout
        << "PASS: verified RandomX context permits verification\n";

    // A RandomX context with no validated seed
    // must never silently fall back to Scrypt.
    if (DecideVerification(
            Context::Verified,
            PoWAlgorithm::RandomX,
            Seed::Unknown) != Decision::Defer) {
        return 5;
    }

    if (DecideVerification(
            Context::Unknown,
            PoWAlgorithm::RandomX,
            Seed::Verified) != Decision::Defer) {
        return 6;
    }

    if (DecideVerification(
            Context::Verified,
            std::nullopt,
            Seed::Unknown) != Decision::Defer) {
        return 7;
    }

    std::cout
        << "PASS: incomplete context deferred\n";

    if (DecideVerification(
            Context::Invalid,
            PoWAlgorithm::RandomX,
            Seed::Verified) != Decision::Reject) {
        return 8;
    }

    if (DecideVerification(
            Context::Verified,
            PoWAlgorithm::RandomX,
            Seed::Invalid) != Decision::Reject) {
        return 9;
    }

    std::cout
        << "PASS: invalid context rejected\n";

    std::cout
        << "PASS: no silent algorithm fallback\n";

    std::cout
        << "NOTE: ancestry and seed are simulated inputs\n";
    std::cout
        << "NOTE: no actual PoW hash checked\n";
    std::cout
        << "NOTE: no RandomX activation enabled\n";

    return 0;
}
