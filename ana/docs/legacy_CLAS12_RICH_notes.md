# Note sul software CLAS12 RICH MAPMT

Questa sintesi deriva dai file in `clas_RICH_software/rich_sw`.

## Flusso legacy

1. Sul crate RICH si esegue la configurazione CODA/SSP con `ssptest_ConfigureAll`.
2. Gli script `sw/daq/thr_scan.sh`, `rich_pedestal.sh`, `rich_dark.sh` e `rate_scan.sh` chiamano `ssptest_ScalerAll`.
3. L'output `ssprich_scaler_???.txt` viene concatenato in `rich_pedestal_YYYYMMDD_HHMMSS.txt`.
4. Fuori dal crate, `sw/ana/ped/recoScalers.sh` lancia ROOT con `richReco.c`.
5. `richReco.c` usa `richScalers.h`, che legge mappa hardware, soglie, gain e file scaler, poi produce `histo.root`, `stat.txt`, `chip.txt`, `rate.txt`, `hot.txt`, `dead.txt`, `noisy.txt` e PDF.

## File di configurazione importanti

- `maps/*/setup.txt`: prime 23 righe di header SSP, poi righe `slot fiber nasics`. Il codice legacy scarta sempre 23 righe.
- `maps/*/fiber.map`: righe `slot fiber asic module pmt tile`. E il file piu importante per trasportare il software a una geometria diversa.
- `maps/*/threshold.txt` o `threshold_25.txt`: righe `slot fiber asic threshold`.
- `maps/*/gain.txt`: righe `slot fiber channel gain`, dove `channel` e 0..191 e viene convertito in `asic=channel/64`, `maroc=channel%64`.
- `maps/*/PmtPedestal.txt`: media e RMS per ASIC/PMT, formato `slot fiber asic module pmt mean rms`.
- `sw/cnf/generate/thr/thrCalc.c`: genera soglie come `ceil(mean_pedestal_chip) + offset`.
- `sw/cnf/generate/thr/thrCalc_file_2x.c`: include il fattore ADC/TDC 2.0, commentato come 0.7 fC ADC e 1.4 fC TDC.
- `sw/cnf/generate/gain/gain.c`: trasforma una tabella canale-gain in registri CODA a gruppi da 16 canali.

## Piedistalli

Nel legacy il piedistallo non e una media diretta dei campioni ADC. Lo scan soglia produce, per ogni canale MAROC, un istogramma `rate vs threshold`; la media del piedistallo e calcolata come media pesata della soglia con peso uguale al rate. Il file `chip.txt` contiene poi la media dei 64 canali di un ASIC/PMT e viene usato per proporre le soglie.

Valori hard-coded nel legacy:

- clock scaler: `125E6`;
- threshold default: `230`;
- gain default: `64`;
- range piedistallo accettato: 150..220 DAC;
- canale rumoroso: RMS piedistallo > 4 DAC;
- regione flat dark rate: 400..425 DAC;
- shoulder vicino soglia: `threshold..threshold+25`;
- mappa MAROC-anodo Hamamatsu cablata nel costruttore di `richScalers`.

## Tempo/TDC

In questa copia del software non c'e una vera analisi di calibrazione temporale TDC. Il solo file dedicato e `sw/daq/rich_tdc.sh`, che chiama `ssptest_TDCAll` e rinomina `ssprich_tdc.bin` con parametri nel nome (`BL1`, frequenza, finestra, lookback). Non e presente un decoder documentato del binario.

Per questo la nuova suite separa il problema in due layer:

- acquisizione/decoder TDC specifico della DAQ;
- calibrazione tempo su hit gia decodificati in testo o CSV.

Il nuovo comando `time` lavora sul secondo layer e produce offset per canale rispetto a un canale di riferimento o alla media globale.

## Criticita nel passaggio a un nuovo progetto

- La decodifica `absChannel -> slot/fiber/asic/channel` assume `slot_base=3`. Se i nuovi slot partono da 13, 0 o altro, va cambiato.
- `NSECTORS=2`, `NPMTS=391`, `NTILES=138` erano dimensioni del RICH CLAS12; nel nuovo codice non sono richieste per l'analisi, ma restano implicite nei vecchi display.
- La geometria di display CLAS12 in `InitDisplay()` non e portabile. Il nuovo software conserva la calibrazione numerica, non la grafica a sagoma RICH.
- `setup.txt` non e autoesplicativo: e meglio considerare `fiber.map` come sorgente primaria della geometria attiva.
- I file di gain e soglia devono essere coerenti con la stessa mappa. Mismatch tra DAQ e `fiber.map` produce canali scartati o PMT sbagliati.
- Il formato TDC binario `ssprich_tdc.bin` resta fuori da questa suite: la
  calibrazione temporale richiede un formato decodificato esplicito.
