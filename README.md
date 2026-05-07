# MAPMT_calibration

Standalone C++/ROOT suite for calibrating MAPMTs read out with MAROC electronics and CODA DAQ.

The suite keeps the useful legacy text formats, but removes many hard-coded
assumptions from the analysis code. It uses `MAPMT_SUITE` as the suite root,
following the role previously played by `RICH_SUITE`.

Main areas:

- `ana/`: C++/ROOT offline code, the `mapmt_calibrate` CLI, and analysis docs.
- `daq/`: DAQ scripts that produce calibration files through `ssptest_*`.
- `cnf/`: detector constants, hardware maps, thresholds, and gains.

`data/` and `results/` are top-level operational areas.

## Build

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
cmake -S "$MAPMT_SUITE/ana" -B "$MAPMT_SUITE/ana/build"
cmake --build "$MAPMT_SUITE/ana/build"
```

Requires ROOT with either `root-config` or a CMake package available.

## DAQ Scripts

The scripts that produce calibration files with `ssptest_*` live in `daq/`.
They are wrappers around the legacy `sw/daq` scripts, with parameters collected
in `daq/daq.env`.

On the DAQ machine:

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
cd "$MAPMT_SUITE/daq"
cp daq.env.example daq.env
vi daq.env
./configure_all.sh 230 64 0
./rich_pedestal.sh
./rich_dark.sh
./rate_scan.sh 100 0 230
```

These scripts require CODA/SSP and a connection to the crate. The offline
`mapmt_calibrate` analysis requires only ROOT and already-produced data files.

## Main Commands

Inspect the hardware map:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" inspect-map
```

Analyze pedestals from a scaler file:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" pedestal \
  --input rich_pedestal_YYYYMMDD_HHMMSS.txt \
  --output "$MAPMT_SUITE/results/pedestal_run"
```

Main outputs:

- `channel_stats.csv`: per-channel statistics.
- `chip_pedestals.txt`: ASIC/PMT pedestal summary compatible with the legacy `thrCalc.c` input.
- `thresholds_suggested.txt`: thresholds computed as `ceil(ASIC_pedestal_mean) + offset`.
- `noisy_channels.txt` and `dead_channels.txt`.
- `histo.root`: ROOT `rate vs threshold` histograms and a summary TTree.

Generate thresholds from `chip_pedestals.txt`:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" thresholds \
  --chip-pedestals results/pedestal_run/chip_pedestals.txt \
  --output results/pedestal_run/threshold_25.txt \
  --offset 25
```

Time calibration from already-decoded TDC hits:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" time \
  --input decoded_tdc.csv \
  --output "$MAPMT_SUITE/results/time_run" \
  --format event-address \
  --reference 3:1:0:0
```

Supported TDC text formats:

- `abs`: `absChannel time [tot]`
- `address`: `slot fiber asic maroc time [tot]`
- `event-address`: `event slot fiber asic maroc time [tot]`
- `auto`: infer the format from the number of columns

The binary files `ssprich_tdc_*.bin` are the expected DAQ output. Their decoding
is intentionally external to this suite; `mapmt_calibrate time` starts from TDC
hits already decoded into one of the text formats above.

For details on the time-calibration input, output, and offset convention, see
`ana/docs/time_calibration.md`.

## Legacy Files To Check Before Migration

See `ana/docs/legacy_CLAS12_RICH_notes.md`. In short, the critical files are
`fiber.map`, `setup.txt`, `threshold*.txt`, and `gain.txt`. The most dangerous
parameter is `slot_base`: the legacy absolute-channel formula assumes the first
slot is 3.

The suite contains local copies of the required legacy files under `cnf/maps/`,
so it does not need to read anything from `clas_RICH_software` at runtime.

For parameter meanings, see also `ana/docs/configuration_files.md`.
