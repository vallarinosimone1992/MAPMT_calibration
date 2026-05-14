# Analysis

This directory contains the offline C++/ROOT software:

- `src/`: implementation.
- `include/`: public headers.
- `docs/`: technical notes on configuration and legacy files.
- `CMakeLists.txt`: build definition for the `mapmt_calibrate` command.

Build:

```bash
cmake -S . -B build
cmake --build build
```

Example from the suite root:

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
"$MAPMT_SUITE/ana/build/mapmt_calibrate" inspect-map \
  --config "$MAPMT_SUITE/cnf/detector.conf"
```

The build also provides `mapmt_bin2root`, a converter for binary TDC files
produced by `ssptest_TDCAll`:

```bash
"$MAPMT_SUITE/ana/build/mapmt_bin2root" \
  --input "$MAPMT_SUITE/data/tdc/ssprich_tdc_run.bin" \
  --output-dir "$MAPMT_SUITE/results/tdc_decode"
```

See `docs/binary_conversion.md` for the output formats.

The default maps under `../cnf/maps/` are the dRICH prototype maps imported from
`clas_RICH_software/suite2.0/maps/`. The pedestal reader supports the
`suite2.0` scaler format where each four-line header can be followed by one
fiber block rather than the full active setup.
