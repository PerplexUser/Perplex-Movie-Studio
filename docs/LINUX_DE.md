# Perplex Movie Studio 1.0.0 für Linux

Die Linux-Ausgabe ist für Ubuntu ab 22.04 und darauf basierendes Linux Mint
ab 21 auf 64-Bit-PCs (x86_64/amd64) vorgesehen. Der native Paketbuild läuft
unter Ubuntu 22.04. Die mitgelieferte `verification.json` nennt das tatsächlich
geprüfte System; sie ist keine Abnahme jeder Mint-/Ubuntu-Version oder Grafikkarte.

## Installation

Das DEB-Paket im Downloadordner über die Paketverwaltung öffnen. Alternativ:

```sh
sudo apt install ./PerplexMovieStudio-1.0.0-Linux-amd64.deb
```

Danach im Anwendungsmenü **Perplex Movie Studio** starten. Die Anwendung enthält
Qt, FFmpeg, LAME und ICU. Die Paketverwaltung installiert fehlende Systembibliotheken;
für den Betrieb sind weder Python noch ein Qt-SDK erforderlich. Zum Entfernen:

```sh
sudo apt remove perplex-movie-studio
```

Eigene Projekte und Medien werden dadurch nicht gelöscht.

## Portable Ausgabe

Das TAR.GZ vollständig in einen eigenen Ordner entpacken und darin
`./PerplexMovieStudio` starten. `bin`, `lib`, `plugins`, Übersetzungen und Lizenzen
zusammenlassen. Die benötigten Systempakete entsprechen den Abhängigkeiten des
DEB-Pakets. Der Starter funktioniert auch aus einem Ordner mit Leerzeichen.

Die Oberfläche verwendet X11 bzw. XWayland in einer Wayland-Sitzung. Die
automatischen Tests prüfen zusätzlich den X11-Start mit einem virtuellen Display.
Ein realer Bildschirm-, Mehrmonitor- und Lautsprechertest bleibt davon getrennt.

## Bedienung

Die erste Sprache ist Englisch; unter **Settings → Language → Deutsch** umstellen.
Die gemeinsame [Kurzanleitung](USER_GUIDE_DE.md) gilt für alle Plattformen.
Für durchsichtige Überlagerungen den oberen Clip auswählen und unter
**Bild → Deckkraft (%)** beispielsweise 50 % einstellen. Die darunterliegende
Videospur muss im selben Zeitabschnitt belegt sein. Nach Änderungen die Vorschau
erneut vorbereiten.

## Selbst bauen

Ein frisches Checkout, GCC 12, GNU make/patch/tar, CMake 3.31.6, Ninja,
Python 3.12, patchelf, Xvfb und Qt 6.8.3 `linux_gcc_64` verwenden. Die benötigten
Qt-Archive sind qtbase, qttools, qttranslations, qtdeclarative, icu und qtmultimedia.
Die vollständige Befehlsfolge einschließlich Systempaketen steht in
`.github/workflows/linux.yml`. Mit gesetztem `QT_ROOT_DIR`:

```sh
bash packaging/build_linux.sh
```

Die Ausgabe unter `dist/` entsteht nach erfolgreicher Prüfung des gebündelten
Programms. Den vollständigen Source-ZIP und die enthaltenen Lizenzhinweise beim
Anbieten der Binärpakete mit bereitstellen. Die Anwendung nutzt auch unter Linux
den bestehenden Software-Bildpfad; Echtzeitwiedergabe wird nicht zugesichert.
