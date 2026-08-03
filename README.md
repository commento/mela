# Mela

Primo prototipo JUCE di un loop editor touch per Raspberry Pi 4/5 e macOS.

## Funzioni attuali

- quattro slot indipendenti e riproducibili contemporaneamente;
- pagina AUDIO iniziale per scegliere ingresso, uscita, sample rate e buffer;
- pagina WIFI per importare negli slot i sample ricevuti da telefono o computer;
- registrazione diretta nello slot attivo dal microfono integrato o da un ingresso USB;
- registrazioni WAV a 24 bit salvate in `Musica/Mela Recordings` e caricate automaticamente;
- ingresso non monitorato sulle casse, per evitare feedback durante la registrazione;
- editor contestuale per modificare un sample alla volta;
- caricamento WAV, AIFF, FLAC e OGG (in base ai formati JUCE disponibili);
- forma d'onda con maniglie touch per scegliere inizio e fine;
- zoom con pinch a due dita, pan con trascinamento e reset con doppio tap;
- riproduzione continua oppure singola;
- reverse indipendente per ogni loop;
- velocità regolabile da 0,25× a 1,5× (con variazione dell'intonazione);
- inviluppo ADSR: fade-in con Attack e fade-out allo Stop con Release;
- modalita `ADSR CICLICO`, visualizzata sopra la forma d'onda e ripetuta a ogni giro;
- manopole touch: trascinamento verso l'alto per aumentare e verso il basso per diminuire;
- doppio tap su una manopola per ripristinarne il valore predefinito;
- pagina FX con rack insert indipendente per ciascuno dei quattro sample;
- Distorsione, Granulare, Flanger e Chorus configurabili separatamente per slot;
- mandate Delay Send e Reverb Send per slot verso due effetti master condivisi;
- selettori S1-S4 e MASTER, indicatore DSP/XRUN e limite ECO di 32 grani complessivi;
- sintesi granulare real-time con Size, Density, Position, Pitch e Mix;
- bypass e mix indipendenti per ogni effetto;
- limiter master unico per proteggere il mix finale dal clipping;
- pagina KEYS con tastiera multitouch a due ottave e otto voci polifoniche;
- root note, cambio ottava e modalita Gate, One Shot o Loop per ogni sample;
- ADSR dedicato alle note della tastiera, separato dall'inviluppo dei loop;
- breve crossfade al punto di loop per ridurre i click;
- conversione della frequenza di campionamento tramite interpolazione lineare;
- volume;
- controlli grandi per display touch 1280×800.

I file sono caricati in RAM. Un loop stereo di 10 secondi a 48 kHz occupa circa
3,84 MB in formato float, quindi questa strategia è appropriata per il primo
prototipo. Lo streaming da disco verrà aggiunto in seguito per tracce molto più
lunghe o per librerie con molti loop.

## Build macOS

Servono Xcode Command Line Tools, Git e CMake 3.22 o successivo.

```sh
cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Debug
cmake --build build-mac --parallel
open build-mac/Mela_artefacts/Debug/Mela.app
```

## Build Raspberry Pi OS 64 bit

Installare prima toolchain e dipendenze Linux richieste da JUCE. Poi:

```sh
cmake -S . -B build-pi -DCMAKE_BUILD_TYPE=Release
cmake --build build-pi --parallel 3
./build-pi/Mela_artefacts/Release/Mela
```

Su Raspberry Pi 4 partire con 48 kHz e buffer da 256 campioni; provare 128 solo
dopo avere verificato l'assenza di drop-out. Durante lo sviluppo su Pi 5 bisogna
anche provare periodicamente una build limitata e tenere bassi CPU e animazioni.

All'avvio aprire la pagina AUDIO, scegliere il microfono o l'ingresso della
scheda USB e l'uscita desiderata, quindi premere `APRI I SAMPLE`. Nella pagina
LOOP, `REC` registra nello slot selezionato e `STOP REC` chiude il WAV e lo rende
subito disponibile nell'editor della forma d'onda.

## Caricamento sample via Wi-Fi

### Test su macOS

Avviare il servizio in un terminale e lasciarlo aperto:

```sh
./wifi/run-mac.sh
```

Al primo avvio vengono installate le dipendenze Python in un ambiente isolato
dentro `wifi/mela_upload/.venv`. Avviare poi `Mela.app`, aprire la pagina `WIFI`
e visitare dal browser `http://localhost:8080`. Per provare da telefono, Mac e
telefono devono essere sulla stessa rete; usare l'indirizzo `.local` o l'IP
mostrato da Mela. macOS potrebbe chiedere di consentire le connessioni in entrata.

I file di test reali vengono salvati in `~/Music/Mela Inbox` e il PIN in
`~/.config/mela/upload-pin.txt`.

### Installazione su Raspberry Pi

Sul Raspberry Pi, dalla cartella del progetto, installare una volta il servizio:

```sh
sudo ./deploy/raspberry-pi/install-upload-service.sh
```

Lo script installa un servizio separato dal motore audio, lo avvia automaticamente
e pubblica la pagina sulla rete locale tramite Avahi. Nella pagina `WIFI` di Mela
sono mostrati l'indirizzo da aprire e il PIN a sei cifre. Dal telefono o dal Mac:

1. aprire l'indirizzo indicato, normalmente `http://nome-raspberry.local:8080`;
2. inserire il PIN mostrato da Mela;
3. inviare un file WAV, AIFF, FLAC o OGG;
4. scegliere il file nella `Mela Inbox` e premere `CARICA IN S1-S4`.

Gli upload sono limitati a 250 MB e vengono scritti prima come file temporanei.
Il sample compare nella libreria soltanto a trasferimento concluso. Il servizio
gira con priorita CPU e disco ridotta per non interferire con l'audio real-time.

Per controllarne lo stato sul Raspberry:

```sh
systemctl status mela-upload.service
```

## Prossimi passi

1. indicatore del livello di ingresso e regolazione del gain di registrazione;
2. caricamento dei file su thread dedicato;
3. griglia temporale e aggancio opzionale degli estremi;
4. time-stretch opzionale per cambiare velocità senza cambiare intonazione;
5. salvataggio e ripristino delle sessioni.
