# TaoX — Experimental Wallet Qualification

Date: 24 November 2026

Status: Partial qualification — NOT release-ready.

## Environment

- Isolated TaoX regtest
- Historical Scrypt followed by experimental RandomX
- Mainnet RandomX activation remains disabled
- No production blockchain modifications

## Verified local results

- Legacy P2PKH receiving addresses generated
- Legacy P2PKH change addresses generated
- Unsupported Bech32 address requests rejected
- Existing wallet reloaded after node restart
- Blockchain advanced through RandomX activation
- RandomX regtest mining verified through height 22
- Coinbase maturity verified at 20 blocks
- Mature coinbase reward successfully spent
- One TAO transferred to a separate test wallet
- Transaction confirmed in block 22
- Legacy payment and change outputs verified
- Sender ownership of change verified
- Both wallets recovered their balances after restart

## Confirmed transaction accounting

- Payment: 1 TAO
- Transaction fee: 0.0000225 TAO
- Change returned: 48.9999775 TAO
- Sender spendable balance: 98.9999775 TAO
- Recipient spendable balance: 1 TAO

All amounts are isolated regtest values.

## Outstanding release requirements

1. Independent wallet backup restoration
2. Historical wallet and script compatibility
3. Independent peer block acceptance
4. Network-facing P2P PRESYNC and REDOWNLOAD security
5. Shutdown, restart and sustained-operation testing
6. Final compatibility and release review

## Source qualification status

Three wallet compatibility source files remain
modified locally and uncommitted pending further tests.

The full evidence manifest and individual RPC results
are retained locally, outside this Git repository.

This report does not authorise mainnet activation,
production deployment or a release announcement.

Reliability first. Release second.
