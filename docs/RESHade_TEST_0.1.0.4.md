# ReShade-VR-Test 0.1.0.4

Unsignierter experimenteller Build von MR_Scarface.
[Release 0.1.0.4](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/releases/tag/v0.1.0.4-experimental).
FS25 1.25.0.0, Windows/D3D12, Quest 2 über Meta Link.
Der Gelände-Fix aus 0.1.0.3 bleibt enthalten.

## Änderung

Der Mod verwendet jetzt den offiziellen Khronos-OpenXR-Loader. Wenn deine
bereits installierte ReShade-dxgi.dll die OpenXR-Schnittstelle exportiert,
fordert der Mod deren API-Layer an. ReShade verarbeitet dann die räumlichen
Augenbilder an der OpenXR-Übergabe. Die getrennten Augen und ihr gemeinsamer
Simulationsframe bleiben erhalten.

Die API-Layer-Suchpfade werden nur während der Initialisierung im Spielprozess
ergänzt und anschließend wiederhergestellt. Keine globale Registrierung,
kein SteamVR, keine zusätzliche Programminstallation. Ein bereits vorhandener
Layer mit demselben Namen kann Vorrang erhalten; solche Mehrfachinstallationen
sind noch nicht geprüft. ReShade selbst wird nicht mitgeliefert.

## Installation bei geschlossenem Spiel

- x64/dinput8.dll und x64/openxr_loader.dll neben die Spiel-EXE kopieren.
- x64/MR_ScarfaceVR_OpenXR/ReShade.json als Unterordner daneben kopieren.
- mods/FS25_MR_ScarfaceVR.zip in den persönlichen Mods-Ordner kopieren.
- Die vorhandene ReShade-dxgi.dll, Shader und Presets beibehalten. Fremde DLLs
  nicht überschreiben. Das Paket benötigt keine ReShade-Neuinstallation.

## Vergleich mit der Quest

1. Meta Link verbinden; FS25 starten und im Hauptmenü F11 drücken. Der Start
   kann wie zuvor einige Zeit dauern.
2. Spielstand mit MR_ScarfaceVR Experimentell laden. Zu Fuß oder in der Kabine
   bleiben: ReShades OpenXR-Pfad bearbeitet die räumliche Welt, nicht unsere
   flache Menüfläche. Ein unverändertes Hauptmenü ist daher kein Fehlernachweis.
3. Am PC mit Pos1/Home das ReShade-Menü öffnen. ReShade führt seine VR-Konfiguration
   als ReShadeVR.ini separat. Für den Vergleich einen klar sichtbaren einfachen
   Farb-/Schwarzweißeffekt verwenden. Falls nichts aktiv ist, einen Effekt aktivieren.
   Die VR-Konfiguration kann einen anderen Preset als das Monitorbild verwenden;
   im Zweifel denselben Preset auswählen. Der Mod ändert deinen Preset nicht.
4. Jeweils kurz ein Auge schließen: Ist der Effekt in beiden Augen sichtbar?
   Effekt aus/ein vergleichen. Kopftracking, F10, normales Gehen/Fahren und
   Bodenauflösung prüfen. Bei Flackern Brille absetzen und Spiel normal beenden.

Farb-/Kontrast-Effekte sind der erste Test. Tiefenabhängige Effekte (AO, DoF,
SSR) haben keinen zugesagten Stereo-Tiefenpuffer; sie sind separat zu prüfen.
Nicht jeder Bildschirm-Shader ist für zwei nebeneinander verarbeitete Augen
geeignet. Der Kandidat behauptet keine allgemeine Shader-Kompatibilität.

## Prüfstatus und Diagnose

Bestanden: native Tests, Loader-Aufruf-/Fehler-/Umgebungsprüfungen, OpenXR-Frame-
Tests, DirectInput-Weiterleitung, Metadaten, 13 Python-Tests und echter GPU-
Pixelvergleich. Eine Meta-Link-Sitzung wurde mit und ohne ReShade erstellt.
ReShades Log bestätigt xrCreateApiLayerInstance und xrCreateSession auf dem
ReShade-D3D12-Pfad; dabei wurden keine Augenbilder angezeigt.

Am 25.10.2026 bestätigte der Nutzer im Spiel mit Quest 2 / Meta Link:
Der gewählte Effekt ist in beiden Augen sichtbar, Kopftracking ist korrekt.
Das Live-Log bestätigt ReShades VR-Runtime mit 3840x1080 und weiterhin beide
Augen aus demselben Simulationsframe. Das Gelände-Tracking bleibt im Log aktiv.
Die Bezeichnung des getesteten Effekts wurde nicht erfasst. Einige weitere
installierte Shader melden Sampler-Deskriptorfehler; ihre Kompatibilität ist
nicht bestätigt. Andere PCs, alle Shader und vollständige Langzeit-/Regressionstests
sind nicht durch diese Rückmeldung abgedeckt.

Eigene Diagnose: x64/fs25-mr_scarfacevr.log. ReShade-Diagnose: x64/ReShade.log.
Beim Start soll das eigene Log den Khronos-Loader und XR_APILAYER_reshade nennen;
ReShade soll die Sitzung verarbeiten und später seinen VR-Runtime anlegen.
Logs enthalten lokale Pfade; vor öffentlichem Teilen bereinigen.

## Rückbau

Bei geschlossenem FS25 die gesicherten eigenen DLL-/Mod-Dateien von 0.1.0.3
wiederherstellen. Nur den von diesem Kandidaten neu installierten Loader und
MR_ScarfaceVR_OpenXR-Ordner entfernen; ReShade-Dateien und Spiel-EXE belassen.
