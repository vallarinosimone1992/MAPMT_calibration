# Configuration Files

There are two configuration families:

1. DAQ parameters used to run `ssptest_*`.
2. Offline maps and calibrations used to interpret channels and produce thresholds, gains, and offsets.

Local copies of the legacy files are under `cnf/maps/`. The top-level files
there (`cnf/maps/fiber.map`, `cnf/maps/setup.txt`, `cnf/maps/threshold.txt`,
and `cnf/maps/gain.txt`) are the default configuration and do not require any
path into `clas_RICH_software`.

`MAPMT_SUITE` is the suite root, the replacement for `RICH_SUITE`.

## 1. DAQ Configuration: `daq/daq.env`

This file is not used by the C++/ROOT analysis. It is used only by the scripts
in `daq/`.

Essential parameters:

- `MAPMT_DAQ_EXEDIR`: path to `ssptest_ConfigureAll`, `ssptest_ScalerAll`, and `ssptest_TDCAll`.
- `MAPMT_SCALER_DURATION`: counting time for each threshold point.
- `MAPMT_Q`: `Q` argument passed to `ssptest_ScalerAll`; legacy value was `0`.
- `MAPMT_DEFAULT_THRESHOLD`: threshold used by `configure_all.sh` and `rate_scan.sh`.
- `MAPMT_DEFAULT_GAIN`: uniform gain when the gain map is not used.
- `MAPMT_DEFAULT_SUPPRESS`: third argument to `ssptest_ConfigureAll`; legacy value was `0`.
- `MAPMT_PEDESTAL_START/STOP/GAIN`: pedestal scan range.
- `MAPMT_DARK_START/STOP/GAIN`: dark-rate scan range.
- `MAPMT_RATE_RUNS`, `MAPMT_RATE_FIXED_THRESHOLD`, `MAPMT_RATE_GAIN`: fixed-threshold rate scan settings.
- `MAPMT_TDC_THRESHOLD`, `MAPMT_TDC_EVENTS`, `MAPMT_TDC_MODE`: arguments for `ssptest_TDCAll`.

In the legacy scripts, `gain=0` means "use the configured gain map"; `gain=64`
means uniform gain 64.

## 2. `setup.txt`

Produced by `ssptest_ConfigureAll` and used to identify which fibers/ASICs were
seen by the DAQ.

The legacy format has:

- 23 initial header lines.
- Then rows `slot fiber nasics`.

Example:

```text
3 0 3
3 1 3
3 2 0
```

The old code always skipped the first 23 lines. In the new software this number
is controlled by:

```text
setup_header_lines = 23
```

in `cnf/detector.conf`.

## 3. `fiber.map`

This is the most important map. It links electronics to detector geometry:

```text
slot fiber asic module pmt tile
```

Example:

```text
3 1 0 1 1 1
3 1 1 1 2 1
3 1 2 1 3 1
```

This means that slot 3, fiber 1, ASIC 0 corresponds to PMT 1 in module 1,
tile 1.

For a new project, rebuild this file with:

- real SSP slots;
- connected fibers;
- ASICs present on each fiber;
- module number;
- PMT number;
- tile or mechanical group, if useful.

If this file is wrong, the whole calibration will be assigned to the wrong
PMT/channel.

## 4. `threshold.txt`

Format:

```text
slot fiber asic threshold_dac
```

Example:

```text
3 1 0 210
3 1 1 214
```

The pedestal flow generates it as:

```text
threshold = ceil(ASIC_pedestal_mean) + offset
```

with a typical offset of `25`.

## 5. `gain.txt`

Format:

```text
slot fiber channel gain
```

where `channel` is from 0 to 191:

```text
asic = channel / 64
maroc = channel % 64
```

Example:

```text
3 0 0 64
3 0 1 64
3 0 64 70
```

If the file is missing, the new software uses `default_gain`.

## 6. `PmtPedestal.txt` / `chip_pedestals.txt`

Format:

```text
slot fiber asic module pmt mean rms
```

This is the ASIC/PMT summary used to generate `threshold.txt`.

## 7. `cnf/detector.conf`

This file contains electronics constants and decoding conventions. Parameters
to check for a new setup:

- `slot_base`: in the legacy formula `absChannel = maroc + 64*asic + 192*fiber + 192*32*(slot-slot_base)`. For CLAS12 this was `3`.
- `clock_hz`: scaler reference clock, legacy value `125000000`.
- `max_fibers`, `n_asics`, `n_pixels`: electronics structure.
- `threshold_min`, `threshold_max`: expected scan range.
- `flat_rate_min`, `flat_rate_max`: region used to estimate the dark-rate plateau.
- `shoulder_range`: region close to the threshold.
- `noisy_pedestal_rms`: cut used to tag noisy channels.

The Hamamatsu MAROC-to-anode map is still the legacy CLAS12 map. If the new
cabling or MAPMT model changes, it must be updated in code or made
configurable.
