// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/data/key_io_invalid.json.h>
#include <test/data/key_io_valid.json.h>

#include <key.h>
#include <key_io.h>
#include <script/script.h>
#include <test/util/json.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <stdexcept>

BOOST_FIXTURE_TEST_SUITE(key_io_tests, BasicTestingSetup)

// Goal: check that parsed keys match test payload
BOOST_AUTO_TEST_CASE(key_io_valid_parse)
{
    UniValue tests = read_json(json_tests::key_io_valid);
    CKey privkey;
    CTxDestination destination;
    SelectParams(ChainType::MAIN);

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        const UniValue& test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 3) { // Allow for extra stuff (useful for comments)
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::string exp_base58string = test[0].get_str();
        const std::vector<std::byte> exp_payload{ParseHex<std::byte>(test[1].get_str())};
        const UniValue &metadata = test[2].get_obj();
        bool isPrivkey = metadata.find_value("isPrivkey").get_bool();
        const auto chain = ChainTypeFromString(
            metadata.find_value("chain").get_str());
        BOOST_REQUIRE(chain.has_value());

        // Preserve inherited unsupported-network vectors.
        // Their network selection must fail explicitly.
        if (*chain != ChainType::MAIN &&
            *chain != ChainType::REGTEST) {
            BOOST_CHECK_THROW(
                SelectParams(*chain),
                std::runtime_error);
            SelectParams(ChainType::MAIN);
            continue;
        }

        SelectParams(*chain);
        bool try_case_flip = metadata.find_value("tryCaseFlip").isNull() ? false : metadata.find_value("tryCaseFlip").get_bool();
        // Denzuve deliberately retains Base58-only addresses.
        // Inherited witness-address vectors must be rejected.
        if (try_case_flip) {
            BOOST_REQUIRE(!isPrivkey);

            BOOST_CHECK_MESSAGE(
                !IsValidDestination(
                    DecodeDestination(exp_base58string)),
                "Witness address unexpectedly accepted:" + strTest);

            BOOST_CHECK_MESSAGE(
                !DecodeSecret(exp_base58string).IsValid(),
                "Witness address accepted as private key:" + strTest);

            for (char& c : exp_base58string) {
                if (c >= 'a' && c <= 'z') {
                    c = (c - 'a') + 'A';
                } else if (c >= 'A' && c <= 'Z') {
                    c = (c - 'A') + 'a';
                }
            }

            BOOST_CHECK_MESSAGE(
                !IsValidDestination(
                    DecodeDestination(exp_base58string)),
                "Case-flipped witness address accepted:" + strTest);

            continue;
        }

        if (isPrivkey) {
            bool isCompressed = metadata.find_value("isCompressed").get_bool();
            // Must be valid private key
            privkey = DecodeSecret(exp_base58string);
            BOOST_CHECK_MESSAGE(privkey.IsValid(), "!IsValid:" + strTest);
            BOOST_CHECK_MESSAGE(privkey.IsCompressed() == isCompressed, "compressed mismatch:" + strTest);
            BOOST_CHECK_MESSAGE(std::ranges::equal(privkey, exp_payload), "key mismatch:" + strTest);

            // Private key must be invalid public key
            destination = DecodeDestination(exp_base58string);
            BOOST_CHECK_MESSAGE(!IsValidDestination(destination), "IsValid privkey as pubkey:" + strTest);
        } else {
            // Must be valid public key
            destination = DecodeDestination(exp_base58string);
            CScript script = GetScriptForDestination(destination);
            BOOST_CHECK_MESSAGE(IsValidDestination(destination), "!IsValid:" + strTest);
            BOOST_CHECK_EQUAL(HexStr(script), HexStr(exp_payload));

            // Try flipped case version
            for (char& c : exp_base58string) {
                if (c >= 'a' && c <= 'z') {
                    c = (c - 'a') + 'A';
                } else if (c >= 'A' && c <= 'Z') {
                    c = (c - 'A') + 'a';
                }
            }
            destination = DecodeDestination(exp_base58string);
            BOOST_CHECK_MESSAGE(IsValidDestination(destination) == try_case_flip, "!IsValid case flipped:" + strTest);
            if (IsValidDestination(destination)) {
                script = GetScriptForDestination(destination);
                BOOST_CHECK_EQUAL(HexStr(script), HexStr(exp_payload));
            }

            // Public key must be invalid private key
            privkey = DecodeSecret(exp_base58string);
            BOOST_CHECK_MESSAGE(!privkey.IsValid(), "IsValid pubkey as privkey:" + strTest);
        }
    }
    SelectParams(ChainType::MAIN);
}

// Goal: check that generated keys match test vectors
BOOST_AUTO_TEST_CASE(key_io_valid_gen)
{
    UniValue tests = read_json(json_tests::key_io_valid);

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        const UniValue& test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 3) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::string exp_base58string = test[0].get_str();
        std::vector<unsigned char> exp_payload = ParseHex(test[1].get_str());
        const UniValue &metadata = test[2].get_obj();
        bool isPrivkey = metadata.find_value("isPrivkey").get_bool();
        const auto chain = ChainTypeFromString(
            metadata.find_value("chain").get_str());
        BOOST_REQUIRE(chain.has_value());

        // Preserve inherited unsupported-network vectors.
        // Their network selection must fail explicitly.
        if (*chain != ChainType::MAIN &&
            *chain != ChainType::REGTEST) {
            BOOST_CHECK_THROW(
                SelectParams(*chain),
                std::runtime_error);
            SelectParams(ChainType::MAIN);
            continue;
        }

        SelectParams(*chain);
        const bool witness_address =
            !metadata.find_value("tryCaseFlip").isNull() &&
            metadata.find_value("tryCaseFlip").get_bool();

        if (witness_address) {
            BOOST_REQUIRE(!isPrivkey);

            const CScript script(
                exp_payload.begin(),
                exp_payload.end());

            CTxDestination dest;

            // Script recognition is distinct from permission
            // to create a witness address.
            BOOST_REQUIRE_MESSAGE(
                ExtractDestination(script, dest),
                "Witness script extraction failed:" + strTest);

            BOOST_CHECK_MESSAGE(
                EncodeDestination(dest).empty(),
                "Witness address unexpectedly generated:" + strTest);

            BOOST_CHECK_MESSAGE(
                GetScriptForDestination(dest) == script,
                "Witness script round trip changed:" + strTest);

            continue;
        }

        if (isPrivkey) {
            bool isCompressed = metadata.find_value("isCompressed").get_bool();
            CKey key;
            key.Set(exp_payload.begin(), exp_payload.end(), isCompressed);
            assert(key.IsValid());
            BOOST_CHECK_MESSAGE(EncodeSecret(key) == exp_base58string, "result mismatch: " + strTest);
        } else {
            CTxDestination dest;
            CScript exp_script(exp_payload.begin(), exp_payload.end());
            BOOST_CHECK(ExtractDestination(exp_script, dest));
            std::string address = EncodeDestination(dest);

            BOOST_CHECK_EQUAL(address, exp_base58string);
        }
    }

    SelectParams(ChainType::MAIN);
}


// Goal: check that base58 parsing code is robust against a variety of corrupted data
BOOST_AUTO_TEST_CASE(key_io_invalid)
{
    UniValue tests = read_json(json_tests::key_io_invalid); // Negative testcases
    CKey privkey;
    CTxDestination destination;

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        const UniValue& test = tests[idx];
        std::string strTest = test.write();
        if (test.size() < 1) // Allow for extra stuff (useful for comments)
        {
            BOOST_ERROR("Bad test: " << strTest);
            continue;
        }
        std::string exp_base58string = test[0].get_str();

        // must be invalid as public and as private key
        for (const auto& chain : {ChainType::MAIN, ChainType::REGTEST}) {
            SelectParams(chain);
            destination = DecodeDestination(exp_base58string);
            BOOST_CHECK_MESSAGE(!IsValidDestination(destination), "IsValid pubkey in mainnet:" + strTest);
            privkey = DecodeSecret(exp_base58string);
            BOOST_CHECK_MESSAGE(!privkey.IsValid(), "IsValid privkey in mainnet:" + strTest);
        }
    }
    SelectParams(ChainType::MAIN);
}

BOOST_AUTO_TEST_CASE(denzuve_foreign_base58_rejection)
{
    for (const auto chain : {ChainType::MAIN, ChainType::REGTEST}) {
        SelectParams(chain);
        BOOST_CHECK(!IsValidDestination(DecodeDestination("1FsSia9rv4NeEwvJ2GvXrX7LyxYspbN2mo")));
        BOOST_CHECK(!DecodeSecret("5JuW2AMDYu4xVwRG9DZW18VbzQrGcd5RCgb99sS6ehJsNQXu5b9").IsValid());
        BOOST_CHECK(!DecodeSecret("L5nJeqKmpHp4P7F8ZYyjwc5a7P4d8EabuGAzfGJk7yC1BJyzNaEd").IsValid());
        BOOST_CHECK(!IsValidDestination(DecodeDestination("1FjL87pn8ky6Vbavd1ZHeChRXtoxwRGCRd")));
        BOOST_CHECK(!DecodeSecret("5HsL2nZuEebU5nM3RxNVQD9GcAnvNMahqQskf4fkqHe54zwd14e").IsValid());
        BOOST_CHECK(!DecodeSecret("KwuVvu6hsuEMHrfFWJQV64tRrWX3QzqHH18JuAHYqYV6dqBvNKxd").IsValid());
        BOOST_CHECK(!IsValidDestination(DecodeDestination("1G9A9j6W8TLuh6dEeVwWeyibK1Uc5MfVFV")));
    }
    SelectParams(ChainType::MAIN);
}

BOOST_AUTO_TEST_CASE(denzuve_native_witness_rejection)
{
    // Independently generated valid witness addresses.
    // Denzuve deliberately supports Base58 destinations only.
    SelectParams(ChainType::MAIN);
    // P2WPKH (Bech32)
    BOOST_CHECK(!IsValidDestination(DecodeDestination("tao-disabled1qqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5nfzw6z")));
    BOOST_CHECK(!IsValidDestination(DecodeDestination("TAO-DISABLED1QQYPQXPQ9QCRSSZG2PVXQ6RS0ZQG3YYC5NFZW6Z")));
    BOOST_CHECK(!DecodeSecret("tao-disabled1qqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5nfzw6z").IsValid());
    // P2WSH (Bech32)
    BOOST_CHECK(!IsValidDestination(DecodeDestination("tao-disabled1qqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqz386fs")));
    BOOST_CHECK(!IsValidDestination(DecodeDestination("TAO-DISABLED1QQYPQXPQ9QCRSSZG2PVXQ6RS0ZQG3YYC5Z5TPWXQERGD3C8G7RUSQZ386FS")));
    BOOST_CHECK(!DecodeSecret("tao-disabled1qqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqz386fs").IsValid());
    // P2TR (Bech32m)
    BOOST_CHECK(!IsValidDestination(DecodeDestination("tao-disabled1pqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqgx8n3v")));
    BOOST_CHECK(!IsValidDestination(DecodeDestination("TAO-DISABLED1PQYPQXPQ9QCRSSZG2PVXQ6RS0ZQG3YYC5Z5TPWXQERGD3C8G7RUSQGX8N3V")));
    BOOST_CHECK(!DecodeSecret("tao-disabled1pqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqgx8n3v").IsValid());
    // UNKNOWN_WITNESS (Bech32m)
    BOOST_CHECK(!IsValidDestination(DecodeDestination("tao-disabled1zqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqqm7ul8")));
    BOOST_CHECK(!IsValidDestination(DecodeDestination("TAO-DISABLED1ZQYPQXPQ9QCRSSZG2PVXQ6RS0ZQG3YYC5Z5TPWXQERGD3C8G7RUSQQM7UL8")));
    BOOST_CHECK(!DecodeSecret("tao-disabled1zqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqqm7ul8").IsValid());
    SelectParams(ChainType::REGTEST);
    // P2WPKH (Bech32)
    BOOST_CHECK(!IsValidDestination(DecodeDestination("taoxrt1qqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5uktta6")));
    BOOST_CHECK(!IsValidDestination(DecodeDestination("TAOXRT1QQYPQXPQ9QCRSSZG2PVXQ6RS0ZQG3YYC5UKTTA6")));
    BOOST_CHECK(!DecodeSecret("taoxrt1qqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5uktta6").IsValid());
    // P2WSH (Bech32)
    BOOST_CHECK(!IsValidDestination(DecodeDestination("taoxrt1qqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqtha3fn")));
    BOOST_CHECK(!IsValidDestination(DecodeDestination("TAOXRT1QQYPQXPQ9QCRSSZG2PVXQ6RS0ZQG3YYC5Z5TPWXQERGD3C8G7RUSQTHA3FN")));
    BOOST_CHECK(!DecodeSecret("taoxrt1qqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqtha3fn").IsValid());
    // P2TR (Bech32m)
    BOOST_CHECK(!IsValidDestination(DecodeDestination("taoxrt1pqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqpqac30")));
    BOOST_CHECK(!IsValidDestination(DecodeDestination("TAOXRT1PQYPQXPQ9QCRSSZG2PVXQ6RS0ZQG3YYC5Z5TPWXQERGD3C8G7RUSQPQAC30")));
    BOOST_CHECK(!DecodeSecret("taoxrt1pqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqpqac30").IsValid());
    // UNKNOWN_WITNESS (Bech32m)
    BOOST_CHECK(!IsValidDestination(DecodeDestination("taoxrt1zqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqfayhly")));
    BOOST_CHECK(!IsValidDestination(DecodeDestination("TAOXRT1ZQYPQXPQ9QCRSSZG2PVXQ6RS0ZQG3YYC5Z5TPWXQERGD3C8G7RUSQFAYHLY")));
    BOOST_CHECK(!DecodeSecret("taoxrt1zqypqxpq9qcrsszg2pvxq6rs0zqg3yyc5z5tpwxqergd3c8g7rusqfayhly").IsValid());
    SelectParams(ChainType::MAIN);
}

BOOST_AUTO_TEST_SUITE_END()
