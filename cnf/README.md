# Configurazione

Questa cartella contiene tutti i file necessari per interpretare i canali e
calibrare un setup MAPMT.

- `detector.conf`: costanti generali dell'elettronica e convenzioni di decodifica.
- `maps/`: mappe hardware, soglie, gain e piedistalli medi.

I file `maps/setup.txt`, `maps/fiber.map`, `maps/threshold.txt`, `maps/gain.txt`
sono la configurazione di default, copiata dal setup legacy `TwoModules`.

Per un nuovo progetto, il file piu importante da riscrivere e `maps/fiber.map`.
