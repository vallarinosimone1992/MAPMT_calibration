# Notes On The CLAS12 RICH MAPMT Software

This summary is based on the files in `clas_RICH_software/rich_sw`.

## Legacy Flow

1. On the RICH crate, CODA/SSP configuration is performed with `ssptest_ConfigureAll`.
2. The scripts `sw/daq/thr_scan.sh`, `rich_pedestal.sh`, `rich_dark.sh`, and `rate_scan.sh` call `ssptest_ScalerAll`.
3. The `ssprich_scaler_???.txt` output is concatenated into `rich_pedestal_YYYYMMDD_HHMMSS.txt`.
4. Off the crate, `sw/ana/ped/recoScalers.sh` runs ROOT with `richReco.c`.
5. `richReco.c` uses `richScalers.h`, which reads the hardware map, thresholds, gains, and scaler file, then produces `histo.root`, `stat.txt`, `chip.txt`, `rate.txt`, `hot.txt`, `dead.txt`, `noisy.txt`, and PDFs.

## Important Configuration Files

- `maps/*/setup.txt`: first 23 lines are an SSP header, followed by `slot fiber nasics` rows. The legacy code always skips 23 lines.
- `maps/*/fiber.map`: rows `slot fiber asic module pmt tile`. This is the most important file to port the software to a different geometry.
- `maps/*/threshold.txt` or `threshold_25.txt`: rows `slot fiber asic threshold`.
- `maps/*/gain.txt`: rows `slot fiber channel gain`, where `channel` is 0..191 and is converted as `asic=channel/64`, `maroc=channel%64`.
- `maps/*/PmtPedestal.txt`: mean and RMS per ASIC/PMT, format `slot fiber asic module pmt mean rms`.
- `sw/cnf/generate/thr/thrCalc.c`: generates thresholds as `ceil(mean_pedestal_chip) + offset`.
- `sw/cnf/generate/thr/thrCalc_file_2x.c`: includes the ADC/TDC factor 2.0, commented as 0.7 fC ADC and 1.4 fC TDC.
- `sw/cnf/generate/gain/gain.c`: converts a channel-gain table into CODA registers in groups of 16 channels.

## Pedestals

In the legacy code the pedestal is not a direct mean of ADC samples. The
threshold scan produces a `rate vs threshold` histogram for each MAROC channel;
the pedestal mean is computed as the threshold mean weighted by the rate. The
`chip.txt` file then contains the mean over the 64 channels of one ASIC/PMT and
is used to propose thresholds.

Hard-coded legacy values:

- scaler clock: `125E6`;
- default threshold: `230`;
- default gain: `64`;
- accepted pedestal range: 150..220 DAC;
- noisy channel: pedestal RMS > 4 DAC;
- flat dark-rate region: 400..425 DAC;
- threshold shoulder: `threshold..threshold+25`;
- Hamamatsu MAROC-to-anode map hard-coded in the `richScalers` constructor.

## Time/TDC

This copy of the software does not contain a real TDC time-calibration analysis.
The only dedicated file is `sw/daq/rich_tdc.sh`, which calls `ssptest_TDCAll`
and renames `ssprich_tdc.bin` with parameters in the filename (`BL1`,
frequency, window, lookback). No documented decoder for the binary format is
present here.

For this reason, the new suite separates the problem into two layers:

- TDC acquisition/decoding specific to the DAQ;
- time calibration on already-decoded hits in text or CSV form.

The new `time` command works on the second layer and produces per-channel
offsets with respect to a reference channel or to the global mean.

## Migration Risks For A New Project

- The `absChannel -> slot/fiber/asic/channel` decoding assumes `slot_base=3`. If the new slots start at 13, 0, or anything else, change it.
- `NSECTORS=2`, `NPMTS=391`, and `NTILES=138` were CLAS12 RICH dimensions. The new code does not need them for numerical analysis, but they remain implicit in the old displays.
- The CLAS12 display geometry in `InitDisplay()` is not portable. The new software keeps the numerical calibration, not the RICH-shaped display graphics.
- `setup.txt` is not self-describing; treat `fiber.map` as the primary source of active geometry.
- Gain and threshold files must be coherent with the same map. Mismatches between DAQ and `fiber.map` produce discarded channels or wrong PMT assignments.
- The binary TDC format `ssprich_tdc.bin` remains outside this suite: time calibration requires an explicit decoded format.
