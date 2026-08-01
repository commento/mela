# Mela

Primo prototipo JUCE di un loop editor touch per Raspberry Pi 4/5 e macOS.

## Funzioni attuali

- un loop modificabile alla volta;
- caricamento WAV, AIFF, FLAC e OGG (in base ai formati JUCE disponibili);
- forma d'onda con maniglie touch per scegliere inizio e fine;
- zoom con pinch a due dita, pan con trascinamento e reset con doppio tap;
- riproduzione continua oppure singola;
- velocità regolabile da 0,25× a 1,5× (con variazione dell'intonazione);
- inviluppo ADSR: fade-in con Attack e fade-out allo Stop con Release;
- modalita `ADSR CICLICO`, visualizzata sopra la forma d'onda e ripetuta a ogni giro;
- manopole touch: trascinamento verso l'alto per aumentare e verso il basso per diminuire;
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

## Prossimi passi

1. selezione del dispositivo audio e della dimensione buffer nella UI;
2. caricamento dei file su thread dedicato;
3. griglia temporale e aggancio opzionale degli estremi;
4. time-stretch opzionale per cambiare velocità senza cambiare intonazione;
5. salvataggio e ripristino delle sessioni.
