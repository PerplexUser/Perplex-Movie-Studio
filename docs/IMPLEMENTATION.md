# Umsetzung und Prüfnachweis

Stand: **0.1.1-dev, Revision 2 · 13. November 2026**.

## Umgesetzt

| Bereich | Verhalten | Prüfnachweis |
|---|---|---|
| Architektur/Commands | Qt-freie Domain, Mikrosekunden, atomare Bearbeitungen, Undo/Redo, Sperren | `domain_tests`, `service_tests` |
| Medien und Ton | Asynchroner Import und Dekodierprüfung, Mehrspur-Mix, Audio-Blenden | Importmatrix und Samplevergleiche in `service_tests` |
| Projektdateien | Schema 2, Migration aus Schema 1, neue Effekte persistiert, atomare Saves | Schema-/Roundtrip-/Fehlerprüfungen |
| Farbfehler | CPU-Optimierungen in beiden Medienpfaden deaktiviert, Software-Preview | Vollbild-Farbprüfungen in sechs Auflösungen bis 1080×1920; Export wird erneut importiert und exportiert |
| Exportdatei | Separates temporäres Verzeichnis; Ausgabe erst nach FFmpeg-Ende atomar übernehmen | Überschreiben, Verweigern, Fehler beim Übernehmen, Abbruch und Aufräumen |
| Text/Bilder | Unicode, Dauer, Position, Typografie, Alpha, Ein-/Ausblenden | Gerenderte Pixel an Zeitpunkten/Positionen, transparentes PNG, JPEG, mehrzeiliger Text |
| Bildkorrektur | Helligkeit, Kontrast, Sättigung, Farbton | Numerische Pixelvergleiche |
| Übergänge | Überblenden, Wischen, Schieben; Split erhält Verlauf | Gerenderte Frames vor/nach Split verglichen |
| Hochformat | Projekte/Vorschau/Export 9:16; Smartphone-Drehmetadaten | 720×1280 / 1080×1920 und rotierte Quelle |
| Oberfläche | Medien entfernen, neue Inspektor-Reiter, Text mit Maus platzieren, About | `editor_workflow`, `ui_smoke`, Screenshotkontrolle |

## Tatsächliche Testausführung

Alle **6 CTest-Gruppen** wurden unter Linux mit Qt 6.8.3 und dem mitgelieferten LGPL-FFmpeg-Build erfolgreich ausgeführt: `domain`, `services`, `ui_smoke`, `preview_playback`, `visual_features`, `editor_workflow` (die exakten Namen stehen im Log). Keine übersprungenen Featuretests. Dauer dieses Durchlaufs: 25,59 Sekunden. Logs liegen unter `docs/verification`.

Die Tests starten echte Probe-/Renderprozesse, dekodieren das Ergebnis und prüfen Pixel, Ton, Dauer, Streamdaten und Dateierhalt. Der GUI-Test importiert ein Video, erzeugt eine pausierte Vorschau, spielt/sucht, bearbeitet Text/Dauer/Farbeinstellungen, wechselt ins Hochformat und platziert Text per Maus. Der deutsche About-Text wird exakt verglichen. 197 eigene Übersetzungseinträge sind fertig; Platzhalter werden geprüft.

Die Windows-Anwendung und Windows-Testprogramme werden mit MinGW-w64/Qt 6.8.3 cross-kompiliert. Das Paket wird auf Architektur, DLL-Abhängigkeiten und importierte Symbole geprüft. Der Installer wird entpackt und seine Nutzdateien gegen das portable Paket verglichen. Diese statischen Prüfungen sind **kein Windows-Laufzeittest**.

## Noch offen

Start/Installation/Deinstallation und Dateisystemfehler auf sauberem Windows, tatsächliche Wiedergabe mit Audioausgabe, A/V-Synchronmarken, DPI/Mehrschirm-Bedienung und längere Leistungstests. Ein Audiogerät stand im Testsystem nicht zur Verfügung; Hörtests wurden nicht durchgeführt. Die skalar ausgeführten Pixeloperationen können langsamer sein. Eine stabile Freigabe wird nicht behauptet.
