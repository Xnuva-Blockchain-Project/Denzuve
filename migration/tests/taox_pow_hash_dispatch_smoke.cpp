// TaoX experimental modular PoW hashing regression.
//
// Uses real Scrypt and pinned RandomX V2 hashing.
// The header and seed are laboratory fixtures.
//
// DOES NOT verify consensus, block acceptance,
// activation height or production seed derivation.
//
// DO NOT RELEASE.

#include <tao/consensus/pow_hash_dispatch.h>
#include <tao/consensus/pow_algorithm.h>

#include <iostream>
#include <optional>

int main()
{
    using namespace taox::consensus;

    CBlockHeader header{};
    header.nVersion = 1;
    header.hashPrevBlock.SetNull();
    header.hashMerkleRoot = uint256{
        "fb923316be9e4f5db3467d8c2d159bb"
        "3a27fa9ba2c71703c078a0ea7ccb676e8"
    };
    header.nTime = 1788642000;
    header.nBits = 0x207fffff;
    header.nNonce = 0;

    const uint256 seed{
        "2d95b3b13732ae491122eaacd12098b2"
        "97ddbb0180f0f6bdfaeee555be491aae"
    };

    // Preserve SHA256d block identity.
    if (header.GetHash() != seed) {
        std::cerr << "FAIL: block identifier\n";
        return 1;
    }

    std::cout << "PASS: SHA256d block identity\n";

    // Preserve existing historical Scrypt hashing.
    const auto scrypt = CalculatePoWHash(
        header, PoWAlgorithm::Scrypt, std::nullopt);

    if (!scrypt || *scrypt != header.GetPoWHash()) {
        std::cerr << "FAIL: Scrypt dispatch\n";
        return 2;
    }

    std::cout << "PASS: historical Scrypt dispatch\n";

    // Existing pinned RandomX V2 reference vector.
    // Seed is explicit laboratory input.
    const auto randomx = CalculatePoWHash(
        header, PoWAlgorithm::RandomX, seed);

    if (!randomx ||
        randomx->GetHex() !=
        "80b59afe417178f46435a111204c14604"
        "d13971b812588e612597a50f9717085") {
        std::cerr << "FAIL: RandomX V2 dispatch\n";
        return 3;
    }

    std::cout << "PASS: pinned RandomX V2 vector\n";

    if (CalculatePoWHash(
            header,
            PoWAlgorithm::RandomX,
            std::nullopt)) {
        std::cerr << "FAIL: missing RandomX seed\n";
        return 4;
    }

    std::cout << "PASS: missing seed fails closed\n";

    if (CalculatePoWHash(
            header,
            std::nullopt,
            std::nullopt)) {
        std::cerr << "FAIL: missing algorithm\n";
        return 5;
    }

    std::cout << "PASS: missing algorithm fails closed\n";

    if (SelectPoWAlgorithm(-1).has_value()) {
        std::cerr << "FAIL: negative height\n";
        return 6;
    }

    for (const int height : {0, 1, 636, 637, 1000}) {
        const auto algorithm = SelectPoWAlgorithm(height);

        if (!algorithm ||
            *algorithm != PoWAlgorithm::Scrypt) {
            std::cerr << "FAIL: selector changed\n";
            return 7;
        }
    }

    std::cout << "PASS: Scrypt-only selector\n";

    if (header.GetHash() != seed) {
        std::cerr << "FAIL: block identity changed\n";
        return 8;
    }

    std::cout << "PASS: block identity unchanged\n";

    std::cout
        << "NOTE: real cryptographic hashes\n"
        << "NOTE: synthetic consensus context\n"
        << "NOTE: no difficulty validation\n"
        << "NOTE: no block acceptance testing\n"
        << "NOTE: no approved seed schedule\n"
        << "NOTE: RandomX mainnet disabled\n";

    return 0;
}
