# Mappe e calibrazioni locali

Questa cartella contiene copie locali dei file di configurazione/calibrazione
necessari, cosi `MAPMT_calibration` non dipende da `clas_RICH_software`.

## Default

I file al primo livello sono la configurazione di default, copiata da
`TwoModules`:

- `setup.txt`
- `fiber.map`
- `threshold.txt`
- `gain.txt`
- `PmtPedestal.txt`

Sono i file usati negli esempi del `README.md`.

## Configurazioni legacy archiviate

- `RICH1/`: mappa e soglie per RICH1.
- `RICH2/`: mappa, soglie, gain e `chip.txt` per RICH2.
- `TwoModules/`: configurazione completa a due moduli, usata come default.

## Quali file modificare per un nuovo progetto

Per rendere operativo un nuovo setup devi creare o aggiornare almeno:

- `fiber.map`: `slot fiber asic module pmt tile`;
- `setup.txt`: output coerente con `ssptest_ConfigureAll`, oppure file equivalente;
- `threshold.txt`: `slot fiber asic threshold_dac`;
- `gain.txt`: `slot fiber channel gain`, con `channel = 64*asic + maroc`.

`PmtPedestal.txt` non e obbligatorio per l'analisi di un nuovo run, ma e utile
come riferimento o per generare soglie iniziali.
