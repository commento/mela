# Mela

Primo prototipo JUCE di un loop editor touch per Raspberry Pi 4/5 e macOS.

## Funzioni attuali

- quattro slot indipendenti e riproducibili contemporaneamente;
- pagina AUDIO iniziale per scegliere ingresso, uscita, sample rate e buffer;
- pagina WIFI con libreria Mela Inbox per importare MP3, WAV e gli altri formati supportati;
- pagina RETE separata per cercare le reti disponibili e connettere Mela alla rete di casa;
- password Wi-Fi protetta, stato della connessione e IP locale per Pi Connect;
- tastiera Wi-Fi completa a schermo, utilizzabile senza tastiera fisica;
- registrazione diretta nello slot attivo dal microfono integrato o da un ingresso USB;
- registrazioni WAV a 24 bit salvate in `Musica/Mela Recordings` e caricate automaticamente;
- pulsante `ELIMINA SAMPLE`: svuota lo slot e, per le registrazioni fatte da Mela,
  cancella anche il relativo WAV dopo una conferma;
- ingresso non monitorato sulle casse, per evitare feedback durante la registrazione;
- editor contestuale per modificare un sample alla volta;
- caricamento WAV, AIFF, FLAC, OGG e MP3;
- forma d'onda con maniglie touch per scegliere inizio e fine;
- zoom con pinch a due dita, pan con trascinamento e reset con doppio tap;
- riproduzione continua oppure singola;
- reverse indipendente per ogni loop;
- velocità regolabile da 0,25× a 1,5× (con variazione dell'intonazione);
- modalità `STRETCH` per cambiare velocità senza cambiare intonazione;
- pitch shifting indipendente da -12 a +12 semitoni quando `STRETCH` è attivo;
- inviluppo ADSR: fade-in con Attack e fade-out allo Stop con Release;
- modalita `ADSR CICLICO`, visualizzata sopra la forma d'onda e ripetuta a ogni giro;
- manopole touch: trascinamento verso l'alto per aumentare e verso il basso per diminuire;
- doppio tap su una manopola per ripristinarne il valore predefinito;
- pagina FX con rack insert indipendente per ciascuno dei quattro sample;
- equalizzatore LOW/MID/HIGH indipendente per ogni sample e per il master;
- Distorsione, Granulare, Flanger e Chorus configurabili separatamente per slot;
- mandate Delay Send e Reverb Send per slot verso due effetti master condivisi;
- selettori S1-S4 e MASTER, indicatore DSP/XRUN e limite ECO di 32 grani complessivi;
- sintesi granulare real-time con Size, Density, Position, Pitch e Mix;
- bypass e mix indipendenti per ogni effetto;
- limiter master unico per proteggere il mix finale dal clipping;
- pagina KEYS con tastiera multitouch a due ottave e otto voci polifoniche;
- pagina SCENE con otto snapshot rinominabili: salva i quattro sample, loop, ADSR,
  tastiera, rack per slot ed effetti master;
- autosave JSON ripristinato all'avvio e gestione non bloccante dei sample mancanti;
- root note, cambio ottava e modalita Gate, One Shot o Loop per ogni sample;
- ADSR dedicato alle note della tastiera, separato dall'inviluppo dei loop;
- breve crossfade al punto di loop per ridurre i click;
- conversione della frequenza di campionamento tramite interpolazione lineare;
- volume;
- controlli grandi per display touch 1920×1200.
- multitouch Linux diretto fino a 10 punti per pinch della waveform e tastiera polifonica.
- skin cartoon originale anni '90 con palette ad alto contrasto, bordi illustrati e
  font Luckiest Guy incorporato nell'eseguibile;
- splash screen cartoon 1920×1200 coerente con la nuova interfaccia;
- tasto `POWER` con conferma touch per salvare lo stato e spegnere o riavviare
  correttamente il Raspberry Pi.

I file sono caricati in RAM. Un loop stereo di 10 secondi a 48 kHz occupa circa
3,84 MB in formato float, quindi questa strategia è appropriata per il primo
prototipo. Lo streaming da disco verrà aggiunto in seguito per tracce molto più
lunghe o per librerie con molti loop.

## Build macOS

Servono Xcode Command Line Tools, Git e CMake 3.24 o successivo.

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

## Connessione alla rete Wi-Fi di casa

Sul Raspberry Pi aprire la pagina `RETE`, premere `CERCA RETI`, scegliere la rete
di casa, inserire la password e premere `CONNETTI`. La scansione e la connessione
avvengono in background e non bloccano il motore audio. Quando la pagina mostra
`CONNESSO`, visualizza anche l'IP locale e la rete e' pronta per Pi Connect, se il
relativo servizio e' gia' installato e associato all'account.

La configurazione usa NetworkManager tramite `nmcli`, incluso in Raspberry Pi OS
Bookworm. Su macOS la pagina e' disponibile come anteprima ma non modifica la rete.

## Caricamento sample nella Mela Inbox

La pagina `WIFI` continua a mostrare l'indirizzo e il PIN del servizio di upload.
Dal telefono o dal computer, sulla stessa rete di Mela:

1. aprire l'indirizzo mostrato, normalmente `http://nome-raspberry.local:8080`;
2. inserire il PIN a sei cifre;
3. caricare un file WAV, AIFF, FLAC, OGG o MP3;
4. aggiornare la libreria e scegliere `CARICA IN S1-S4`.

Sul Raspberry Pi il servizio si installa o aggiorna con:

```sh
sudo ./deploy/raspberry-pi/install-upload-service.sh
```

I file ricevuti vengono conservati in `~/Music/Mela Inbox`. Dalla pagina `WIFI`
possono essere caricati negli slot oppure eliminati con conferma.

## Avvio kiosk su Raspberry Pi

Per il dispositivo finale e' consigliato Raspberry Pi OS Lite 64 bit: Mela usa
direttamente Xorg senza desktop environment o window manager. Dopo avere creato
la build Release e installato il servizio Inbox:

```sh
cmake -S . -B build-pi -DCMAKE_BUILD_TYPE=Release
cmake --build build-pi --parallel 3
sudo ./deploy/raspberry-pi/install-upload-service.sh
sudo ./deploy/raspberry-pi/install-kiosk-service.sh
sudo systemctl reboot
```

Lo script copia Mela in `/opt/mela/bin/Mela`, disabilita il display manager per
il riavvio successivo, abilita `mela-kiosk.service` e concede all'utente Mela i soli
comandi privilegiati di spegnimento e riavvio. Mela occupa tutto lo schermo,
imposta il touch a 1920x1200, nasconde il cursore e viene riavviata
automaticamente se termina. La sessione
corrente non viene chiusa durante l'installazione; dopo il riavvio la manutenzione
puo' essere effettuata via SSH.

Il mode video puo' essere cambiato senza modificare gli script impostando
`MELA_DISPLAY_MODE` nel servizio. Se il display non espone il mode richiesto,
Mela usa quello corrente e stampa l'elenco dei mode disponibili nel journal.

Comandi utili:

```sh
systemctl status mela-kiosk.service
journalctl -u mela-kiosk.service -f
sudo systemctl restart mela-kiosk.service
```

Per ripristinare il target e il display manager precedenti:

```sh
sudo ./deploy/raspberry-pi/restore-desktop.sh
sudo systemctl reboot
```

Durante lo sviluppo Linux si puo' evitare il fullscreen avviando `Mela --windowed`.

## Prossimi passi

1. indicatore del livello di ingresso e regolazione del gain di registrazione;
2. caricamento dei file su thread dedicato;
3. griglia temporale e aggancio opzionale degli estremi;
4. salvataggio e ripristino delle sessioni.
