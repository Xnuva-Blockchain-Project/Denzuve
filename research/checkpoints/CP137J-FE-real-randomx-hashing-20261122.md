# CP137J-FE — Real RandomX V2 contextual-hashing research checkpoint

**Date:** 2026-11-22  
**Branch:** `modernisation/taox-randomx`  
**Status:** EXPERIMENTAL / DO NOT RELEASE / NO MAINNET ACTIVATION

This record captures console-reported outcomes for CP137J-FA through FD. The local `taox-build-lab` sources, executables, and logs remain outside Git and have **not** been committed as runnable test artifacts. Results were observed from the developer's terminal output, not independently reproduced by GitHub CI.

## Results

- **FA:** Existing `tao-randomx-selftest` exited 0; both pinned RandomX V2 reference vectors passed, including an 80-byte serialized block-header fixture. CMake option `TAO_EXPERIMENTAL_RANDOMX_HASHING` defaults OFF in source and is ON only in the isolated lab build.
- **FB:** The fetched engine source revision was checked directly and matched pinned commit `857841056ed939eeea4fd640463b54b6a5cd54af`. Existing experimental adapter, shared foundation and static-library link dependencies located.
- **FC:** A combined isolated C++ harness calculated **genuine** RandomX hashes for two synthetic competing block branches using parent-derived seeds, checked exact branch-specific `nBits`, and compared those hashes against derived PoW targets. Both real candidate hashes were **above** their targets (correctly reported not met). Invalid `nBits` and wrong parent were rejected prior to hashing.
- **FD:** The FC harness was extended and executed successfully: same header + same seed reproduced the same digest, changing the seed changed the digest, changing the nonce changed the digest, and missing RandomX seed failed closed. Existing FC checks also passed again.

## Reported SHA-256 evidence (local; not uploaded to GitHub)

- FA self-test log: `74a1977946105b77770beea9093e3d1bbebd0ea8dcfc92fef1ade5275b66f362`
- FC `runtime.log`: `26f275a062d11d6b302eb377c554b64399d0bfb5eeb0e7251f8b26e64ae9824e`
- FC `combined-real-test`: `1cd4a64bf23fb8f7f26ea79dea41aeee8334e0fb6e0007304d20085c18780e7c`
- FD `runtime.log`: `66884cf424d82279840acfa4de022ff7ba48e42664d41d30b42496fb04082048`
- FD `combined-fd-test`: `efb15b95f9f97b23c855555c364ceb7cd311acba843d7389fe2c135e6ebeb0ca`

## Constraints and open release blockers

- Existing selector `SelectPoWAlgorithm()` remains **Scrypt-only**; RandomX is disabled for live mainnet.
- Synthetic ancestry flags are not proof of fully validated ancestor history.
- Illustrative seed epochs, activation heights, first-target `nBits`, and retarget periods are not approved consensus policy.
- Successful RandomX hash calculations **do not** demonstrate a genuinely target-qualifying nonce, mined block acceptance, real network validation, chainwork security, presync header handling, reorg protection, wallet accounting, or recovery after restart.
- Next focused test: **positive real RandomX PoW** using an explicitly easy *isolated test target*, a bounded nonce search, and an independent compact-target comparison. No mainnet connection or block submission.
- After that, audit and test the full validation path, header presynchronisation, and mixed-algorithm chainwork before considering any deployment.

**Golden standard:** Reliability first. Release second.
