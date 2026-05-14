# Local Maps And Calibrations

This directory contains local copies of the configuration and calibration files
needed by the suite. Runtime code must not read from `clas_RICH_software`.

Top-level files are the active default configuration used by the suite. They
are currently the dRICH prototype maps imported from
`clas_RICH_software/suite2.0/maps/`:

- `setup.txt`
- `fiber.map`
- `threshold.txt`
- `gain.txt`
- `PmtPedestal.txt`

These are the files used by the examples in the top-level `README.md`.

## Archived Legacy Configurations

- `dRICH_prototype/`: direct copy of the dRICH prototype map set from
  `suite2.0/maps/dRICH_prototype/`.
- `RICH1/`: map and thresholds for RICH1.
- `RICH2/`: map, thresholds, gain, and `chip.txt` for RICH2.
- `TwoModules/`: complete two-module legacy configuration.

These subdirectories are kept only as references. The analysis code does not
read them unless their files are passed explicitly on the command line.

## Files To Edit For A New Project

To operate a new setup, create or update at least:

- `fiber.map`: slot/fiber/ASIC to module/PMT/tile mapping.
- `setup.txt`: output consistent with `ssptest_ConfigureAll`, or an equivalent file.
- `threshold.txt`: one threshold per active ASIC.
- `gain.txt`: one gain per channel.

`PmtPedestal.txt` is not required to analyze a new run, but it is useful as a
reference or to generate initial thresholds.

For the dRICH prototype import, `setup.txt` is the active hardware filter while
`fiber.map` is the larger electronics-to-detector map. If a new
`ssptest_ConfigureAll` run changes the connected fibers, update
`cnf/maps/setup.txt` or pass the generated setup file explicitly with
`--setup`.
