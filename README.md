# MAPMT_calibration

Nuova suite C++/ROOT per calibrare MAPMT letti con elettronica tipo CLAS12 RICH/MAROC, creata accanto a `clas_RICH_software` senza modificarlo.

La suite conserva i formati testuali utili del software legacy, ma rimuove molte assunzioni hard-coded dal codice di analisi. Usa `MAPMT_SUITE` come radice della suite, in analogia alla vecchia variabile `RICH_SUITE`.

E organizzata in tre aree principali:

- `ana/`: codice C++/ROOT, CLI `mapmt_calibrate` e documentazione tecnica dell'analisi.
- `daq/`: script DAQ per produrre i file di calibrazione con `ssptest_*`.
- `cnf/`: mappe, soglie, gain e costanti detector.

`data/` e `results/` restano al livello principale come aree operative.

## Build

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
cmake -S "$MAPMT_SUITE/ana" -B "$MAPMT_SUITE/ana/build"
cmake --build "$MAPMT_SUITE/ana/build"
```

Richiede ROOT con `root-config`/CMake package disponibile.

## Script DAQ

Gli script per produrre i file di calibrazione con `ssptest_*` sono in `daq/`.
Sono wrapper dei file legacy `sw/daq`, ma con parametri raccolti in `daq/daq.env`.

Sulla macchina DAQ:

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
cd "$MAPMT_SUITE/daq"
cp daq.env.example daq.env
vi daq.env
./configure_all.sh 230 64 0
./rich_pedestal.sh
./rich_dark.sh
./rate_scan.sh 100 0 230
```

Questi script richiedono CODA/SSP e connessione alla crate. L'analisi offline
`mapmt_calibrate` richiede invece solo ROOT e i file dati gia prodotti.

## Comandi principali

Ispezione mappa:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" inspect-map
```

Analisi piedistalli da file scaler:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" pedestal \
  --input rich_pedestal_YYYYMMDD_HHMMSS.txt \
  --output "$MAPMT_SUITE/results/pedestal_run"
```

Output principali:

- `channel_stats.csv`: statistiche per canale;
- `chip_pedestals.txt`: formato compatibile con il vecchio `thrCalc.c`;
- `thresholds_suggested.txt`: soglie `ceil(media_piedistallo_ASIC) + offset`;
- `noisy_channels.txt` e `dead_channels.txt`;
- `histo.root`: istogrammi ROOT `rate vs threshold` e TTree riassuntivo.

Generazione soglie da `chip_pedestals.txt`:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" thresholds \
  --chip-pedestals results/pedestal_run/chip_pedestals.txt \
  --output results/pedestal_run/threshold_25.txt \
  --offset 25
```

Calibrazione temporale da hit TDC gia decodificati:

```bash
"$MAPMT_SUITE/ana/build/mapmt_calibrate" time \
  --input decoded_tdc.csv \
  --output "$MAPMT_SUITE/results/time_run" \
  --format event-address \
  --reference 3:1:0:0
```

Formati TDC supportati:

- `abs`: `absChannel time [tot]`;
- `address`: `slot fiber asic maroc time [tot]`;
- `event-address`: `event slot fiber asic maroc time [tot]`;
- `auto`: inferenza dal numero di colonne.

I file binari `ssprich_tdc_*.bin` sono l'output atteso della DAQ. La loro
decodifica resta esterna a questa suite; `mapmt_calibrate time` parte dal file
TDC gia decodificato in uno dei formati testuali sopra.

Per dettagli su input, output e uso degli offset temporali vedi
`ana/docs/time_calibration.md`.

## File legacy da controllare prima della migrazione

Vedi `ana/docs/legacy_CLAS12_RICH_notes.md`. In breve, i file critici sono `fiber.map`, `setup.txt`, `threshold*.txt` e `gain.txt`. La variabile piu pericolosa e `slot_base`: nel legacy la formula degli absolute channel assume slot iniziale 3.

La suite contiene copie locali dei file legacy necessari in `cnf/maps/`, quindi non deve leggere nulla da `clas_RICH_software` per girare.

Per il significato dei parametri vedi anche `ana/docs/configuration_files.md`.
