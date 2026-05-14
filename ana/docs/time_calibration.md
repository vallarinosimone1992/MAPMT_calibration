# Time Calibration

The new time-calibration code works on TDC hits already decoded into text/CSV.
The DAQ scripts correctly produce binary files named `ssprich_tdc_*.bin`;
decoding those binaries remains part of the existing external chain and is not
duplicated in this directory.

## Accepted Input

The command reads numeric rows separated by spaces or commas. Empty lines and
anything after `#` are ignored.

Formats:

```text
absChannel time [tot]
slot fiber asic maroc time [tot]
event slot fiber asic maroc time [tot]
```

Optional columns after `time`, for example `tot`, are ignored. With
`--format auto`, the format is inferred from the number of columns:

- 2 or 3 columns: `abs`
- 5 columns: `address`
- 6 or more columns: `event-address`

`absChannel` uses the legacy convention:

```text
absChannel = maroc + 64*asic + 192*fiber + 192*32*(slot - slot_base)
```

The `slot_base` parameter is defined in `cnf/detector.conf`.

## Command

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
"$MAPMT_SUITE/ana/build/mapmt_calibrate" time \
  --input "$MAPMT_SUITE/data/tdc/decoded_tdc.csv" \
  --output "$MAPMT_SUITE/results/time_run" \
  --format event-address \
  --reference 3:1:0:0
```

Main options:

- `--reference slot:fiber:asic:maroc`: channel used as the time zero.
- `--min-entries N`: minimum number of hits required to calibrate a channel, default `20`.
- `--bins N`: number of time-histogram bins, default `400`.
- `--min-time T` and `--max-time T`: manual histogram range.
- `--json-output FILE`: override the default JSON output path.
- `--legacy-mapmt-output FILE`: also write a legacy `MAPMT_time_calibration.dat`-style file.
- `--no-json`: disable JSON output.
- `--null-calibration`: write a zero/no-op calibration without TDC input.
- `--allow-empty`: fall back to a zero/no-op calibration if the input has no usable hits.

If `--reference` is not given, the time zero is the mean of the channel means
for all channels with enough statistics.

## Output

The command produces:

```text
time_offsets.csv
time_calibration.root
mapmt_time_calibration.json
```

`time_offsets.csv` contains one row per channel:

```text
slot,fiber,asic,maroc,pixel,module,pmt,tile,entries,mean_time,sigma_time,offset
```

`offset` is computed as:

```text
offset = reference_mean_time - channel_mean_time
```

A corrected time can therefore be built as:

```text
time_corrected = time_raw + offset
```

`time_calibration.root` contains:

- a `time_offsets` TTree with the same quantities as the CSV;
- one `hTime_slot_fiber_asic_maroc` histogram per calibrated channel.

`mapmt_time_calibration.json` is the preferred machine-readable output for the
new dRICH analysis. It stores explicit per-channel delays and enough metadata to
interpret them:

```json
{
  "schema_version": "1.0",
  "calibration_type": "mapmt_time_delay",
  "is_null_calibration": false,
  "method": {
    "estimator": "mean",
    "min_entries": 20,
    "reference_time": 184.25
  },
  "channels": [
    {
      "slot": 3,
      "fiber": 1,
      "asic": 0,
      "maroc": 0,
      "module": 1,
      "pmt": 1,
      "analysis_pmt": 0,
      "pixel": 58,
      "entries": 125,
      "delay": 184.25,
      "sigma": 0.73,
      "offset_vs_reference": 0.0
    }
  ]
}
```

`pmt` and `pixel` use the natural calibration convention: both are 1-based.
`analysis_pmt` is included for compatibility with the current dRICH analysis
convention and is defined as `pmt - 1`.

If `--legacy-mapmt-output FILE` is used, the command also writes:

```text
key value
```

with:

```text
key = analysis_pmt * 256 + pixel
value = delay
```

This legacy export is meant only for comparisons and compatibility checks. The
JSON file is the primary output.

## Delay, Offset, And Target Time

`delay` is the absolute channel peak/mean time in the same units as the decoded
TDC input. The current estimator is the arithmetic mean.

`offset_vs_reference` is:

```text
offset_vs_reference = reference_time - delay
```

This is the additive correction convention used by the older CSV/ROOT output:

```text
time_corrected = time_raw + offset_vs_reference
```

The new dRICH analysis should primarily consume `delay` and choose its own
target time:

```text
time_corrected = time_raw - delay + target_time
```

The legacy analysis used `target_time = 390`. That value is stored only as
metadata (`legacy_target_time`) and is not applied to `delay`.

## Null Calibration

When no data are available, use:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" time \
  --null-calibration \
  --output "$MAPMT_SUITE/results/time_null"
```

This writes a JSON file with one entry per mapped channel and:

```text
delay = 0
sigma = 0
offset_vs_reference = 0
entries = 0
is_null_calibration = true
```

Consumers should treat `is_null_calibration = true` as an explicit instruction
to leave the raw time unchanged. The zero values are not physical peak times.

## Operational Notes

The suite boundary is intentional: binary acquisition with `ssptest_TDCAll`,
external decoding, offline time calibration on decoded text/CSV. For this
directory, it is enough that the decoder produces at least `slot fiber asic
maroc time` or `event slot fiber asic maroc time`.

The current non-null calibration uses the arithmetic mean of the time
distribution. For real data with tails, noise, or multiple peaks, it may be
better to replace the mean with a main-peak fit or a robust windowed estimator.
