# Legacy TwoModules test data

Questa cartella contiene un piccolo set di calibrazioni legacy copiato da:

```text
clas_RICH_software/rich_sw/maps/TwoModules/
```

File:

- `PmtPedestal.txt`: piedistalli gia ricostruiti, una riga per ASIC/PMT.
- `threshold_25.expected.txt`: soglie legacy attese con offset 25.
- `gain.txt`: gain per canale, formato legacy.
- `fiber.map`: mappa elettronica-geometria del setup TwoModules.

Non e presente un file scaler grezzo completo prodotto da `ssptest_ScalerAll`;
quindi questo set testa la fase di esportazione soglie da piedistalli gia
ricostruiti, non il fit/scansione completa dei piedistalli da scaler.

Test:

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
"$MAPMT_SUITE/ana/build/mapmt_calibrate" thresholds \
  --chip-pedestals "$MAPMT_SUITE/ana/testdata/legacy_two_modules/PmtPedestal.txt" \
  --output "$MAPMT_SUITE/results/test_legacy_threshold_25.txt" \
  --offset 25

diff -u \
  "$MAPMT_SUITE/ana/testdata/legacy_two_modules/threshold_25.expected.txt" \
  "$MAPMT_SUITE/results/test_legacy_threshold_25.txt"
```

Il `diff` deve essere vuoto.

Per esportare una calibrazione verso il software di analisi, i file piu utili
sono:

- `threshold_25.expected.txt` o una nuova soglia prodotta con il comando sopra;
- `gain.txt`;
- `fiber.map`, se il software esterno deve tradurre slot/fiber/ASIC/canale in
  modulo/PMT/pixel.
