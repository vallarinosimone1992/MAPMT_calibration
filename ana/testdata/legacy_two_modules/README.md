# Legacy TwoModules Test Data

This directory contains a small legacy calibration set copied from:

```text
clas_RICH_software/rich_sw/maps/TwoModules/
```

Files:

- `PmtPedestal.txt`: reconstructed pedestal summary, one row per ASIC/PMT.
- `threshold_25.expected.txt`: expected legacy thresholds with offset 25.
- `gain.txt`: per-channel gain, legacy format.
- `fiber.map`: electronics-to-geometry map for the TwoModules setup.

A complete raw scaler file produced by `ssptest_ScalerAll` is not available
here. This fixture therefore tests threshold export from already-reconstructed
pedestals, not the full scaler-based pedestal reconstruction.

Test:

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
"$MAPMT_SUITE/ana/build/mapmt_calibrate" thresholds \
  --chip-pedestals "$MAPMT_SUITE/ana/testdata/legacy_two_modules/PmtPedestal.txt" \
  --output "$MAPMT_SUITE/results/test_legacy_threshold_25.txt" \
  --offset 25

diff -u \
  "$MAPMT_SUITE/ana/testdata/legacy_two_modules/threshold_25.expected.txt" \
  "$MAPMT_SUITE/results/test_legacy_threshold_25.txt"
```

The `diff` must be empty.

The most useful files to export toward an external analysis package are:

- `threshold_25.expected.txt`, or a new threshold file produced with the command above.
- `gain.txt`.
- `fiber.map`, if the external package must translate slot/fiber/ASIC/channel into module/PMT/pixel.
