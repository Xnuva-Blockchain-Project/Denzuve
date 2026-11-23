// TaoX isolated wrong-seed RandomX regression fixture.
// No node execution, RPC, mining or chain modifications.

#include <arith_uint256.h>
#include <primitives/block.h>
#include <tao/crypto/randomx.h>
#include <uint256.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <span>

int main()
{
    try {
        CBlockHeader header{};
        header.nVersion = 536870912;
        header.hashPrevBlock = uint256{
            "3ef8b3bb2418102fd3143d064cf6ad0ebd46d3c521bfb14ffb08b287bcead0ad"};
        header.hashMerkleRoot = uint256{
            "6a44a13738b08b37b3f8418e573cb65bbad68af85ecdf9142ee57534fb5123d2"};
        header.nTime = 1795434684;
        header.nBits = 0x207fffff;
        header.nNonce = 1;

        const uint256 expected_block{
            "194b97ca6382447b00dc5cbee88f36675adcfdc5132bdb0b80c4c50e6f1b558e"};

        if (header.GetHash() != expected_block) {
            std::cerr << "FAIL: Block 34 header fixture mismatch\n";
            return 1;
        }

        const uint256 correct_seed = header.hashPrevBlock;

        const uint256 wrong_seed{
            "abe1547eb376d48c9f0bf14308b7656d188248629c6b84ac148b97983d4c6e4a"};

        const std::span<const unsigned char> correct_bytes{
            correct_seed.data(), correct_seed.size()};
        const std::span<const unsigned char> wrong_bytes{
            wrong_seed.data(), wrong_seed.size()};

        tao::experimental::RandomXLightHasher correct_hasher{
            std::as_bytes(correct_bytes)};
        tao::experimental::RandomXLightHasher wrong_hasher{
            std::as_bytes(wrong_bytes)};

        arith_uint256 target;
        bool negative = false;
        bool overflow = false;
        target.SetCompact(header.nBits, &negative, &overflow);

        if (negative || overflow || target == 0) {
            std::cerr << "FAIL: Invalid target\n";
            return 1;
        }

        for (uint32_t nonce = 2; nonce < 256; ++nonce) {
            if (nonce == 1) continue;

            header.nNonce = nonce;

            const uint256 wrong_hash =
                wrong_hasher.HashHeader(header);

            if (UintToArith256(wrong_hash) > target) {
                continue;
            }

            const uint256 correct_hash =
                correct_hasher.HashHeader(header);

            if (UintToArith256(correct_hash) <= target) {
                continue;
            }

            std::cout << "PASS: Wrong-seed candidate found\n";
            std::cout << "NONCE=" << nonce << '\n';
            std::cout << "CANDIDATE_BLOCK_ID="
                      << header.GetHash().GetHex() << '\n';
            std::cout << "WRONG_SEED_HASH="
                      << wrong_hash.GetHex() << '\n';
            std::cout << "CORRECT_SEED_HASH="
                      << correct_hash.GetHex() << '\n';
            std::cout << "WRONG_SEED_TARGET=PASS\n";
            std::cout << "CORRECT_SEED_TARGET=FAIL\n";
            return 0;
        }

        std::cerr << "NOT QUALIFIED: No distinguishing nonce found\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
