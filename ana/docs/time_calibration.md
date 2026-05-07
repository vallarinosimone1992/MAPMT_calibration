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

If `--reference` is not given, the time zero is the mean of the channel means
for all channels with enough statistics.

## Output

The command produces:

```text
time_offsets.csv
time_calibration.root
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

## Operational Notes

The suite boundary is intentional: binary acquisition with `ssptest_TDCAll`,
external decoding, offline time calibration on decoded text/CSV. For this
directory, it is enough that the decoder produces at least `slot fiber asic
maroc time` or `event slot fiber asic maroc time`.

The current calibration uses the arithmetic mean of the time distribution. For
real data with tails, noise, or multiple peaks, it may be better to replace the
mean with a main-peak fit or a robust windowed estimator.
