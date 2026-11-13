# Perplex Movie Studio — Kurzanleitung

**0.1.1-dev · Revision 2**

SCHNELLSTART

Video, Audio, JPG oder PNG importieren. Doppelklick zum Anhängen oder auf eine Spur ziehen. Ungenutzte Medien mit „Aus Projekt entfernen“ löschen; Quelldateien bleiben erhalten.

Über der Vorschau Querformat 16:9 oder Hochformat 9:16 wählen. Smartphone-Drehmetadaten werden automatisch angewendet. Clips zum Verschieben und ihre Kanten zum Kürzen ziehen; Strg+B teilt am Abspielkopf. Der Reiter „Clip“ enthält Beginn, Dauer, Quellbereich, Lautstärke und Tonblenden.

Im Reiter „Bild“ Helligkeit, Kontrast, Sättigung und Farbton ändern. Überblenden, Wischen oder Schieben mit Ein-/Ausblenddauer wählen. Für Übergänge zwischen Clips diese auf verschiedenen Videospuren überlappen. Ohne darunterliegenden Clip erscheint Schwarz.

„+ Text“ anklicken. Inhalt, Anzeigedauer, Schrift, Größe, Farbe und Position einstellen. Zum Platzieren in die vorbereitete Vorschau klicken oder ziehen. Bildübergänge gelten auch für Text und Bilder. T1 liegt über V2, V2 über V1. M schaltet Spurton stumm; L sperrt Bearbeitungen.

„Vorbereiten und abspielen“ erstellt lokal eine 720p-Vorschau mit der Exportkomposition. Nach Änderungen erneut vorbereiten. „Bild vor“ erstellt auch eine Standbildvorschau. Lineal oder Schieberegler zum Suchen verwenden.

Als .pmsproj speichern. Ältere Projekte lassen sich öffnen; Speichern schreibt Schema 2 mit Bildeinstellungen. Ältere Anwendungsversionen können Schema 2 nicht öffnen. Quelldateien werden niemals verändert.

MP4/MKV mit Software-MPEG-4 Part 2 und AAC oder MP3/WAV-Audio exportieren. Quer- oder Hochformat mit 720p oder 1080p wählen. Die gesamte Timeline einschließlich Lücken wird exportiert. Vorhandene Zieldateien werden erst nach Erfolg ersetzt.

TASTENKÜRZEL
Strg+N/O/S: Neu/Öffnen/Speichern · Strg+Umschalt+S: Speichern unter
Strg+Z/Y: Rückgängig/Wiederholen · Strg+B: Teilen · Strg+D: Duplizieren
Strg+X/C/V: Ausschneiden/Kopieren/Einfügen · Entf: Löschen
Leertaste: Abspielen/Pause · Links/Rechts: Bild zurück/vor
Strg+Mausrad: Zoom · Umschalt+Mausrad: Scrollen

ENTWICKLUNGSVERSION
Windows-Abnahme und Leistungstests stehen noch aus. Der Software-Bildpfad stellt korrekte Farben vor Geschwindigkeit. Keine Konten, Telemetrie oder Uploads. Die vollständige Dokumentation liegt in docs.


## Text und Bilder genauer einstellen

„+ Text“ erzeugt zunächst einen fünf Sekunden langen Text auf T1 am Abspielkopf. Im Inspektor unter „Text“ Inhalt, Anzeigedauer, Schrift, Größe, Farbe, Breite und Position bearbeiten. X/Y bezeichnen die Mitte des Textblocks in Prozent des Projektbildes. Bei 50/50 liegt die Mitte im Zentrum. Große Texte am Bildrand können abgeschnitten werden. Schriften müssen lokal installiert sein.

Zum Platzieren zunächst eine Vorschau vorbereiten und den Textclip auswählen. Ein Klick bzw. Loslassen nach dem Ziehen setzt seine Mitte; die Vorschau wird danach neu vorbereitet. Zahlenfelder und Textänderungen übernehmen ihre Werte bei Abschluss der Eingabe; danach Vorschau bei Bedarf erneut vorbereiten. Unter „Bild“ die Ein-/Ausblendzeiten setzen; diese wirken ebenso auf Texte und Bilder. Die Ein-/Ausblendzeiten müssen in den verfügbaren Zeitraum passen.

JPG/PNG werden zunächst fünf Sekunden lang eingefügt. Die Dauer im Reiter „Clip“ oder durch Ziehen der Clipkante ändern. PNG-Transparenz wird beibehalten. Obergrenzen beim Import: 8192 Pixel pro Seite, 32 Megapixel, 128 MiB Datei. Bilder werden ohne Verzerrung eingepasst.

## Übergänge

Beispiel: Clip A auf V1 von Sekunde 0 bis 6, Clip B auf V2 von Sekunde 5 bis 10. Bei B unter „Bild“ eine Einblenddauer von 1 Sekunde wählen und „Überblenden“, „Wischen“ oder „Schieben“ einstellen. Im Überlappungsbereich wird A durch B ersetzt. Die Ausblendung lässt den darunterliegenden Clip wieder sichtbar werden. Auf derselben Spur dürfen Clips nicht überlappen.

## Hochformat

Über der Vorschau „Hochformat 9:16“ wählen. Beim Export dieselbe Ausrichtung wählen: 720p ergibt 720×1280, 1080p ergibt 1080×1920. Drehmetadaten des Smartphones werden berücksichtigt. Querformatmaterial in einem Hochformatprojekt wird eingepasst und erhält Balken; ein freier Zuschnitt ist nicht vorhanden.

## Speichern, Leistung und Fehlermeldungen

Beim Öffnen älterer Projekte am besten einmal unter neuem Namen speichern: Schema 2 kann von älteren Programmständen nicht gelesen werden. Die MP4-/MKV-Ausgabe verwendet Software-MPEG-4 Part 2 mit AAC; der Containername MP4 bedeutet nicht H.264. Manche Zielplattformen unterstützen diesen Codec eingeschränkter. Die Farbumwandlung arbeitet bewusst ohne CPU-Optimierungen; bei längeren Projekten kann die Vorbereitung dauern. Vorschau und Export verwenden temporären Speicherplatz.

Der Installer ist für den aktuellen Benutzer; die portable Ausgabe vollständig in einen eigenen Ordner entpacken. EXE, DLLs und Plugins zusammenlassen. In dieser Revision enthält ein fehlgeschlagenes Ersetzen der Zieldatei auch den konkreten Windows-Fehler. Eine bereits vorhandene Zieldatei bleibt bei Fehlern oder Abbruch erhalten. Originalmedien werden nie gelöscht oder überschrieben.
