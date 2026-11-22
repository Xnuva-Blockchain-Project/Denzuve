# CP137J-FL — Positive RandomX PoW and validation-boundary audit

Date: 2026-11-22  
Branch: `modernisation/taox-randomx`  
Status: **Research only. RandomX is NOT activated. DO NOT RELEASE.**

## Recorded tests

**FG positive RandomX PoW, isolated synthetic context:** compiled and linked pinned RandomX adapter; existing branch/seed/difficulty checks repeated; a nonce=0 candidate produced the real RandomX hash `4adc82319f28db230d23b87b960e8200b7d94a066aa1154526c03cfab60f7337`, satisfying deliberately easy test `nBits=0x207fffff`. The arithmetic target comparison agreed and independent adapter replay reproduced the hash. The established contextual difficulty policy correctly rejected the altered easy target. No real block was mined or accepted; no network submission.

FG local evidence hashes (artifacts remain outside Git):
- positive_pow.cpp: `67b071f11f2a0208c641abe6fc7fe751b0a4e1c7612c4522b5d36808f0449937`
- positive_pow.o: `f0457a7b4b592df14a406987ed8775bf09ef8be25b63107448ec8a3030eb7bec`
- positive-pow-test: `3649b02b8feb63d84e8ab43280753a6c7e912993fb89bd921eb2b8ea7d7c9ba3`
- runtime.log: `061f0dc48d29a1eb42adf109e947708544d7b443b3b7359dc7e4aac7663ab2f1`

## FH–FK read-only validation-path findings

- `CheckBlockHeader()` in `validation.cpp` presently performs Scrypt PoW through modular dispatcher.
- `AcceptBlockHeader()` invokes `CheckBlockHeader()` before looking up the parent, then `ContextualCheckBlockHeader()` after parent lookup. Admission also requires the anti-DoS `min_pow_checked` condition.
- `CheckBlock()` also invokes `CheckBlockHeader()`. Context-free verification cannot silently become ancestry-dependent.
- Current `CalculateContextualPoWHash()` validates matching parent identity and `BLOCK_VALID_TREE`, but intentionally rejects RandomX selection until consensus seed/activation rules are approved. `BLOCK_VALID_TREE` is not proof of fully validated historical chainstate.
- Network `CheckHeadersPoW()` checks Scrypt prior to chain-start lookup, and header presync uses difficulty-transition limits and `GetBlockProof()` for work estimation. Untrusted-header anti-DoS protections require a separately qualified design.
- `BlockTreeDB::LoadBlockIndexGuts()` checks Scrypt PoW during index loading; `BlockManager::LoadBlockIndex()` later calculates chainwork in height order. A future contextual verifier must handle reconstruction and ancestry-validity ordering safely.
- `BlockManager::ReadBlock()` checks Scrypt on stored block read; restart/reindex must be qualified, not bypassed.
- `CBlockHeader::GetHash()` (block identity) is distinct from `GetPoWHash()`; historical identity must be preserved.
- Compared source copies: `validation.cpp`, `node/blockstorage.cpp`, and `primitives/block.h` match. Development draft is missing `validation.h`, `net_processing.cpp`, `headerssync.cpp`, and `headerssync.h` which exist in isolated full source copy. These must be reconciled in a reproducible build before integration.

## Proposed design contract (NOT implemented)

1. Preserve historical Scrypt consensus and block identifiers, with no premature RandomX activation.
2. Define explicit future activation/epoch seed/retarget policy; only use trusted ancestry and exact expected `nBits` to verify candidate work.
3. Preserve robust early header anti-DoS checks and bound costly RandomX verification; do not omit PoW checking.
4. Validate stored/restarted/reindexed chains with the same effective consensus, and define mixed-algorithm chainwork/reorg treatment before implementation.

## Mandatory release gates

Future work: reproducible full node source/build; exact validation and networking API audit; mixed-PoW safety and work comparisons; actual mined block acceptance on isolated nodes; independent peer validation; wallet/maturity checks where relevant; restart/reindex/reorg tests and sustained-operation testing. No production activation authorized.

**All test results reflect developer-provided local logs and have not been independently reproduced by GitHub CI.**

Reliability first. Release second.
