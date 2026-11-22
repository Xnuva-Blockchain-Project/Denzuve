// XNUVA reference vectors reused independently in the TAO adapter.
// This is an isolated cryptographic self-test, NOT a TAO consensus test.
#include <tao/crypto/randomx.h>

#include <primitives/block.h>
#include <uint256.h>

#include <exception>
#include <iostream>
#include <span>
#include <string>

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
        std::cout << "PASS: two pinned RandomX v2 reference vectors\n";
        std::cout << "NOTE: TAO consensus/mining integration NOT tested\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
