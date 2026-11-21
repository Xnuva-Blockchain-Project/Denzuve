# TaoX — Historical Consensus Baseline

Date: 2026-11-21

Status: EXPERIMENTAL / DO NOT RELEASE

Branch: modernisation/taox-randomx

## Verified checkpoint results

- CP7: TaoX RandomX v2 reference vectors passed.
- CP8: Shared RandomX foundation CTest passed (1/1).
- CP17: Five historical Scrypt vectors passed in Python.
- CP18A: Five historical Scrypt vectors passed in modern C++.
- CP19: Historical headers 0–636 independently verified.
- CP22: Experimental daemon replayed historical blocks 0–636.
- CP23: Restart recovered height 636 and matching chain tip.
- CP24: Three independent negative-header tests passed.
- CP26: Modern daemon rejected an invalid Scrypt PoW header.

## Historical chain identity

Genesis:
41de2a4b7018115eae4a3e7e82189b551aaf871af66929fa28a49ee852abb67e

Height 636:
7e235df054e299f729ef32e136fe0acc198aa792422fd24bc0309ecbc4847c88

Historical fixture SHA256:
ba52baa8d8158ad63046c85efa7ad5e3d6724e6bb2e7e45d3c12014ca368d666

## Experimental daemon

SHA256:
0f0f49bfca29561188e911b00eec06d6ffe22ad482c6cb6a3c868a04f8f56502

RandomX consensus integration: NOT ENABLED.

## Negative consensus test

Invalid fixture SHA256:
6c8ba7c992615aee87c056ca6ec68d9d59795c4d686a84a80e0e198e3f3dcd9d

Observed rejection:
high-hash, proof of work failed

Invalid block was not accepted.
Genesis remained active.
Daemon exited cleanly.

## Next engineering stage

Introduce a separately tested, context-aware
Proof-of-Work algorithm selector.

Initial selector behaviour must remain Scrypt-only.

Future RandomX activation, seed derivation,
difficulty transition, chainwork treatment,
P2P validation and reorganisation rules
require explicit design and testing.

Do not activate RandomX on an existing network.

## Remaining release gates

- Full historical and live-chain compatibility.
- Contextual difficulty and chainwork qualification.
- RandomX activation-boundary tests.
- Independent network consensus acceptance.
- Mining and wallet reward verification.
- Restart, reindex and reorganisation tests.
- Sustained operation.

Reliability first, release second.
