#!/usr/bin/env bash
set -euo pipefail

# Materialise the current TAO Revival-on-Bitcoin-Core-31.1 draft.
# DEVELOPMENT ONLY — DO NOT CONNECT TO LIVE TAO.

EXPECTED_UPSTREAM="9be056a8a72b624dae9623b2f7bded92c2a21c91"
AUTHORITATIVE_REVIVAL="74216f33f634f5dde37e96805c72c63c181c52df"
DEST="${1:-modern-tao-worktree}"

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

if [[ -e "$DEST" ]]; then
    echo "STOP: destination already exists: $DEST" >&2
    exit 1
fi

git submodule update --init --recursive modern-core
actual="$(git -C modern-core rev-parse HEAD)"
if [[ "$actual" != "$EXPECTED_UPSTREAM" ]]; then
    echo "STOP: modern-core commit mismatch" >&2
    echo "expected: $EXPECTED_UPSTREAM" >&2
    echo "actual:   $actual" >&2
    exit 1
fi

mkdir -p "$DEST"
# Keep upstream Git metadata out of the TAO build tree.
tar -C modern-core --exclude='.git' -cf - . | tar -C "$DEST" -xf -
cp -a migration/modern-tao-draft/. "$DEST"/

# TAO Checkpoint 31C: mainnet-only startup
# Apply the tested startup compatibility repair.
# Refuse fuzzy application against changed upstream code.
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-mainnet-only-startup.patch

# TAO Checkpoint 34: preserve Scrypt Proof of Work
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-scrypt-blockstorage.patch

# TAO Checkpoint 39: Scrypt mining nonce validation
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-scrypt-mining.patch

# TAO Checkpoint 45: critical GUI branding
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-qt-english-branding.patch

# TAO Checkpoint 47: application identity
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-qt-app-identity.patch

# TAO Checkpoint 48: approved artwork is retained as reproducible migration assets.
install -m 0644 migration/assets/tao-revival-icon.png "$DEST/src/qt/res/icons/tao-revival-icon.png"
install -m 0644 migration/assets/tao-revival-splash.png "$DEST/src/qt/res/icons/tao-revival-splash.png"
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-qt-splash-and-icon.patch

# TAO Checkpoint 48C: wallet support links
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-client-links.patch

# TAO Checkpoint 49A: client identity and offline status
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-qt-useragent-offline-status.patch

# TAO Checkpoint 51A: visible English Qt labels only
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-qt-visible-labels.patch


# TAO CP64: conservative user-facing English UI / warning wording.
# Protocol, wallet paths, licences and translations require separate review.
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-public-surface-branding.patch

# TAO CP65S: CLI help must use authoritative TAO mainnet only.
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-cli-mainnet-only-startup.patch

# TAO CP66G: retain original OpenSSL short DER key compatibility.
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-legacy-short-form-ec-der.patch

# TAO CP66H: preserve original TaoCoin signed-message interoperability.
patch --batch --fuzz=0 --forward -p1 -d "$DEST" \
  < migration/patches/tao-historical-signed-message-domain.patch

if [[ -e "$DEST/.git" || -L "$DEST/.git" ]]; then
    echo "STOP: unexpected Git metadata in build tree" >&2
    exit 1
fi

cat <<EOF
TAO Revival modern draft materialised at:
  $DEST

Bitcoin Core baseline:
  $EXPECTED_UPSTREAM

Authoritative TAO Revival source:
  $AUTHORITATIVE_REVIVAL

Status:
  UNTESTED / DO NOT RELEASE / DO NOT CONNECT TO LIVE TAO

This command only creates a source working tree. It does not launch a node,
modify tao-revival, or create a blockchain.
EOF
