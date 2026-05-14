# Configuration

This directory contains the files needed to interpret channels and run the
offline calibration:

- `detector.conf`: general electronics constants and decoding conventions.
- `maps/`: hardware maps, thresholds, gains, and average pedestal files.

The files `maps/setup.txt`, `maps/fiber.map`, `maps/threshold.txt`, and
`maps/gain.txt` are the active default configuration for this suite.

Subdirectories under `maps/` are archived legacy configurations and are not read
by default.

For a new project, the most important file to rewrite is `maps/fiber.map`.
