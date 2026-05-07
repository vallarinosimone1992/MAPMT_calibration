# Calibrazione temporale

La calibrazione temporale del nuovo software lavora su hit TDC gia decodificati
in formato testo/CSV. Gli script DAQ producono correttamente file binari
`ssprich_tdc_*.bin`; la decodifica di quei binari resta parte della catena
esterna esistente e non viene duplicata in questa directory.

## Input accettati

Il comando legge righe numeriche, separate da spazi o virgole. Le righe vuote e
la parte dopo `#` sono ignorate.

Formati:

```text
absChannel time [tot]
slot fiber asic maroc time [tot]
event slot fiber asic maroc time [tot]
```

Le colonne opzionali dopo `time`, per esempio `tot`, sono ignorate. Con
`--format auto` il formato viene dedotto dal numero di colonne:

- 2 o 3 colonne: `abs`;
- 5 colonne: `address`;
- 6 o piu colonne: `event-address`.

`absChannel` usa la convenzione legacy:

```text
absChannel = maroc + 64*asic + 192*fiber + 192*32*(slot - slot_base)
```

Il parametro `slot_base` si trova in `cnf/detector.conf`.

## Comando

```bash
export MAPMT_SUITE=/path/to/MAPMT_calibration
"$MAPMT_SUITE/ana/build/mapmt_calibrate" time \
  --input "$MAPMT_SUITE/data/tdc/decoded_tdc.csv" \
  --output "$MAPMT_SUITE/results/time_run" \
  --format event-address \
  --reference 3:1:0:0
```

Opzioni principali:

- `--reference slot:fiber:asic:maroc`: canale usato come zero temporale.
- `--min-entries N`: minimo numero di hit per calibrare un canale, default `20`.
- `--bins N`: bin degli istogrammi temporali, default `400`.
- `--min-time T` e `--max-time T`: range manuale degli istogrammi.

Se `--reference` non viene dato, lo zero temporale e la media dei tempi medi di
tutti i canali con statistica sufficiente.

## Output

Il comando produce:

```text
time_offsets.csv
time_calibration.root
```

`time_offsets.csv` contiene una riga per canale:

```text
slot,fiber,asic,maroc,pixel,module,pmt,tile,entries,mean_time,sigma_time,offset
```

`offset` e calcolato come:

```text
offset = reference_mean_time - channel_mean_time
```

Quindi un tempo corretto puo essere costruito come:

```text
time_corrected = time_raw + offset
```

`time_calibration.root` contiene:

- un TTree `time_offsets` con le stesse grandezze del CSV;
- un istogramma `hTime_slot_fiber_asic_maroc` per ogni canale calibrato.

## Note operative

Il confine della suite e intenzionale: acquisizione binaria con `ssptest_TDCAll`,
decodifica esterna, calibrazione temporale offline su testo/CSV decodificato.
Per questa directory e sufficiente che il decoder produca almeno `slot fiber
asic maroc time` oppure `event slot fiber asic maroc time`.

La calibrazione attuale usa la media aritmetica della distribuzione temporale.
Per dati reali con code, rumore o multi-picco puo essere meglio sostituire la
media con un fit del picco principale o con una stima robusta in finestra.
