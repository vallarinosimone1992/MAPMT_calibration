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
