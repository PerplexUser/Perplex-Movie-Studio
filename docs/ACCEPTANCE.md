# Abnahmestand

**0.1.1-dev · Revision 2, 13. November 2026.** Entwicklungsversion; Windows-Abnahme und Leistungstests stehen aus.

| Bereich | Nachgewiesen | Noch offen |
|---|---|---|
| Build/Verteilung | Native Linux-Kompilierung; Windows-x64-Cross-Build; statische DLL-/Symbolprüfung; Installer-Nutzdateien | Start, Update, Deinstallation auf sauberem Windows |
| Farben | Vollbild-Pixelprüfung in sechs Quer-/Hochformaten; erneuter MP4-Import/Export; Qt-Software-Vorschau | Wiedergabe der konkreten Nutzerdateien auf dessen Windows-System |
| Vorschau | Pausiertes Bild, Play/Pause, Seek, Bildschritte; gemeinsame Exportkomposition | Langzeitwiedergabe und Audioausgabe auf Windows |
| Medien | AVI/MPEG/MP4/MKV/MP3/WAV-Importmatrix; JPG/PNG mit Alpha; ungenutzte Medien entfernen | Historische Sondercodecs und beschädigte Fremddateien im Einzelfall |
| Schnitt/Ton | Verschieben, Trimmen, Split, Lock, Undo/Redo, Quellschutz, Audio-Mix/Blenden | Hörtest und Synchronmarken-Test auf Windows |
| Text/Bild | Dauer, Position, Unicode, Schrift/Farbe, Helligkeit/Kontrast/Sättigung/Farbton; Fade/Wipe/Slide | Lokale Ersatzschrift/DPI-Verhalten auf Windows |
| Projekte | Schema-2-Roundtrip und Schema-1-Migration, relative Pfade, Relink, ungültige Werte | Schließen-/Recovery-Dialoge nach Windows-Neustart |
| Export | MP4/MKV (MPEG-4 Part 2/AAC), MP3/WAV, Portrait bis 1080×1920, Stille und Audio-only | Abspielen in allen Zielgeräten; volle Datenträger und Windows-Berechtigungen |
| Exportdateien | Erfolg/Überschreiben, Abbruch, bestehende Datei erhalten, fehlgeschlagene Übernahme, temporäre Dateien aufräumen | MoveFileExW-Ausführung auf realem Windows-Dateisystem |
| Sprache/About | 197 fertige Übersetzungen, Platzhalterprüfung, exakter deutscher About-Text, UI-Screenshot | Vollständige interaktive Windows-Dialogabnahme |
| Offline/Lizenzen | Keine Netzwerkfunktionen; FFmpeg ohne GPL/nonfree/network; Lizenztexte, Quellen, SBOM | Abschließende Release-, Sicherheits- und Vertriebsprüfung |

Automatisierte Ausführung: alle sechs CTest-Gruppen bestanden (Linux). Windows-Testprogramme wurden gebaut, aber nicht ausgeführt. Kein Echtzeit-, HDR- oder Hardwareencoder-Versprechen. Beliebige Videotransformationen, Keyframes, Proxys, Wellenformen und 4K sind nicht Bestandteil dieser Revision.

## Kurze Windows-Nachprüfung

1. Revision 2 starten, in den Einstellungen Deutsch wählen und den About-Text prüfen.
2. Ein MP4 importieren, an V1 anhängen, mit „Bild vor“ vorbereiten; Farben und Standbild prüfen, anschließend abspielen/suchen.
3. MP4 in einen normalen Zielordner exportieren und in einem Player öffnen; anschließend nach Bestätigung überschreiben.
4. Text hinzufügen, Dauer/Position/Ein- und Ausblende ändern, Vorschau erneut vorbereiten und exportieren.
5. JPG/PNG auf V2 einfügen, darunter V1 überlappen lassen und drei Übergänge prüfen.
6. Smartphone-Video verwenden, Projekt und Export auf Hochformat einstellen; 720×1280 bzw. 1080×1920 kontrollieren.

Diese Liste dokumentiert die noch ausstehende Windows-Abnahme; sie wird nicht als bereits bestanden geführt.
