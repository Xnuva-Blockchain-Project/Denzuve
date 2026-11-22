# TaoX — Authoritative Consolidation Manifest

## 1. Authoritative identity

- Branch: `modernisation/taox-consolidated-full-source`
- Baseline: `cda1211630901586616d046101a3a870f876b303`
- Remote: `https://github.com/Xnuva-Blockchain-Project/Tao.git`
- Bitcoin Core submodule: `9be056a8a72b624dae9623b2f7bded92c2a21c91`
- Consolidated source: `taox-core/`

## 2. Complete repository reconciliation

- Original candidate: 2,937 files
- Consolidated candidate: 2,937 files
- Exact candidate differences: ZERO
- Upstream source files (excluding Git metadata): 2,923
- TaoX source files: 2,937
- Common files: 2,923
- Modified upstream files: 21
- Added TaoX files: 14
- Removed upstream source files: ZERO

The upstream `.git` metadata entry is excluded from source accounting.

### Modified files

- `README.md`
- `src/CMakeLists.txt`
- `src/chainparams.cpp`
- `src/chainparams.h`
- `src/chainparamsbase.cpp`
- `src/chainparamsbase.h`
- `src/consensus/amount.h`
- `src/consensus/consensus.h`
- `src/consensus/params.h`
- `src/init.cpp`
- `src/kernel/chainparams.cpp`
- `src/kernel/chainparams.h`
- `src/key_io.cpp`
- `src/key_io.h`
- `src/node/blockstorage.cpp`
- `src/node/miner.cpp`
- `src/pow.cpp`
- `src/pow.h`
- `src/primitives/block.cpp`
- `src/primitives/block.h`
- `src/validation.cpp`

### Added files

- `src/chain_identity_security.h`
- `src/tao/consensus/pow_algorithm.h`
- `src/tao/consensus/pow_bridge_experimental.h`
- `src/tao/consensus/pow_contextual_hash.h`
- `src/tao/consensus/pow_hash_dispatch.h`
- `src/tao/consensus/pow_seed_resolver_experimental.h`
- `src/tao/consensus/pow_transition_policy_experimental.h`
- `src/tao/crypto/randomx.cpp`
- `src/tao/crypto/randomx.h`
- `src/tao/crypto/randomx_selftest.cpp`
- `src/tao/crypto/scrypt.cpp`
- `src/tao/crypto/scrypt.h`
- `src/tao/subsidy.cpp`
- `src/tao/subsidy.h`

## 3. Source subtree reconciliation

- Upstream src files: 2,003
- TaoX src files: 2,017
- Modified: 20
- Added: 14
- Removed: ZERO

The additional modified root-level file is `README.md`.

## 4. RandomX and PoW component SHA-256

- `6374ffe8b89fd5785f7cf0fe68e8156de8c7a807bd2b120bd0b53dc9863af159` — `src/tao/consensus/pow_algorithm.h`
- `9d261d20fe725b8dc2d45f51b0142ff1c7400a0e29f6ae0dfb1f10992d03d8cd` — `src/tao/consensus/pow_bridge_experimental.h`
- `6adaeecf9b7fa44648e24952e1e5d9a06adb625b54d1d4030c637b7f58740012` — `src/tao/consensus/pow_contextual_hash.h`
- `2a445d40f2c27731859772728da0a73eceb45636b4006dc83814a1a92c71026d` — `src/tao/consensus/pow_hash_dispatch.h`
- `1f24ad5996b0366095113a5ce2ffa70e0b4b33bdc3d986010268bfc9d0a90d16` — `src/tao/consensus/pow_seed_resolver_experimental.h`
- `7bf7d8c3c531693d5f372950cb960c9ec90be435c585c567ca7abb7c68ad57f3` — `src/tao/consensus/pow_transition_policy_experimental.h`
- `d228c1812f49e46bf8e11cbd50a40149e05932bc4118aff6cfe13084f0ed3cec` — `src/tao/crypto/randomx.cpp`
- `890a4d66ebf45ec2f0df53d321b2fde0b75c2144e2a084c3c42310831a6081e0` — `src/tao/crypto/randomx.h`
- `2a62cdcd47aa7a4f78f706109b8e75d0fc91fde4bfe243154d946c6ccd1a6c24` — `src/tao/crypto/randomx_selftest.cpp`
- `882256d53ed9d85420e4c707fce3a66c0fe03d2b540d80647ed56ae3a19fc3e5` — `src/test/pow_tests.cpp`

## 5. Previous engineering evidence

- CP137J-FU: complete source candidate and original change manifest.
- CP137J-GJ: RandomX build and linkage investigations.
- CP137J-GL: synthetic seed-context tests passed; no cryptographic validation.
- Operations 1–4B: source consolidation and reconciliation.

Historical test results are not production release qualification.

## 6. Outstanding RandomX work

- Production consensus transition.
- Network-specific activation configuration.
- Production ancestry-based seed selection.
- RandomX block validation.
- RandomX mining integration.
- Difficulty and retarget transition qualification.
- Complete daemon build and regression testing.
- Independent network and sustained-operation qualification.

## 7. Preservation policy

- Preserve the original candidate and pinned upstream foundation.
- No live-network changes during consolidation.
- No premature RandomX activation.
- No release until all published migration requirements are satisfied.

## 8. Consolidation status

Full source materialised and independently compared.
Manifest corrected using both repository and src subtree scopes.
Awaiting final review, Git commit and remote verification.
