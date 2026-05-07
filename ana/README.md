# Analisi

Questa cartella contiene tutto il software offline C++/ROOT:

- `src/`: implementazione;
- `include/`: header pubblici;
- `docs/`: note tecniche su file legacy e configurazione;
- `CMakeLists.txt`: build del comando `mapmt_calibrate`.

Build:

```bash
cmake -S . -B build
cmake --build build
```

Esempio dalla radice della suite:

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
"$MAPMT_SUITE/ana/build/mapmt_calibrate" inspect-map \
  --config "$MAPMT_SUITE/cnf/detector.conf"
```
