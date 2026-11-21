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


## TaoX shared-engine integration checkpoint — 21 November 2026

**SOURCE-LEVEL ONLY — BUILD AND TESTS NOT EXECUTED.** On branch `modernisation/taox-randomx`, the optional hashing self-test now consumes the separate, shared `Xnuva-RandomX-Engine` at **exact commit `857841056ed939eeea4fd640463b54b6a5cd54af`**. The TaoX adapter delegates hashing/cache management to `xnuva::randomx_foundation::LightHasher` and retains its existing `uint256` byte representation and serialized 80-byte header interface. The previous direct vendored RandomX code in the TaoX adapter has been removed; this does **not** remove the historical Scrypt path.

The original commands above describe the preserved experimental branch and its earlier direct vendored-library dependency. To test **the new TaoX branch**, substitute the following checkout:

```bash
git switch modernisation/taox-randomx
bash migration/materialise-modern-tao.sh /tmp/taox-randomx-source
cmake -S /tmp/taox-randomx-source -B /tmp/taox-randomx-build \
  -DTAO_EXPERIMENTAL_RANDOMX_HASHING=ON \
  -DBUILD_DAEMON=OFF -DBUILD_GUI=OFF -DBUILD_CLI=OFF -DBUILD_TESTS=OFF
cmake --build /tmp/taox-randomx-build --target tao-randomx-selftest -j2
/tmp/taox-randomx-build/bin/tao-randomx-selftest
```

Before executing, verify the materialiser overlays the `migration/modern-tao-draft/src` files into the disposable source tree and check the local build configuration, dependency source SHA and log output. A compilation failure or fixture mismatch is a blocker, not a reason to bypass verification.

**Acceptance checklist (all pending):**
- [ ] Reproduce source materialisation and CMake configuration on the development machine.
- [ ] Confirm the fetched *shared-engine* Git revision exactly matches the pinned SHA; review upstream licence provenance.
- [ ] Compile the shared library, the TaoX adapter and the isolated test executable.
- [ ] Run the unchanged deterministic RandomX V2 reference vectors; independently compare byte-for-byte with the original adapter.
- [ ] Run shared-engine tests, then negative, boundary and cross-platform tests before choosing any consensus activation rules.
- [ ] Independently verify legacy Scrypt replay remains unchanged; check that production daemon, consensus, wallet and miner builds have **no RandomX linkage** unless explicitly approved.
- [ ] Retain checkpoint logs and precise commit/build hashes. No live-node or network deployment is permitted.

**Separate later work:** TaoX/TAOX GUI and executable branding; coin-specific RandomX seed schedule, activation, difficulty/chainwork and P2P compatibility. No mining, wallet, treasury or live system is changed in this checkpoint.

**Reliability first, release second.**
