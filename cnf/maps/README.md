# Local Maps And Calibrations

This directory contains local copies of the configuration and calibration files
needed by the suite. Runtime code must not read from `clas_RICH_software`.

Top-level files are the default configuration copied from the legacy
`TwoModules` setup:

- `setup.txt`
- `fiber.map`
- `threshold.txt`
- `gain.txt`
- `PmtPedestal.txt`

These are the files used by the examples in the top-level `README.md`.

## Archived Legacy Configurations

- `RICH1/`: map and thresholds for RICH1.
- `RICH2/`: map, thresholds, gain, and `chip.txt` for RICH2.
- `TwoModules/`: complete two-module configuration, used as the default source.

## Files To Edit For A New Project

To operate a new setup, create or update at least:

- `fiber.map`: slot/fiber/ASIC to module/PMT/tile mapping.
- `setup.txt`: output consistent with `ssptest_ConfigureAll`, or an equivalent file.
- `threshold.txt`: one threshold per active ASIC.
- `gain.txt`: one gain per channel.

`PmtPedestal.txt` is not required to analyze a new run, but it is useful as a
reference or to generate initial thresholds.
