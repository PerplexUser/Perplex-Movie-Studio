# Perplex Movie Studio 1.0.0 für macOS

Die Mac-Ausgabe verwendet denselben Anwendungscode wie Windows und Linux.
Version 1.0.0 korrigiert die Texteingabe und ergänzt die Deckkraft je Clip.

## Pakete und Installation

Es werden getrennte native Pakete für Apple Silicon (`arm64`, M1 und neuer) und
Intel (`x86_64`) erstellt. Zielsystem ist macOS 12 oder neuer. Der Build prüft die
Mindestversion aller eingebetteten Binärdateien. Eine Betriebssystemmatrix mit
allen älteren macOS-Versionen ist damit nicht ersetzt.

Das passende DMG öffnen und **Perplex Movie Studio.app** nach **Programme** ziehen.
Die ZIP-Ausgabe enthält dieselbe App sowie Hinweise und Prüfsummen. Qt, FFmpeg
und LAME sind enthalten; Endanwender benötigen weder Homebrew noch Python noch Qt.
Projektdateien mit `.pmsproj` lassen sich auch über Finder öffnen. Die üblichen
Tastenkürzel verwenden auf dem Mac die Befehlstaste ⌘.

Die Pakete sind ad-hoc signiert, **nicht mit einer
Apple Developer ID signiert und nicht notarisiert**. Eine Warnung von Gatekeeper
ist daher möglich. Für eine öffentliche, ohne solche Warnungen installierbare
Ausgabe sind Developer-ID-Signierung und Apple-Notarisierung zusätzlich nötig.
Die Systemsicherheit nicht pauschal deaktivieren. Apple-Zugangsdaten oder private
Zertifikatschlüssel gehören nicht in den Quellcode oder Chat.

## Build auf einem Mac

Voraussetzungen: Xcode Command Line Tools, Python 3.12, Qt 6.8.3 macOS, CMake 3.31
und Ninja. Der Build läuft nativ; für beide Ausgaben werden ein ARM-Mac und ein
Intel-Mac verwendet. Die Build-Verzeichnisse dürfen keine Leerzeichen enthalten.
Der Start aus einem verschobenen App-Verzeichnis mit Leerzeichen wird getestet.

```sh
python3 -m pip install aqtinstall==3.2.1 cmake==3.31.6 ninja==1.11.1.1
python3 -m aqt install-qt mac desktop 6.8.3 clang_64 --modules qtmultimedia --archives qtbase qttools qttranslations qtdeclarative --outputdir "$PWD/Qt"
export QT_ROOT_DIR="$PWD/Qt/6.8.3/macos"
bash packaging/build_macos.sh
```

Der Build prüft die mitgelieferten Quellen, baut die LGPL-Medienkomponenten,
übersetzt die Anwendung, bündelt die Bibliotheken, korrigiert Bibliothekspfade,
prüft Architektur und Mindestversion, signiert und führt native Tests aus.
ZIP und DMG entstehen **nur nach erfolgreichen Tests** in `dist/`.
Für einen Wiederholungsbuild ein frisches Checkout verwenden.

## GitHub Actions

`.github/workflows/macos.yml` nutzt `macos-15` für Apple Silicon und
`macos-15-intel` für Intel. Er wird auf dem Build-Zweig `codex/release-1.0.0`
bei Codeänderungen oder manuell gestartet. Er veröffentlicht keine GitHub Release.
Nach einem erfolgreichen Lauf enthalten die Artefakte die App-ZIP, das DMG und
das vollständige Source-ZIP. GitHub-Artefakte werden nach 30 Tagen gelöscht;
für dauerhafte Bereitstellung die geprüften Downloads auf die eigene Website
oder in eine gesondert freigegebene Release übernehmen.

Die in GitHub aufgeteilte Qt-Quelldatei wird bei Bedarf zusammengesetzt und
gegen dieselbe SHA-256-Prüfsumme wie im vollständigen Original-ZIP geprüft.
Das erzeugte Source-ZIP enthält die vollständige Datei, keine Teilarchive.
Die zusätzliche Qt-Declarative-Quelle für die eingebetteten QML-/Quick-Bibliotheken
wird beim Build anhand festgelegter Größe und SHA-256 geladen und ebenfalls
vollständig in das Source-ZIP aufgenommen.

## Prüfung und Vertrieb

`verification.json` enthält die tatsächlich gelaufenen Tests und die Hashes der
geprüften App-Binärdateien. `dependency-check.json` nennt Bibliotheken,
Architekturen, Mindestversionen und Signierungsstatus. Vorschau, MP4/MKV/MP3/WAV,
Farbkorrekturen, Texte, Bilder, Übergänge, Hochkant und Projektbearbeitung werden
automatisiert geprüft. Sichtbare Fensterbedienung, Lautsprecherausgabe,
Leistung auf Benutzerhardware und Finder/Gatekeeper-Verhalten müssen zusätzlich
auf den Ziel-Macs abgenommen werden.

Lizenztexte stehen im Paket und unter `Perplex Movie Studio.app/Contents/Resources`.
Beim Verteilen die Hinweise und das passende vollständige Source-ZIP mit anbieten.
Mit `PMS_CODESIGN_IDENTITY` kann beim eigenen Build eine lokal installierte
Developer-ID-Identität gewählt werden; die Notarisierung folgt separat über
Apples `notarytool` und `stapler`. Ohne bestätigte Apple-Prüfung keine Notarisierung
behaupten. Der Über-Dialog zeigt Version 1.0.0 ohne Entwicklungshinweis.
