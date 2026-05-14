# DAQ Scripts

These scripts are imported and lightly updated versions of the scripts in
`clas_RICH_software/rich_sw/sw/daq`. They must be run on the DAQ/crate machine
where `ssptest_ConfigureAll`, `ssptest_ScalerAll`, and `ssptest_TDCAll` are
available.

## Configuration

1. Copy `daq.env.example` to `daq.env`.
2. Edit at least `MAPMT_DAQ_EXEDIR` if the `ssptest_*` commands are not in `PATH`.
3. Check scan ranges, gains, and durations.

Example from a bash shell:

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
cd "$MAPMT_SUITE/daq"
cp daq.env.example daq.env
vi daq.env
./configure_all.sh 230 64 0
./rich_pedestal.sh
./rich_dark.sh
./rate_scan.sh 100 0 230
./rich_tdc.sh
```

If the DAQ account uses `csh` or `tcsh`, do not source `daq.env` directly. It
uses bash syntax and is read internally by the wrapper scripts. Instead:

```csh
cd /path/to/MAPMT_daq_only/daq
cp setup_daq.csh.example setup_daq.csh
cp daq.env.example daq.env
vi setup_daq.csh
vi daq.env
source setup_daq.csh
./configure_all.sh 230 64 0
./rich_pedestal.sh
```

The `./*.sh` commands still run under bash because of their shebang line. This
requires `/usr/bin/env bash` to find bash on the DAQ machine. Check with:

```csh
which bash
/usr/bin/env bash --version
```

The legacy `ssptest_*` executables may still read maps from
`$RICH_SUITE/maps`. The provided `setup_daq.csh.example` creates the
compatibility symlink:

```text
$MAPMT_SUITE/maps -> $MAPMT_SUITE/cnf/maps
```

If the DAQ executable reports `Gain file .../maps/gain.txt not found`, create
that link manually:

```csh
cd $MAPMT_SUITE
ln -s cnf/maps maps
ls -l maps/gain.txt
```

## Main Parameters In `daq.env`

- `MAPMT_SUITE`: suite root, the modern replacement for `RICH_SUITE`.
- `MAPMT_DAQ_EXEDIR`: directory containing the DAQ executables. In the legacy setup this was `$CODA/src/rol/Linux_i686_vme/bin`.
- `MAPMT_SETUP_FILE`: `setup.txt` file produced by `ssptest_ConfigureAll`.
- `MAPMT_PED_DATA_DIR`: destination for scaler/pedestal files.
- `MAPMT_TDC_DATA_DIR`: destination for binary TDC files.
- `MAPMT_Q`: `Q` argument passed to `ssptest_ScalerAll`; legacy value was `0`.
- `MAPMT_SCALER_DURATION`: counting duration in seconds for each threshold point; legacy value was `2`.
- `MAPMT_DEFAULT_THRESHOLD`: threshold used by configuration and rate scans; legacy value was `230`.
- `MAPMT_DEFAULT_GAIN`: default uniform gain; legacy value was `64`.
- `MAPMT_PEDESTAL_START`, `MAPMT_PEDESTAL_STOP`, `MAPMT_PEDESTAL_GAIN`: pedestal scan settings. The suite2.0 dRICH pedestal script used `160..220` with gain `64`.
- `MAPMT_DARK_START`, `MAPMT_DARK_STOP`, `MAPMT_DARK_GAIN`: dark-rate scan settings.
- `MAPMT_RATE_RUNS`, `MAPMT_RATE_FIXED_THRESHOLD`, `MAPMT_RATE_GAIN`: fixed-threshold rate scan settings.
- `MAPMT_TDC_*`: parameters for `ssptest_TDCAll`.

`gain=0` preserves the legacy behavior: use the gain map loaded by the
configuration.

## Installing Suggested Thresholds In suite2.0 DAQ

`mapmt_calibrate pedestal` writes `thresholds_suggested.txt` in the same text
format used by `suite2.0/maps/threshold.txt`:

```text
slot fiber asic threshold_dac
```

To use those thresholds with the legacy `suite2.0` standalone DAQ, copy the
file into the active map directory, then configure with threshold `0`.
According to the `suite2.0` documentation, `ssptest_ConfigureAll` uses:

```text
ssptest_ConfigureAll [threshold, use 0 for map] [gain, use 0 for map] [ctest amplitude]
```

From a `csh`/`tcsh` DAQ account:

```csh
setenv RICH_SUITE /home/clasrun/rich/suite2.0
set stamp = `date +%Y%m%d_%H%M%S`

cp $RICH_SUITE/maps/threshold.txt $RICH_SUITE/maps/threshold.txt.$stamp.bak
cp /path/to/thresholds_suggested.txt $RICH_SUITE/maps/threshold.txt

awk 'NF!=4 || $4<0 || $4>1023 {print; bad=1} END{exit bad}' \
  $RICH_SUITE/maps/threshold.txt

cd $RICH_SUITE/sw/daq
ssptest_ConfigureAll 0 0 0
```

Use `ssptest_ConfigureAll 0 64 0` instead if the threshold map should be used
with a uniform MAROC gain of `64`. Passing a nonzero first argument such as
`230` ignores the threshold map and programs that same threshold everywhere.

Changing `maps/threshold.txt` is not enough by itself: the MAROC boards are
updated only when `ssptest_ConfigureAll` is run again.

## Relationship With Offline Calibration Files

The DAQ scripts produce files such as:

- `data/ped/rich_pedestal_YYYYMMDD_HHMMSS.txt`
- `data/ped/rich_dark_YYYYMMDD_HHMMSS.txt`
- `data/ped/rich_rate_YYYYMMDD_HHMMSS.txt`
- `data/tdc/ssprich_tdc_*.bin`

Scaler files in `data/ped` can be analyzed with:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" pedestal \
  --input "$MAPMT_SUITE/data/ped/rich_pedestal_YYYYMMDD_HHMMSS.txt" \
  --config "$MAPMT_SUITE/cnf/detector.conf" \
  --map "$MAPMT_SUITE/cnf/maps/fiber.map" \
  --thresholds "$MAPMT_SUITE/cnf/maps/threshold.txt" \
  --gains "$MAPMT_SUITE/cnf/maps/gain.txt" \
  --output "$MAPMT_SUITE/results/pedestal_YYYYMMDD_HHMMSS"
```

If the run-specific `rich_setup_YYYYMMDD_HHMMSS.txt` differs from
`cnf/maps/setup.txt`, pass it explicitly:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" pedestal \
  --input "$MAPMT_SUITE/data/ped/rich_pedestal_YYYYMMDD_HHMMSS.txt" \
  --setup "$MAPMT_SUITE/data/ped/rich_setup_YYYYMMDD_HHMMSS.txt" \
  --output "$MAPMT_SUITE/results/pedestal_YYYYMMDD_HHMMSS"
```

The binary TDC file is the correct DAQ output. The offline `time` command uses
an already-decoded TDC file; decoding can remain in the existing external chain.
For local decoding, the analysis build provides:

```bash
"$MAPMT_SUITE/ana/build/mapmt_bin2root" \
  --input "$MAPMT_SUITE/data/tdc/ssprich_tdc_*.bin" \
  --output-dir "$MAPMT_SUITE/results/tdc_decode"
```
