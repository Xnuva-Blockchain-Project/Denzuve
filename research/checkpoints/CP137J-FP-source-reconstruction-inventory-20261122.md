# CP137J-FP — TaoX complete-source reconstruction inventory

**Date:** 2026-11-22  
**Branch:** `modernisation/taox-randomx`  
**Status:** Research only — source reconstruction NOT yet performed. RandomX remains disabled. DO NOT RELEASE.

## CP137J-FN and FO findings (developer-reported, read-only)

Development draft: `migration/modern-tao-draft/src` inside the Tao Git repository.  
Isolated full laboratory source: `~/taox-build-lab/cp137jbi-context/source/src`.  
Existing laboratory build: `~/taox-build-lab/cp137jbl-build`, with `CMAKE_HOME_DIRECTORY` pointing to the isolated laboratory source; it is not a Git checkout.

FO reported:
- 34 Git-tracked files under the development draft.
- 31 of those are byte-for-byte identical to copies in the laboratory.
- 0 conflicting files among the files present in both locations.
- 3 tracked development files missing from the laboratory source:
  - `tao/consensus/pow_bridge_experimental.h`
  - `tao/consensus/pow_seed_resolver_experimental.h`
  - `tao/consensus/pow_transition_policy_experimental.h`
- 2,014 regular laboratory source files, of which 1,983 are not tracked in the development draft.
- Examples of untracked-in-draft laboratory files: `validation.h`, `net_processing.cpp`, `headerssync.cpp`, `headerssync.h`, `chain.cpp`, `chain.h`. The full inventory has **not** been recorded in this document, only the aggregate and examples.
- Verified identical SHA-256 source pairs include `CMakeLists.txt`, `validation.cpp`, `node/blockstorage.cpp`, `pow.cpp`, `pow.h`, `primitives/block.h`.
- The full laboratory source contains upstream Bitcoin Core copyright/version material and experimental Tao migration customisations; its exact upstream commit provenance has **not yet** been established.
- An authoritative Tao Revival regtest is explicitly not defined; some non-mainnet network definitions remain unresolved.

## Important distinction

Local Git and GitHub are already synchronised. The missing material is in the **incomplete tracked source snapshot**, not in the Git transport. Neither the 1,983 files nor the three draft-only headers have been imported, replaced or deleted by this documentation checkpoint.

## Safe source reconstruction gates

1. Identify and verify the full laboratory source's upstream provenance and exact pinned base revision, including any existing migration overlays and licensing notices. Do not infer provenance from file contents alone.
2. Produce a complete manifest (paths, SHA-256, file types, symlinks and sizes) and compare against the Git-tracked draft; distinguish source files from generated outputs and third-party vendoring.
3. Assemble a **new isolated candidate source directory**, combining a verified full baseline and the 34 tracked experimental files without overwriting the three draft-only headers. Never modify live nodes or the existing qualified laboratory source during assembly.
4. Review diffs, build configuration and dependency provenance. Apply a dedicated ignore/exclusion policy; no blind import of all laboratory files.
5. Build and run historical Scrypt regression vectors, pinned RandomX self-tests, relevant contextual PoW tests and startup checks in the isolated candidate.
6. Commit the reconciled, qualified source only on the experimental branch, in traceable increments. No merges to main, mainnet activation, releases or production deployment without full independent validation.

**Golden standard:** Reliability first. Release second.
