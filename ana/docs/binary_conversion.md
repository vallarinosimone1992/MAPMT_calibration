# Binary TDC Conversion

`mapmt_bin2root` is the MAPMT-calibration version of the legacy
`clas_RICH_software/conv_bin/bin2root.c` converter.

It decodes the binary output produced by `ssptest_TDCAll` and writes:

- a ROOT file compatible with the dRICH raw-analysis input tree;
- a decoded text/CSV file directly usable by `mapmt_calibrate time`;
- a small text summary.

## Command

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
"$MAPMT_SUITE/ana/build/mapmt_bin2root" \
  --input "$MAPMT_SUITE/data/tdc/ssprich_tdc_run.bin" \
  --output-dir "$MAPMT_SUITE/results/tdc_decode" \
  --csv-edge 0
```

Default outputs are derived from the input filename:

```text
<input-stem>.root
<input-stem>_tdc.csv
<input-stem>_summary.txt
```

They can be overridden with:

```bash
--output-root FILE
--output-csv FILE
--summary FILE
```

Other useful options:

- `--config detector.conf`: load electronics constants such as `clock_hz`, `n_asics`, and `n_pixels`.
- `--csv-edge 0|1|all`: choose which edge polarity is exported to the CSV. The ROOT output always contains all edges.
- `--max-events N`: stop after `N` parsed events.
- `--no-root`: skip ROOT output.
- `--no-csv`: skip CSV output.

With `MAPMT_SUITE` set, `cnf/detector.conf` is loaded automatically.

## ROOT Output

The ROOT file contains a `data` TTree with the branches expected by the new
dRICH prototype analysis:

```text
evt
trigtime
nedge
slot[nedge]
fiber[nedge]
ch[nedge]
pol[nedge]
time[nedge]
```

It also stores basic summary histograms:

- `hEdgeMultiplicity`
- `hTdcTime`
- `hTriggerTime`

## CSV Output

The CSV is whitespace-separated and starts with comment lines. Data rows are:

```text
event slot fiber asic maroc time edge trigtime raw_channel
```

The first six numeric columns match the `event-address` input accepted by:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" time \
  --input decoded_tdc.csv \
  --output "$MAPMT_SUITE/results/time_run" \
  --format event-address
```

Extra columns are ignored by the time-calibration command and are kept only for
diagnostics.

## Edge Convention

The binary format stores FPGA edge polarity as:

```text
0 = rising
1 = falling
```

For time calibration, export only the edge that represents the timing pickoff
for the run. The default is `--csv-edge 0`, matching the current dRICH
prototype analysis configuration. Use `--csv-edge 1` if the run was configured
with the opposite timing edge.

`--csv-edge all` is useful for diagnostics but usually should not be used
directly for time calibration, because rising and falling edges would be mixed
in the same channel distribution.

## Parser Notes

The converter keeps the useful decoding rules from the legacy macro:

- tag 0: block header;
- tag 1: block trailer;
- tag 2: event header;
- tag 3: trigger timestamp;
- tag 7: fiber marker;
- tag 8: TDC edge.

Unlike the old macro, it does not hard-code a final slot such as slot 5. Events
are closed when a new trigger ID is encountered and once again at end of file,
which is more appropriate for setups with different slot ranges.
