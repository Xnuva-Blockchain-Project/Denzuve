# TAO RandomX v2 — first code integration (2026-11-21)

**STATUS: SOURCE ADDED; NOT COMPILED OR EXECUTED HERE; NOT CONSENSUS-ACTIVE; DO NOT RELEASE.**

This experimental child branch of the original TAO Bitcoin Core 31.1 modernisation branch now contains a real C++ RandomX v2 *hashing adapter*, with an optional build/test target. It deliberately does not switch consensus, alter difficulty, select mainnet activation height, modify mining RPCs or connect to live peers.

## Source and build provenance

- RandomX dependency: vendored RandomX under `Xnuva-Blockchain-Project/Xnuva/src/randomx`, pinned via CMake FetchContent to XNUVA commit `853a3171ece198f60b5125a16dca83d68a8f53c3`.
- TAO adapter: `migration/modern-tao-draft/src/tao/crypto/randomx.{h,cpp}`.
- Deterministic test: `migration/modern-tao-draft/src/tao/crypto/randomx_selftest.cpp`.
- CMake option: `TAO_EXPERIMENTAL_RANDOMX_HASHING` **OFF by default**.
- Uses light-mode V2 VM + mutex-protected reusable cache; accepts an explicit seed key and canonical 80-byte block header. It is not yet an optimised mining implementation.
- Builds an independent `tao-randomx-selftest` executable only when enabled. The node and original `bitcoin_consensus` PoW path do not link the new adapter.
- Original `CBlockHeader::GetHash()` (SHA256d) and `GetPoWHash()` (Scrypt), historic chain replay and existing miner/block-storage patches remain unchanged.

## How to build and verify on an isolated development machine

Use the project migration materialiser in a disposable working directory, never a live datadir:

```bash
git switch modernisation/randomx-hardfork-experimental-20261121
bash migration/materialise-modern-tao.sh /tmp/tao-randomx-source
cmake -S /tmp/tao-randomx-source -B /tmp/tao-randomx-build \
  -DTAO_EXPERIMENTAL_RANDOMX_HASHING=ON \
  -DBUILD_DAEMON=OFF -DBUILD_GUI=OFF -DBUILD_CLI=OFF -DBUILD_TESTS=OFF
cmake --build /tmp/tao-randomx-build --target tao-randomx-selftest -j2
/tmp/tao-randomx-build/bin/tao-randomx-selftest
```

`FetchContent` downloads the exact pinned XNUVA revision and may require build-network access. Confirm dependencies, build paths and binary location for the local environment. **No verification is claimed until the actual test log is captured.**

Expected tests: pinned XNUVA RandomX V2 string vector, canonical 80-byte header / SHA256d block ID, pinned XNUVA RandomX V2 header digest. The fixture is about hashing compatibility, *not* agreement on TAO's seed schedule.

## Explicitly blocked pending design and validation

1. Choose and independently test TAO-specific RandomX seed/key schedule, consensus variant and activation parameters.
2. Specify distinct post-fork PoW difficulty/chainwork transitions, miner API and negative/activation-boundary tests.
3. Implement *contextual* `height < H` Scrypt and `height >= H` RandomX selection across header, block-storage, reindex, mining and peer validation paths; do NOT rewrite the height-agnostic `GetPoWHash()` as RandomX.
4. Verify historic TAO chain and wallet equivalence against the Revival authoritative implementation, then mine post-fork blocks on isolated independent nodes.
5. Complete GSH release gates, operator/miner coordination and separate approval for any live activation. An enabled CMake test option does NOT authorise a live hard fork.

**Reliability first, release second.**
