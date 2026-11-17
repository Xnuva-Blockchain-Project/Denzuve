# TAO CP68 — Historical P2P Synchronisation

Date: 2026-11-17
Status: EXPERIMENTAL — DO NOT RELEASE

## Test configuration
- Legacy peer: original TAO 0.8.7.5.
- Receiving peer: modern TAO Core 31.1.
- Modern executable SHA256:
  cf4744971ba284fc3ce75662a4e3c39a5bd3ae3acf777373a92cd61c4fffc113
- Historical fixture: revival-replay-fixture/blk00000.dat.
- 637 block records, genesis through height 636.
- Isolated, loopback-only network namespace.
- Disposable datadirs.
- No mining and no live-network connections.
- Modern daemon did not directly import the fixture.
- No manual v2transport override.

## CP68C-R: Legacy import
- Original daemon accepted historical records.
- Final height: 636.
- Clean shutdown: exit code 0.
- Result: PASS.

## CP68D: Real P2P synchronisation
- Original node height: 636.
- Modern node progressed from height 0 to 636.
- Automatic v1 connection: PASS.
- Historical block reception and UpdateTip: PASS.
- Original and modern final best-block hashes match:
  7e235df054e299f729ef32e136fe0acc198aa792422fd24bc0309ecbc4847c88
- Result: CP68D_HISTORICAL_P2P_SYNC_PASS.

## Scope and limitations
This qualifies P2P transfer and validation of the
637-block TAO Revival fixture only.

Not yet qualified:
- Full historical/live-network chain synchronisation.
- Funded historical wallet migration and balance recovery.
- Persistent automated regression testing.
- Legacy alert-signature warning investigation.
- Final security, GUI and release packaging verification.

No genesis reset, historical rewrite, consensus
relaxation or production release is authorised.
