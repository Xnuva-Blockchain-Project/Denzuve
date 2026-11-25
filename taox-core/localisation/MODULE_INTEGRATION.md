# Independent Localisation dependency

Status: experimental Denzuve development integration.

Repository:
https://github.com/Xnuva-Blockchain-Project/Localisation

Dependency path:
modules/localisation

Pinned revision:
05ea3e65c17e4c79dbe22fd9bffc6e121a9f5277

The Git submodule commit is the authoritative dependency
revision. The development branch must not be followed
automatically during application builds.

## Application-owned inputs

- identity/denzuve.json
- source-inventory.ts

The source inventory is preserved from the qualified
Denzuve presentation development baseline.

It is not automatically refreshed by this checkpoint.
Application source changes require renewed extraction
and source inventory qualification.

## Shared module responsibilities

- Identity validation.
- Portable branding templates.
- Deterministic catalogue generation.
- Independent catalogue validation.
- Application-aware filtering.
- Native Qt translation compilation.
- Deterministic Qt resource generation.
- Reusable CMake integration.

## Outstanding application integration

- Connect the existing Qt application build target.
- Prevent duplicate translation resource registration.
- Explicitly initialise static-library resources.
- Qualify the integrated Denzuve wallet.
- Verify application source inventory freshness.
- Review translation suppression and placeholders.

No consensus changes, monetary changes, production
deployment or mainnet activation are authorised.
