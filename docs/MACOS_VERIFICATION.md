# macOS-Port: Prüfnachweise

Dieser Bericht hält die lokalen Vorprüfungen vom 14. November 2026 fest.
Den aktuellen nativen Build- und Teststatus eines fertigen Mac-Pakets enthalten
die beigefügten Dateien `verification.json` und `dependency-check.json`.

## Tatsächlich ausgeführte Prüfungen

- Vollständiger C++-Build unter Linux x86_64, GCC 14.2 und Qt 6.8.3: erfolgreich.
- CTest: **7/7 Testgruppen bestanden**, 13,08 Sekunden.
- Gruppen: Domain, Medien/Persistenz/Export, UI-Smoke, Vorschau/Wiedergabe, visuelle Funktionen, Editor-Workflow und lokale Dokumentöffnungsereignisse.
- Übersetzungen: 197 abgeschlossene Einträge mit passenden Platzhaltern.
- SHA-256-Abgleich aller fünf mitgelieferten Drittanbieter-Quellarchive: erfolgreich.
- Zusammengesetztes Qt-Archiv aus den fünf GitHub-Teilen ist bytegleich zum Archiv aus dem Benutzer-ZIP.
- Shell-Syntax, Python-Kompilierung, Workflow-YAML, App-Plist und ICNS-Struktur: erfolgreich.
- Abhängigkeitspfad-Auflösung lehnt fehlende Bibliotheken ab; Mac-Builds werden auf Linux ausdrücklich verweigert.

Der neue Ereignistest prüft frühe und spätere lokale Projektöffnungen, Leerzeichen/Umlaute und die Ablehnung fremder Dateitypen sowie entfernter URLs. Er simuliert die Qt-Ereignisse; ein realer Finder-Test ist damit nicht behauptet.

## Native Paketprüfung

Der ARM64-/Intel-Build und die Paketprüfung laufen nativ über GitHub Actions.
Die vorstehenden Linux-Ergebnisse sind ausdrücklich kein Nachweis eines Mac-Builds.
Nur die mit dem jeweiligen Paket gelieferten nativen Prüfberichte bestätigen
dessen erfolgreiche Erstellung. ZIP und DMG werden bei fehlgeschlagenen Tests
nicht erzeugt.

Zusätzlich offen: manuelle Finder-/Dock- und Fensterbedienung, hörbare Audioausgabe,
Leistung auf Benutzerhardware, Tests auf allen unterstützten macOS-Versionen
sowie Developer-ID-Signierung und Notarisierung für den öffentlichen Vertrieb.

Nach einem nativen Build liefern `verification.json` und `dependency-check.json`
die Ergebnisse des jeweiligen Pakets. Der Paketbau bricht vor ZIP/DMG-Erstellung
ab, wenn Tests, Architektur-, Bibliotheks-, Mindestversions- oder Signaturprüfungen
fehlschlagen.
