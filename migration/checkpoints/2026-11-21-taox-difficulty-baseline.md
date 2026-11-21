# TaoX — Historical Difficulty Qualification

Date: 2026-11-21

Status: EXPERIMENTAL / DO NOT RELEASE

## Mainnet consensus parameters

Historical Proof of Work: Scrypt1024_1_1_256

Block identifier: SHA256d

Target spacing: 300 seconds

Target timespan: 600 seconds

Difficulty adjustment interval: 2 blocks

Minimum-difficulty exception: disabled

Retargeting: enabled

powLimit:
00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff

## Checkpoint 47 — Historical target audit

Historical fixture blocks: 637

Height range: 0–636

Recorded difficulty changes: 307

Changes at even heights: 307

Changes at odd heights: 0

First historical nBits: 0x1e0ffff0

Height 636 nBits: 0x1e04d8b0

## Checkpoint 48 — Independent arithmetic audit

Expected nBits calculated independently in Python.

Historical targets checked: 636

Matching targets: 636

Mismatches: 0

Even-height checks: 318

Odd-height checks: 318

The audit reproduces:

- Historical two-block retarget boundaries.
- Initial post-genesis retarget handling.
- Target-timespan adjustment limits.
- Compact difficulty-target encoding.
- powLimit enforcement.
- Intermediate-shift arithmetic.

The historical fixture SHA256 is:

ba52baa8d8158ad63046c85efa7ad5e3d6724e6bb2e7e45d3c12014ca368d666

## Scope of qualification

This audit covers the available historical fixture
through block height 636.

It does not establish full live-network compatibility,
RandomX transition safety or independent-node acceptance.

## RandomX transition — unresolved consensus questions

1. How will the first RandomX target be established?

2. How will post-activation difficulty adjust?

3. How will cumulative work be compared across
   different Proof-of-Work algorithms?

4. How will chain reorganisations crossing the
   activation boundary be handled?

5. How will RandomX seed epochs be derived from
   validated predecessor-chain context?

6. How will P2P prescreening, block import,
   reindex and mining use consistent rules?

## Mandatory safeguards

Preserve historical Scrypt difficulty exactly.

Do not change historical block identifiers.

Do not activate RandomX on mainnet.

Do not invent a production activation height.

Test transition rules independently before
integrating them into block validation.

Reliability first, release second.
