# TaoX RandomX research checkpoint — CP137J-EX (2026-11-22)

**Status: experimental, isolated; DO NOT RELEASE or activate.**

This research record summarises console results observed for CP137J-EO, EP, ET, EV and EW. It is **not** a claim that test source files, binaries, or full local evidence directories were committed to Git. All reported lab paths are on the developer workstation, outside the repository. The development checkout reported branch `modernisation/taox-randomx`, short HEAD `3b510d0`, clean status during the checkpoint scripts.

## Verified runtime milestones

- **CP137J-EO:** Real `CBlockIndex::GetAncestor()` branch-aware ancestry resolver test compiled, linked and executed with a genesis-rooted *synthetic* chain. Parent mismatch and invalid selected seed ancestor were rejected.
- **CP137J-EP:** Extended adversarial seed-ancestry tests passed: seed epoch rollover, invalid parent, absent parent identifier, competing branches. **Trust-boundary finding:** the resolver can accept a manually inconsistent synthetic chain with an invalid *intermediate* ancestor if the parent and selected seed ancestor retain their manually assigned `BLOCK_VALID_TREE` flags. Correct upstream block-index validation remains essential.
- **CP137J-ET:** Existing experimental retarget prototype compiled, linked and executed for illustrative activation heights **1000, 1001, 1016**. Tested first and subsequent retarget windows, fork-specific difficulty, extreme timestamp clamping, malformed compact target and invalid activation ancestor. Parent difficulty history and contextual timestamp validity remain upstream prerequisites.
- **CP137J-EV:** Combined **synthetic** contextual test compiled, linked and executed. Tested branch-specific seed resolution, different fork difficulty, exact `nBits` rejection, high-hash rejection, wrong-parent and invalid-parent rejection, pre-activation seed rejection. **Hash probes were fabricated** (zero/max); no RandomX hash was computed and no blocks were accepted.
- **CP137J-EW:** Read-only integration audit confirmed `tao/crypto/randomx.cpp`, header-based `RandomXHeaderHashV2` adapter and existing *isolated* RandomX static libraries in `cp137jbv-randomx-build`. These are not evidence of successful combined RandomX hashing with contextual target verification.

## Local SHA-256 evidence (user-reported console output)

CP137J-EP: `runtime.log` — `d04ff49e6ee75e22b140acfdfc43d1cf80eefd68a00d4b1847317e241093f4d5`

CP137J-ET: `runtime.log` — `7fe1a4afca4d8992ebbc02daa6084c23f016e57f1c8796a2a920925e2cb003c7`

CP137J-EV: `ev-runtime.log` — `4aad27f48b65bb4d2b8bd17b5adb0929cd616d72b708e82dab9417648525d6c0`

CP137J-EV: `combined-test` — `180519e115c1e49f788f3af29c020d5ee86c88c7b47e93b98df8a5f69ba5c0d0`

The hash values are provenance markers from reported local output, **not independent GitHub CI verification**.

## Safety and unresolved consensus design

- Existing mainnet PoW selector still returns `PoWAlgorithm::Scrypt`. **RandomX disabled**.
- Illustrative activation, starting `nBits`, epoch size and retarget period are **not approved consensus parameters**.
- Existing early header validation / presynchronisation still depends on Scrypt-based checks. This must not reject future valid RandomX headers, nor allow unverified headers to inflate claimed chainwork.
- Mixed-algorithm chainwork security, exact contextual hash integration, full ancestor validity, header propagation, block acceptance, fee/mining economics, restart and reorganisation safety remain unqualified.
- **Next technical milestone:** isolated test of the existing pinned RandomX V2 adapter with branch-derived seed, real header serialization and exact target enforcement. Preserve the production selector and all live nodes.

**Golden standard: Reliability first. Release second.**
