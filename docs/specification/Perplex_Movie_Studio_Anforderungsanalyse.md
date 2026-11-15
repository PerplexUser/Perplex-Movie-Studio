# Perplex Movie Studio – Anforderungsanalyse

Version 1.0 | 12.11.2026 | Entwurf zur Freigabe

Projektinitiator: Michael Lappenbusch · www.perplex.click

## 1. Ziel und Produktvision
Eine eigenständige, kostenlose, quelloffene Windows-Desktop-Anwendung für nichtlinearen Videoschnitt. Zielgruppen sind Einsteiger, Hobbyfilmer und Content Creator. Die Anwendung arbeitet lokal, ohne Konto, Abonnement, Wasserzeichen oder Exportkontingente. Der eigene Quellcode steht unter MIT.

## 2. Stakeholder und Nutzung
Projektinitiator: Produktentscheidungen, Freigabe und Release. Entwickler/Codex: Implementierung, Tests, Dokumentation. Anwender: Medien importieren, arrangieren, schneiden, gestalten, speichern und exportieren. Open-Source-Mitwirkende: reproduzierbarer Build, nachvollziehbare Architektur und Beitragsregeln.

## 3. Anforderungen und Prioritäten
M = Muss für Version 1.0; S = Soll, nach stabiler Kernfunktion; K = Kann, spätere Erweiterung. Die expliziten Nutzervorgaben sind Muss; weitere Anforderungen sind vorgeschlagene Festlegungen zur Freigabe.

| ID | Priorität | Anforderung / überprüfbares Ergebnis |
|---|---|---|
| A01 | M | Windows 10 22H2 und Windows 11 x64; Installation und Start ohne Entwicklungsumgebung. Windows 10 nur Kompatibilitätsziel, kein Versprechen von Betriebssystem-Sicherheitsupdates. |
| A02 | M | Produktname Perplex Movie Studio; eigener Code MIT, keine Bezahlschranken. |
| A03 | M | Erster Start Englisch; Umschaltung auf Deutsch, Einstellung bleibt erhalten. Alle Menüs, Dialoge und Fehlertexte übersetzt. |
| A04 | M | Sichtbarer Text `© by www.perplex.click` im Hauptfenster; Hilfe > About Perplex Movie Studio / Über Perplex Movie Studio zeigt Michael Lappenbusch, Version, Website und Lizenzhinweise. |
| A05 | M | Videoimport AVI, MPG, MPEG, MP4, MKV; Audioimport MP3, WAV. Unterstützte Codecs explizit dokumentieren; Dateiendung allein ist keine Kompatibilitätsgarantie. |
| A06 | M | Videoexport MP4 und MKV, stets mit Audiospur; bei stummem Projekt Stille erzeugen. Audio-only MP3 und WAV als getrennte Exportoption. |
| A07 | M | Nichtdestruktiver Schnitt: Clips teilen, trimmen, verschieben, löschen, duplizieren; Quelldateien unverändert. |
| A08 | M | Medienbibliothek mit Metadaten, Drag-and-drop, Mehrspur-Timeline, Vorschau, kontextsensitiver Inspektor. Mindestens 2 Video- und 2 Audiospuren; Ziel 9 Video- und 2 Audiospuren als Soll. |
| A09 | M | Projekte erstellen, speichern, öffnen; fehlende Quellen neu verknüpfen; Undo/Redo für Bearbeitungen. |
| A10 | M | Wiedergabe, Pause, Scrubbing, Einzelbildnavigation, Zeitcode, Timeline-Zoom, Spur stumm/sperren. |
| A11 | M | Audio-Lautstärke, Stummschaltung, Fade-in/out, Mischung von Quellton und Musik. |
| A12 | M | Exportdialog mit Zielpfad, Auflösung, Bildrate, Qualität, Fortschritt, Abbruch und verständlichen Fehlern. |
| A13 | S | Titel, Text, Bilder PNG/JPEG, Überblendungen, Farbkorrektur, Position/Skalierung/Rotation, Deckkraft, Bild-in-Bild. |
| A14 | S | Keyframes, Geschwindigkeitsänderung, weitere Effekte, Wellenformen, Thumbnail-Cache, Proxy-Medien, automatische Sicherung. |
| A15 | S | 4K, Hardwareencoder, 9:16/1:1/4:3, zusätzliche Spuren, Vorlagen mit geklärten Lizenzen. |
| A16 | K | Multicam, Tracking, KI-Untertitel, erweiterte Masken, Plugin-Schnittstelle; keine Zusage für 1.0. |
| A17 | M | Keine verpflichtende Internetverbindung, Telemetrie oder Uploads; lokale Datenverarbeitung. |
| A18 | M | Quellcode, Build-Anleitung, Tests, MIT-Lizenz, Drittanbieterhinweise, Installationspaket und Nutzerhandbuch verfügbar. |

## 4. Qualitätsziele
- Referenzsystem (Vorschlag): Windows 11 x64, 4 CPU-Kerne/8 Threads, 16 GB RAM, SSD, 1920×1080. Auf diesem System soll ein 1080p30-H.264-Projekt mit einer Videospur und zwei Audioströmen flüssig abspielbar sein; Messziel mindestens 29 fps über 60 Sekunden, ohne anhaltende Audioaussetzer. Testmaterial und Hardware dokumentieren.
- Oberfläche bleibt bei Import und Export bedienbar; lange Aufgaben laufen im Hintergrund. Abbruch reagiert binnen 2 Sekunden im Referenztest; Prozessbereinigung kann länger dauern und wird angezeigt.
- Kein Datenverlust bei normalem Speichern; atomare Speicherung, Wiederherstellung nach fehlgeschlagenem Speichern. Automatische Sicherung als Soll.
- Tastaturbedienung, sichtbarer Fokus, skalierbare Oberfläche bei 100–200 % Windows-DPI, ausreichender Kontrast.
- Verständliche Meldungen bei ungültigen Dateien, fehlenden Codecs, fehlendem Speicherplatz und nicht erreichbaren Quellen.

## 5. Hauptanwendungsfälle
UC01: Neues Projekt erstellen, Seitenverhältnis wählen, Medien importieren, auf Timeline ziehen, Vorschau starten. UC02: Clip am Abspielkopf teilen, Teil löschen, Reihenfolge ändern, Undo/Redo ausführen. UC03: Musik hinzufügen, Quellton absenken, Fade setzen. UC04: Projekt speichern, schließen, erneut öffnen, fehlende Datei neu zuordnen. UC05: MP4/MKV mit gemischtem Audio exportieren; alternativ nur Ton als MP3/WAV. UC06: Sprache wechseln, Anwendung neu starten und About-Dialog prüfen.

## 6. Abgrenzung und Risiken
Keine Übernahme proprietärer Ressourcen. Keine Garantie für sämtliche historischen AVI/MPEG-Codecs, DRM-Medien oder beschädigte Dateien. Keine kommerzielle Codec-Lizenz wird durch MIT automatisch erteilt. Echtzeitvorschau und Export müssen dieselbe Timeline-Semantik besitzen. Besonders risikoreich sind variable Bildraten, A/V-Synchronität, Mehrspur-Compositing und Lizenzierung von Multimedia-Bibliotheken. Diese Risiken werden vor Ausbau der GUI durch einen technischen Prototyp geprüft.

## 7. Freigabeentscheidungen
Vorgeschlagen, nicht bereits vom Auftraggeber beschlossen: Windows-x64-Ziel, Qt/C++/FFmpeg, Mindestspurzahl, Qualitätsmesswerte, H.264/AAC als Zielcodec, Projektformat, Installationsverfahren und gestufter Funktionsumfang. Vor Release müssen insbesondere Codec-Verfügbarkeit und Patent-/Drittanbieterlizenzen geprüft werden. Das Pflichtenheft konkretisiert diese Vorschläge.
