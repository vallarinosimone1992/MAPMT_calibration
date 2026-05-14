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

On a new Linux machine, always configure from a clean build directory:

```bash
rm -rf build
source /opt/root-cern/bin/thisroot.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --verbose
```

The verbose compile line must contain `-std=c++17` or a newer C++ standard. If
ROOT headers fail in `ROOT/TypeTraits.hxx` with template redefinition errors,
the usual cause is that the project is being compiled without C++17 or with a
stale build directory from another machine.

ROOT 6.24 often reports `-std=c++14` through `root-config --cflags`. The CMake
build removes ROOT-provided `-std=...` flags by default and then applies the
project C++17 standard. If the verbose build still shows a later `-std=c++14`,
configure explicitly with:

```bash
cmake -S . -B build -DCMAKE_CXX_STANDARD=17 -DMAPMT_STRIP_ROOT_CXX_STANDARD=ON
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
