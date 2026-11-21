# TAO Revival — RandomX forward-only hard-fork design (DRAFT)

**Created:** 2026-11-21
**Branch:** `modernisation/randomx-hardfork-experimental-20261121`
**Status:** DESIGN ONLY; UNIMPLEMENTED; UNTESTED; MAINNET DISABLED; DO NOT RELEASE.

## Baseline evidence from the preserved modernisation branch

This child branch was created from `modernisation/bitcoin-core-31.1-experimental-do-not-release`. It must not change the parent branch, the original `tao-revival` authority, mainnet or installed binaries. The mechanical Bitcoin Core 31.1 port itself remains unfinished and unreleased.

- Source authority: `tao-revival` @ `74216f33f634f5dde37e96805c72c63c181c52df`.
- Historical block IDs: serialized double SHA-256 (80-byte header).
- Historical PoW: Scrypt `1024_1_1_256` over that header; `CBlockHeader::GetPoWHash()` is currently height-agnostic and implements Scrypt.
- Existing target spacing 300 seconds; target timespan 600 seconds; retarget interval two blocks, with Revival's exact original rules.
- Existing genesis hash `41de2a4b7018115eae4a3e7e82189b551aaf871af66929fa28a49ee852abb67e`; checkpoint height 1 `717fc5ef97ded9f7d514fcb1ed558bc76596ce85f09890efa6e668d228c4a57f`.
- Original supply, subsidy, maturity, scripts, Base58 identities, ports and peer network remain unchanged.
- Historical preservation contract: `migration/TAO-REVIVAL-CONSENSUS-PRESERVATION.md`.

## Proposed transition invariant

Let `H` be a *currently unset* mainnet hard-fork height. For each candidate block at verified chain height `h`:

- If `h < H`, use exact historic Scrypt PoW and difficulty consensus. All old blocks and their 256-bit IDs must continue to match the legacy node bit-for-bit.
- If `h >= H`, use an independently specified RandomX hash to check a target determined by **post-fork** consensus difficulty logic; block identifiers remain SHA256d unless a later separately justified specification explicitly says otherwise.
- No PoW validation bypass, time-based ambiguity, retroactive application, or fallback to Scrypt after activation.
- All validation paths must choose using contextual candidate height; the current height-less `GetPoWHash()` cannot alone perform the switch safely. Audit `src/primitives/block.cpp`, `src/pow.cpp`, `src/validation.cpp`, `src/node/miner.cpp`, P2P/header acceptance, block storage, compact-target / chainwork APIs and all miner RPCs; verify sources after materialisation.

**This is the design boundary, not a ready implementation or an activation commitment.**

## Consensus questions — resolve *before* writing activation logic

1. **RandomX input:** canonical serialized 80-byte TAO block header, byte ordering, and exact PoW result representation. Obtain independent cross-implementation vectors.
2. **RandomX key/seed:** deterministic, consensus-pinned epoch/block ancestry scheme independent of any unavailable future block or noncanonical timestamp. Specify seed-block age, interval, reorg and deep-history validation behavior; do not copy Monero's 2048/64 schedule blindly for a five-minute chain.
3. **Dependency pinning:** exact reviewed RandomX upstream release/source commit and build flags; reproducible static/dynamic linkage and cross-platform verification including light-mode validators.
4. **Transition difficulty:** existing Scrypt compact targets/relative work do not map automatically to RandomX hash performance. Specify initial post-fork target, effective work comparison, subsequent adjustment, all edge cases, hash-rate shock behavior, timestamp manipulation bounds and anti-oscillation behavior; retain deterministic `nBits` validation.
5. **Activation and block versions:** explicit future height `H` based on observed live tip, release readiness and miners/nodes; reject incompatible candidate blocks; decide whether version signaling is necessary. Do not set `H` while the live chain and peer/operator readiness are unverified.
6. **Mining protocol:** `getblocktemplate`/external mining RPC selection, RandomX seed advertisement, block serialization, nonce/search loop, submission, coinbase address and acceptance by an *independent* node. Verify mining payout and game treasury after reward maturity.
7. **Security:** measure viable independent CPU hashrate across all five potential RandomX chains; analyse cross-chain hashrate switching/rental and 51%/reorg risk. CPU-friendly is not CPU-exclusive or inherently attack-proof.
8. **Network upgrade:** miners, explorers, nodes, wallets, seeds and game/treasury must coordinate. Legacy clients can reject new blocks. Rollback of binaries is not safe as an automatic rollback of an activated chain fork.
9. **Test network:** Revival testnet genesis is unresolved; design a separately identified, isolated test fixture with distinct ports/datadir/keys and mainnet mining fail-closed, without representing invented genesis as canonical historical testnet.

## Stage-by-stage implementation checklist

- [x] Create separate RandomX hard-fork experimental branch from current modernisation work.
- [x] Capture existing consensus contracts, port status and specific implementation touchpoints in this design document.
- [ ] Record baseline branch commit, chain tip hashes and node/miner status; verified offline recovery assets. No live restarts.
- [ ] Extract and independently verify historic Scrypt and SHA256d vectors for heights 0, 1, several historical retargets and at least a known current checkpoint.
- [ ] Audit XNUVA RandomX implementation for portable libraries, seed strategy, security limitations, startup, wallet and real-network evidence.
- [ ] Pin upstream RandomX, add standalone isolated hashing harness/tests; **no mainnet consensus activation** and no mining-service changes.
- [ ] Specify `H`, post-fork difficulty and chainwork only in isolated tests first, using test-only activation settings; include boundary `H-2,H-1,H,H+1` and negative tests.
- [ ] Integrate contextual dual-PoW validation and mining in the TAO draft only after passing independent hash vectors and migration review.
- [ ] Build test-only binaries; prove historic full-chain replay and state-equivalence against Revival.
- [ ] Mine/validate post-transition blocks across independently configured nodes; reorg across boundary; test wallet rewards, spending, restart and rescan, failures and no-history-rewrite.
- [ ] Check nodes/miners/operator readiness and publish a separate proposed live activation only with explicit approval after Golden Standard gates pass.

## Release blockers (currently OPEN)

- TAO Bitcoin Core 31.1 port itself is not release-qualified.
- No RandomX TAO code, matching fixtures, tests or live consensus activation exist.
- Neither testnet/regtest consensus nor new-network chain identity has been qualified.
- No confirmed miner/operator upgrade consensus or safety analysis.

**Decision record:** RandomX is the intended long-term CPU-friendly PoW direction for TAO, ERA, ZUS and HOPE; XNUVA remains on RandomX. Actual hard-fork activation is a separately gated operational event, not an effect of creating a GitHub branch or RFC.

## References

- `migration/PORT-STATUS.md`
- `migration/TAO-REVIVAL-CONSENSUS-PRESERVATION.md`
- `MODERNISATION-NOTICE.md`
- https://github.com/tevador/RandomX
- https://github.com/Xnuva-Blockchain-Project/Xnuva/blob/docs/golden-standard-modernisation-handbook-v1/doc/engineering-standards/GOLDEN_STANDARD_CRYPTO_MODERNISATION.md
