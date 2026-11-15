# Perplex Movie Studio – Pflichtenheft

Version 1.0 | 12.11.2026 | Entwurf zur Freigabe

Auftraggeber / Projektinitiator: Michael Lappenbusch · www.perplex.click
Referenz: Perplex_Movie_Studio_Anforderungsanalyse.md, Version 1.0

## 1. Verbindliche Umsetzungsvorgaben
Produktname exakt `Perplex Movie Studio`. Windows-Desktop, Open Source Freeware, eigener Code MIT. Erster Start Englisch (en-US), umschaltbar auf Deutsch (de-DE), persistent. Hauptfenster zeigt dauerhaft `© by www.perplex.click` in der Statusleiste; Hilfe/About zeigt Michael Lappenbusch, Website, Version, MIT-Lizenz und Drittanbieterhinweise. Kein Wasserzeichen im exportierten Medium. Keine Konten, Abos, Laufzeitlimits oder verpflichtenden Netzwerkzugriffe.

## 2. Technologie und Lizenz-Gate (A01–A02, A18)
Vorgeschlagener Stack: C++20, Qt 6 Widgets, CMake, MSVC 2022 x64, FFmpeg (libavformat, libavcodec, libavutil, libswscale, libswresample, libavfilter), Qt Test/CTest, Git. Versionen in Build-Konfiguration festschreiben. Qt Widgets bietet Dock-Panels und native Desktop-Interaktion; die Timeline wird als eigene QGraphicsView/QGraphicsScene-Komponente oder gleichwertige performante Zeichenfläche umgesetzt.

MIT gilt für den eigenen Code, nicht automatisch für Abhängigkeiten. Qt nur mit LGPL-kompatiblen Modulen dynamisch linken oder andere zulässige Lizenz verwenden. FFmpeg-LGPL-Build ohne GPL-/nonfree-Komponenten bevorzugen; insbesondere kein libx264 in diesem Build. FFmpeg-Build-Konfiguration, Lizenztexte, erforderliche Quellcodebereitstellung und Austauschbarkeit der Bibliotheken dokumentieren. Keine proprietären Fremd-Assets. Lizenzinventar (SBOM), THIRD_PARTY_NOTICES und Rechte an mitgelieferten Medien vor Release prüfen. H.264/AAC-Patent- und Vertriebsfragen separat klären; MIT ist keine Patentfreigabe. Offizielle Referenzen: https://ffmpeg.org/legal.html und https://www.qt.io/licensing/.

H.264-Export ist ein Ziel, kein durch FFmpeg-LGPL allein garantierter Encoder. Prototyp muss verfügbare Windows-Media-Foundation- oder Hardwareencoder prüfen. Falls keine geeignete H.264-Implementierung verfügbar ist, muss MP4/MKV mit einem dokumentierten, kompatiblen alternativen Encoder (z. B. MPEG-4 Part 2) möglich sein; Benutzer über Kompatibilität informieren. AAC-Encoder und MP3-Encoder (z. B. libmp3lame bei passender Lizenz) prüfen. Release blockieren, falls ein gefordertes Format nicht exportierbar ist oder Lizenzpflichten ungeklärt sind.

## 3. Architektur
Schichten: GUI → Application Services/Commands → Domain Model → Media Engine/Infrastructure. Domain Model kennt keine Qt-Widgets und keine FFmpeg-Pointer. FFmpeg-Ressourcen mittels RAII verwalten. GUI-Thread niemals für langlaufende Dekodierung oder Export blockieren; Worker mit begrenzten Queues, Cancellation und eindeutiger Besitzsemantik. Vorschau und Export verwenden dieselbe Kompositionslogik; Preview darf Auflösung reduzieren, nicht Schnittsemantik ändern.

| Modul | Verantwortung |
|---|---|
| app | Start, Einstellungen, Lokalisierung, Hauptfenster |
| domain | Project, Asset, Track, Clip, Effect, Keyframe, Validierung |
| commands | Undo/Redo, atomare Bearbeitungsoperationen |
| media | Probe, Decoder, Frame-/Audio-Cache, Resampling |
| timeline | Auswahl, Drag-and-drop, Snapping, Zeitlineal, Spurverwaltung |
| render | Frame-Komposition, Transformation, Audio-Mix, Preview/Export |
| persistence | Projekt-JSON, Migration, Relink, Recovery |
| export | Encoder-Auswahl, Muxing, Fortschritt, Abbruch |
| tests | Unit-, Integrations-, GUI- und Exporttests |

## 4. GUI-Spezifikation (A03–A04, A08, A10)
Dunkles, eigenständiges Design. Menü File/Edit/View/Settings/Help; links Medienbibliothek und Ressourcenregister, Mitte Player, rechts Inspector, unten Timeline über die Fensterbreite. Export-Schaltfläche oben rechts; Statusleiste mit Copyright. Panels skalierbar, Mindestfenstergröße vorgeschlagen 1024×768, DPI 100–200 %. Keine fremden Logos oder pixelgenauen Kopien.

```
+ File  Edit  View  Settings  Help                 Export +
| Media / Text / Transitions | Preview | Inspector        |
| Import Media               | Play    | Properties       |
+---------------------------------------------------------+
| Undo Redo Split Delete Snap           Timeline Zoom     |
| V2     [overlay]                                        |
| V1 [clip A] [clip B]                                     |
| A1 [music waveform]                                     |
| A2        [voice]                                       |
+ © by www.perplex.click                         Ready   +
```

File: New, Open, Save, Save As, Import Media, Exit. Edit: Undo, Redo, Cut, Copy, Paste, Delete, Split. View: Panel visibility, timeline zoom, fit. Settings: Language (English/Deutsch), cache location. Help: User Guide, About Perplex Movie Studio. Übersetzungen über Qt Linguist .ts/.qm; keine hartkodierten UI-Texte, fehlende Übersetzungen im Test melden. Sprachwechsel sofort oder nach bestätigtem Neustart ohne Verlust ungespeicherter Arbeit. Tastenkürzel: Ctrl+N/O/S/Shift+S, Ctrl+Z/Y/C/V, Delete, Space, Ctrl+B für Split; keine globalen Shortcuts während Texteingabe.

## 5. Projekt- und Medienmodell (A05, A07–A09)
Projektdatei `.pmsproj`, UTF-8 JSON, schemaVersion, UUID, project settings, assets, tracks, clips, effects. Quelldateien referenzieren, nicht einbetten. Relative Pfade bevorzugen, absolute Fallbacks; fehlende Quellen mit Relink-Dialog. Speichern via temporäre Datei und atomaren Austausch (QSaveFile); Save As verändert nur Projektpfad, niemals Originalmedien. Unbekannte neuere Schema-Versionen nicht stillschweigend überschreiben.

Zeit intern als ganzzahlige Mikrosekunden bzw. rationaler Zeitbasis; keine kumulierten Float-Zeitwerte. Framegrenzen aus Projekt-FPS rational berechnen. Clip besitzt ID, assetId, trackId, timelineStart, sourceIn, sourceOut, gain, mute, transform; Dauer = sourceOut − sourceIn bei normaler Geschwindigkeit. Intervalle halboffen [start,end). Assets enthalten Pfad, Container, Streams, Dauer, Rotation, Auflösung, FPS, Audioformat. Track besitzt Typ, Reihenfolge, mute, lock. Kein negativer Start, keine Null-Dauer, keine Überlappung auf derselben Spur in V1; Übergänge später als explizite Ausnahme. Höhere Videospur liegt visuell über tieferer. Audio aller nicht stummen Spuren wird gemischt. Quellton ist dem Videoclip zeitlich zugeordnet.

Import per Dateidialog und Drag-and-drop. Probe mit FFmpeg statt Endungsprüfung allein. Mindest-Testmatrix: AVI MPEG-4 Part 2/MP3; MPG/MPEG MPEG-1/2 mit MP2; MP4 H.264/AAC; MKV H.264/AAC und VP9/Opus; MP3 CBR/VBR; WAV PCM 16/24 Bit. Nur nach erfolgreichem Probe importieren; bei unbekanntem Codec verständliche Meldung. Mehrere Audiospuren: standardmäßig erste geeignete Audiospur; Auswahl weiterer Streams als Soll. VFR und nicht-null Start-PTS normalisieren; Rotation berücksichtigen. Keine DRM-Unterstützung.

## 6. Schnitt und Timeline (A07–A11)
Mindestumfang zwei Video- und zwei Audiospuren, weitere Spuren später. Drag-and-drop aus Bibliothek erzeugt Clip am Zielzeitpunkt. Clips verschieben mit Snapping an Playhead und Clipkanten (abschaltbar), trimmen durch Ziehen an Rändern, teilen an Playhead, löschen, duplizieren, kopieren/einfügen. Split nur innerhalb des Clips, beide Teilclips behalten korrekte Quellintervalle und Eigenschaften. Standardlöschung lässt Lücke; Ripple Delete als separate Aktion (Soll). Gesperrte Spuren dürfen nicht verändert werden. Undo/Redo pro abgeschlossener Aktion, mindestens 100 Schritte, Drag als ein Command. Änderungen markieren Projekt dirty; Schließen/Neues Projekt fragt Speichern/Verwerfen/Abbrechen.

Player: Play/Pause, Seek, Frame vor/zurück, aktuelle Zeit/Gesamtdauer, Lautstärke, Fit, Seitenverhältnis. Audio als Masterclock bei Wiedergabe, Video anhand PTS synchronisieren; Seek flushes Decoder und verwirft veraltete Frames. Schwarzes Bild in Lücken. Audio auf 48 kHz Stereo mischen, Gain und Fades anwenden, Clipping durch dokumentierte Begrenzung vermeiden. Audio-only Projekte zulassen; Videoexport daraus mit schwarzem Bild. Stummes Projekt erhält beim Videoexport eine stille Audiospur.

## 7. Export (A06, A12)
| Ausgabe | Container | Zielcodec | Audio |
|---|---|---|---|
| MP4 | MP4 | H.264 bevorzugt, MPEG-4 Part 2 Fallback | AAC, 48 kHz Stereo |
| MKV | Matroska | H.264 bevorzugt, MPEG-4 Part 2 Fallback | AAC, 48 kHz Stereo |
| MP3 | MP3 | kein Video | MP3, 44,1/48 kHz, 192 kbit/s Standard |
| WAV | WAV | kein Video | PCM s16le, 48 kHz Stereo |

V1-Profile: 720p/1080p, 24/25/30/60 fps, Standard 1080p30, 16:9. 4K und weitere Seitenverhältnisse Soll. Qualität Low/Medium/High mit dokumentierten encoderabhängigen Parametern; Medium 1080p30 etwa 8 Mbit/s als Zielwert. Audio 192 kbit/s AAC. Encoder-Verfügbarkeit zur Laufzeit ermitteln, Softwarefallback dokumentieren. Exportbereich gesamte Timeline bis Ende des letzten Clips. Audio-only exportiert denselben Mix ohne Videostream. Leeres Projekt: Export deaktivieren. Ausgabe in temporäre Datei im Zielordner, bei Erfolg umbenennen; Überschreiben nur nach Bestätigung; bei Abbruch/Fehler temporäre Ausgabe entfernen, vorhandene Datei erhalten. Fortschritt aus verarbeiteter Zeit, ETA optional. Fehlerprotokoll lokal, keine automatische Übermittlung. Audio/Video-Dauer bis auf maximal einen Videoframe plus dokumentierte Encoder-Padding-Toleranz synchron; Ziel A/V-Offset maximal 40 ms im Test.

## 8. Erweiterungen (A13–A16)
Nach stabiler Kernversion: PNG/JPEG-Import, Textobjekte mit Font/Farbe/Ausrichtung, lineare Überblendung, Position/Skalierung/Rotation/Deckkraft, Farbregler, Bild-in-Bild. Danach Keyframes mit linearer Interpolation und eindeutigem Verhalten außerhalb des Intervalls, Geschwindigkeit, Wellenformen, Proxy-Cache, Autosave, 9 Video-/2 Audiospuren, 4K und Hochkantprofile. Keine Attrappen: unfertige Funktionen nicht als aktiv anzeigen. Jede Erweiterung erhält Tests für Preview und Export.

## 9. Qualität, Sicherheit und Datenschutz (A17–A18)
Lokale Verarbeitung, keine Telemetrie. Keine Shell-Interpolation von Dateipfaden; Bibliotheks-API oder QProcess mit Argumentliste. Medien als nicht vertrauenswürdig behandeln; Eingabegrößen, Dimensionen und Cache begrenzen, Decoderfehler abfangen. Abhängigkeiten auf Sicherheitsupdates prüfen. Logs ohne unnötige persönliche Pfade in exportierbaren Diagnoseberichten. Cache in Benutzerprofil, löschbar; Originaldateien nie löschen. Test auf Unicode-Pfade, Leerzeichen, lange Pfade, schreibgeschützte Ordner und volle Datenträger. Referenzsystem und Performance-Ziele gemäß Anforderungsanalyse.

## 10. Abnahmematrix
| Test | Bezug | Prüfschritt / Erfolg |
|---|---|---|
| T01 | A01,A02,A18 | Clean Windows VM: Installer ausführen, ohne SDK starten, deinstallieren; MIT und Drittanbieterhinweise vorhanden. |
| T02 | A03,A04 | Frisches Profil Englisch; Deutsch wählen, Neustart; alle Dialoge deutsch; Copyright und Michael Lappenbusch exakt sichtbar. |
| T03 | A05 | Je ein dokumentiertes Testmedium jeder Endung importieren, abspielen; falsche Endung/defekter Stream liefert Fehler ohne Crash. |
| T04 | A07,A10 | 10-s-Clip bei 4 s teilen; 4/6 s Teile, trimmen, verschieben, löschen, Undo/Redo; Originalhash unverändert. |
| T05 | A08,A11 | Zwei Video- und zwei Audiospuren; Overlay-Reihenfolge, Quellton, Musik, Mute, Fade, Lock prüfen. |
| T06 | A09 | Speichern, öffnen, Medien verschieben, relink; Timeline und Eigenschaften identisch; Abbruch beim Schließen erhält Projekt. |
| T07 | A06,A12 | MP4 und MKV exportieren, ffprobe bestätigt je Video- und Audiostream, Dauer, FPS und Auflösung; in unabhängigem Player abspielen. |
| T08 | A06 | Stummes Projekt als MP4/MKV: Audiostream vorhanden und still. Audio-only MP3/WAV: kein Videostream, korrekter Mix. |
| T09 | A12 | Export abbrechen, Ziel schreibgeschützt/voll simulieren; UI bleibt responsiv, keine beschädigte Zieldatei. |
| T10 | A17 | Offline starten, importieren, bearbeiten, exportieren; keine notwendigen Netzwerkverbindungen. |
| T11 | A10,A11 | VFR-Clip mit Synchronmarken und 60-s-1080p30-Test; A/V-Offset ≤40 ms, mindestens 29 fps auf Referenzsystem. |
| T12 | A18 | Clean Checkout, dokumentierter Build, CTest erfolgreich, Lizenzinventar geprüft. |

Soll-Features werden separat abgenommen, bevor sie als verfügbar beworben werden. Keine Freigabe bei offenen kritischen Fehlern, Datenverlust, fehlenden Muss-Tests oder ungeklärter Lizenzierung.

## 11. Entwicklungsplan für Codex
M0: Repository, LICENSE, CMake, CI, Architekturentscheidungen, Testfixtures. M1: FFmpeg-Probe, Decode, Audio/Video-Export-Prototyp inklusive Lizenz-Gate. M2: Domain, Projektpersistenz, Undo/Redo, Unit-Tests. M3: GUI, Import, Timeline, Player, Sprachwechsel und About. M4: Mehrspur-Compositor, Audio-Mix, Schnittoperationen. M5: Exportdialog, alle vier Formate, Fehlerbehandlung. M6: Integrationstests, Performance, Installer, Handbuch, Release 1.0. M7: Soll-Funktionen in einzeln abnehmbaren Releases. Kein fester Zeitplan ohne Aufwandsschätzung und Prototyp-Ergebnisse.

## 12. Repository und Lieferumfang
`src/app`, `src/domain`, `src/media`, `src/render`, `src/ui`, `src/export`, `tests/unit`, `tests/integration`, `tests/fixtures`, `translations`, `docs`, `packaging`, `.github/workflows`. Root: README.md, LICENSE, THIRD_PARTY_NOTICES.md, CONTRIBUTING.md, CHANGELOG.md, AGENTS.md, CMakeLists.txt, CMakePresets.json. Windows-Installer (Vorschlag Inno Setup) und portable ZIP; keine Adminrechte für normale Nutzung. Signierung optional, fehlende Signatur transparent dokumentieren. CI baut Windows x64, führt Tests aus und erzeugt Release-Artefakte. Testmedien synthetisch oder nachweisbar frei lizenziert.

## 13. Arbeitsanweisung für Codex
Lies zuerst Anforderungsanalyse und Pflichtenheft. Erstelle AGENTS.md mit Architektur- und Lizenzregeln. Implementiere jeweils nur einen Meilenstein, beginne mit M0/M1. Vor jeder Änderung betroffene Anforderungen und Tests benennen. Keine Funktionen vortäuschen, keine Testfehler ignorieren, keine GPL/nonfree-Abhängigkeit ohne Freigabe einführen. Führe Build und Tests aus; dokumentiere tatsächlich ausgeführte Befehle und Ergebnisse, verbleibende Fehler und Entscheidungen. Bei nicht ausführbaren Windows-Tests ausdrücklich offen markieren. Änderungen klein und nachvollziehbar halten. Definition of Done: Code, Tests, Dokumentation, Lizenzprüfung, reproduzierbarer Build und erfüllte Abnahmekriterien.

## 14. Offene Entscheidungen und Freigabe
Auftraggeber bestätigt vorgeschlagenen Stack, Windows-Ziel, 1.0-Umfang, Codec-Fallback, Projektdateiendung, Referenzhardware, Installationsmodell und Copyright-Inhaber für die MIT-LICENSE-Datei. Sichtbarer Copyright-Text und About-Name sind bereits vorgegeben. Die MIT-Datei benötigt zusätzlich den rechtlich zutreffenden Copyright-Inhaber und Jahr. Die technischen Vorschläge wurden noch nicht durch einen Windows-Prototyp validiert. Das Pflichtenheft wird nach M1 anhand gemessener Ergebnisse aktualisiert.
