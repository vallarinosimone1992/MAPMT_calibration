# File di configurazione

Ci sono due famiglie di configurazione:

1. parametri DAQ per eseguire `ssptest_*`;
2. mappe/calibrazioni offline per interpretare i canali e produrre soglie, gain e offset.

Le copie locali dei file legacy sono in `cnf/maps/`. I file al primo livello
(`cnf/maps/fiber.map`, `cnf/maps/setup.txt`,
`cnf/maps/threshold.txt`, `cnf/maps/gain.txt`) sono la
configurazione di default e non richiedono path verso `clas_RICH_software`.

`MAPMT_SUITE` e la radice della suite, equivalente nuovo di `RICH_SUITE`.

## 1. Configurazione DAQ: `daq/daq.env`

Questo file non viene usato dall'analisi C++/ROOT. Serve solo agli script in
`daq/`.

Parametri essenziali:

- `MAPMT_DAQ_EXEDIR`: percorso degli eseguibili `ssptest_ConfigureAll`, `ssptest_ScalerAll`, `ssptest_TDCAll`.
- `MAPMT_SCALER_DURATION`: tempo di conteggio per ogni punto di soglia.
- `MAPMT_Q`: argomento `Q` di `ssptest_ScalerAll`; nel legacy era `0`.
- `MAPMT_DEFAULT_THRESHOLD`: soglia usata da `configure_all.sh` e da `rate_scan.sh`.
- `MAPMT_DEFAULT_GAIN`: gain uniforme, se non si usa la mappa.
- `MAPMT_DEFAULT_SUPPRESS`: terzo argomento di `ssptest_ConfigureAll`; nel legacy era `0`.
- `MAPMT_PEDESTAL_START/STOP/GAIN`: range dello scan piedistallo.
- `MAPMT_DARK_START/STOP/GAIN`: range dello scan dark.
- `MAPMT_RATE_RUNS`, `MAPMT_RATE_FIXED_THRESHOLD`, `MAPMT_RATE_GAIN`: rate scan a soglia fissa.
- `MAPMT_TDC_THRESHOLD`, `MAPMT_TDC_EVENTS`, `MAPMT_TDC_MODE`: argomenti di `ssptest_TDCAll`.

Nel legacy `gain=0` significa "usa la gain map caricata dalla configurazione"; `gain=64`
significa gain uniforme 64.

## 2. `setup.txt`

Prodotto da `ssptest_ConfigureAll` e usato per sapere quali fibre/ASIC sono stati visti dalla DAQ.

Il formato legacy ha:

- 23 righe iniziali di header;
- poi righe `slot fiber nasics`.

Esempio:

```text
3 0 3
3 1 3
3 2 0
```

Nel vecchio codice le prime 23 righe sono scartate sempre. Nel nuovo software il numero si cambia con:

```text
setup_header_lines = 23
```

in `cnf/detector.conf`.

## 3. `fiber.map`

E la mappa piu importante. Associa elettronica e geometria:

```text
slot fiber asic module pmt tile
```

Esempio:

```text
3 1 0 1 1 1
3 1 1 1 2 1
3 1 2 1 3 1
```

Significa che nello slot 3, fibra 1, ASIC 0 si trova il PMT 1 del modulo 1, tile 1.

Per un nuovo progetto devi ricostruire questo file con:

- slot SSP reali;
- fibre collegate;
- ASIC presenti per fibra;
- numero modulo;
- numero PMT;
- tile/gruppo meccanico se utile.

Se questo file e sbagliato, tutta la calibrazione viene assegnata al PMT/canale sbagliato.

## 4. `threshold.txt`

Formato:

```text
slot fiber asic threshold_dac
```

Esempio:

```text
3 1 0 210
3 1 1 214
```

Nel flusso pedestal si genera come:

```text
threshold = ceil(media_piedistallo_ASIC) + offset
```

con offset tipico `25`.

## 5. `gain.txt`

Formato:

```text
slot fiber channel gain
```

dove `channel` va da 0 a 191:

```text
asic = channel / 64
maroc = channel % 64
```

Esempio:

```text
3 0 0 64
3 0 1 64
3 0 64 70
```

Se manca, il nuovo software usa `default_gain`.

## 6. `PmtPedestal.txt` / `chip_pedestals.txt`

Formato:

```text
slot fiber asic module pmt mean rms
```

E il riassunto per ASIC/PMT usato per generare `threshold.txt`.

## 7. `cnf/detector.conf`

Questo file contiene costanti dell'elettronica e convenzioni di decodifica. I parametri da controllare in un nuovo setup sono:

- `slot_base`: nella formula legacy `absChannel = maroc + 64*asic + 192*fiber + 192*32*(slot-slot_base)`. Per CLAS12 era `3`.
- `clock_hz`: clock reference scaler, legacy `125000000`.
- `max_fibers`, `n_asics`, `n_pixels`: struttura dell'elettronica.
- `threshold_min`, `threshold_max`: range atteso per gli scan.
- `flat_rate_min`, `flat_rate_max`: regione usata per stimare il dark rate in plateau.
- `shoulder_range`: regione vicino alla soglia.
- `noisy_pedestal_rms`: taglio per canali rumorosi.

La mappa MAROC-anodo Hamamatsu e ancora quella legacy CLAS12. Se il nuovo cablaggio o il modello MAPMT cambia, va aggiornata nel codice o resa configurabile.
