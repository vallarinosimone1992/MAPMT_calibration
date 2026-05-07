# DAQ scripts

Questi script sono la versione importata e leggermente aggiornata degli script in
`clas_RICH_software/rich_sw/sw/daq`. Devono essere eseguiti sulla macchina DAQ/crate
dove sono disponibili `ssptest_ConfigureAll`, `ssptest_ScalerAll` e `ssptest_TDCAll`.

## Configurazione

1. Copiare `daq.env.example` in `daq.env`.
2. Modificare almeno `MAPMT_DAQ_EXEDIR`, se i comandi `ssptest_*` non sono nel `PATH`.
3. Controllare range, gain e durata degli scan.

Esempio:

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
cd "$MAPMT_SUITE/daq"
cp daq.env.example daq.env
vi daq.env
./configure_all.sh 230 64 0
./rich_pedestal.sh
./rich_dark.sh
./rate_scan.sh 100 0 230
./rich_tdc.sh
```

## Parametri principali in `daq.env`

- `MAPMT_SUITE`: radice della suite, equivalente moderno di `RICH_SUITE`.
- `MAPMT_DAQ_EXEDIR`: directory degli eseguibili DAQ. Nel legacy era
  `$CODA/src/rol/Linux_i686_vme/bin`.
- `MAPMT_SETUP_FILE`: file `setup.txt` prodotto da `ssptest_ConfigureAll`.
- `MAPMT_PED_DATA_DIR`: destinazione dei file scaler/piedistallo.
- `MAPMT_TDC_DATA_DIR`: destinazione dei file TDC binari.
- `MAPMT_Q`: argomento `Q` passato a `ssptest_ScalerAll`; nel legacy valeva `0`.
- `MAPMT_SCALER_DURATION`: durata in secondi per ogni punto di soglia; nel legacy valeva `2`.
- `MAPMT_DEFAULT_THRESHOLD`: soglia usata per configurazione/rate scan; nel legacy `230`.
- `MAPMT_DEFAULT_GAIN`: gain uniforme di default; nel legacy `64`.
- `MAPMT_PEDESTAL_START`, `MAPMT_PEDESTAL_STOP`, `MAPMT_PEDESTAL_GAIN`: scan piedistallo.
- `MAPMT_DARK_START`, `MAPMT_DARK_STOP`, `MAPMT_DARK_GAIN`: scan dark rate.
- `MAPMT_RATE_RUNS`, `MAPMT_RATE_FIXED_THRESHOLD`, `MAPMT_RATE_GAIN`: rate scan a soglia fissa.
- `MAPMT_TDC_*`: parametri per `ssptest_TDCAll`.

`gain=0` mantiene il comportamento legacy: usare la gain map caricata dalla configurazione.

## Relazione con i file di calibrazione offline

Gli script DAQ producono file tipo:

- `data/ped/rich_pedestal_YYYYMMDD_HHMMSS.txt`
- `data/ped/rich_dark_YYYYMMDD_HHMMSS.txt`
- `data/ped/rich_rate_YYYYMMDD_HHMMSS.txt`
- `data/tdc/ssprich_tdc_*.bin`

I file scaler in `data/ped` possono essere analizzati con:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" pedestal \
  --input "$MAPMT_SUITE/data/ped/rich_pedestal_YYYYMMDD_HHMMSS.txt" \
  --config "$MAPMT_SUITE/cnf/detector.conf" \
  --map "$MAPMT_SUITE/cnf/maps/fiber.map" \
  --thresholds "$MAPMT_SUITE/cnf/maps/threshold.txt" \
  --gains "$MAPMT_SUITE/cnf/maps/gain.txt" \
  --output "$MAPMT_SUITE/results/pedestal_YYYYMMDD_HHMMSS"
```

Il binario TDC e l'output corretto della DAQ. La decodifica resta nella catena
esterna esistente; il comando offline `time` usa il file TDC gia decodificato.
