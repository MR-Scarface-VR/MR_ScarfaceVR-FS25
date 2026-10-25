# MR_ScarfaceVR für FS25 — Installation

Von **MR_Scarface**. Inoffizieller **Mod for Farming Simulator**.
Version 0.1.0.4: unsignierter experimenteller Build mit optionaler ReShade-
Unterstützung und dem Gelände-Fix aus 0.1.0.3. Ein Nutzer bestätigte am
25.10.2026 mit Quest 2 / Meta Link den gewählten Effekt in beiden Augen und
korrektes Kopftracking (ReShade 6.8.0.2155). Nicht alle Shader sind kompatibel.
Der gezielte Vergleich am 25.10.2026 mit Quest 2 / Meta Link auf einem PC
bestätigte: Boden-/Feldtexturen folgen dem Headset statt der Maus-Kamera.
Der frühere Build 0.1.0.1 bestand den kurzen Funktionstest für Menü, 3D/Tracking,
zu Fuß/Kabine, normale Steuerung und F10; eine vollständige Wiederholung und
Langzeitprüfungen mit 0.1.0.4 stehen noch aus.

## Voraussetzungen

- Windows/D3D12, exakt FS25 **1.25.0.0** mit unterstützter ausführbarer Datei.
- Meta Quest Link installiert, Quest erfolgreich verbunden.
- Bisherige praktische Bestätigung: Quest 2, Singleplayer, zu Fuß und Kabine.
- Normale Steuerung; keine VR-Hände und keine Multiplayer-Unterstützung.

Bei anderen Spielversionen bleibt der native Adapter deaktiviert. Andere
Headsets/PCs, längerer Betrieb und Verbindungsabbrüche sind ungeprüft.
Smart App Control kann diese unsignierte DLL blockieren; die Veröffentlichung
behauptet keine SignPath-Aufnahme. Schutzfunktionen nicht als regulären
Installationsschritt abschalten.

## Installation bei geschlossenem FS25

1. Das Gesamtpaket entpacken. Es gehört als Ganzes nicht in den Mods-Ordner.
2. `x64/dinput8.dll`, `x64/openxr_loader.dll` und den Ordner
   `x64/MR_ScarfaceVR_OpenXR/` in den `x64`-Ordner der FS25-Installation kopieren,
   direkt neben `FarmingSimulator2025Game.exe`. Fremde vorhandene DLLs nicht
   überschreiben. Beim Upgrade alle mitgelieferten Teile aktualisieren.
3. `mods/FS25_MR_ScarfaceVR.zip` **unentpackt** in den persönlichen Mods-Ordner
   kopieren: normalerweise `Dokumente/My Games/FarmingSimulator2025/mods/`.
4. Beim Laden des Spielstands **MR_ScarfaceVR Experimentell** aktivieren.

Bei einem Upgrade vom alten eigenen Pilot-Mod `FS25_OwnVR.zip` diesen bei
geschlossenem Spiel entfernen beziehungsweise deaktivieren. Beide Varianten
nicht gleichzeitig aktivieren. Nur Dateien dieses Projekts entfernen.

## ReShade optional verwenden

ReShade separat für DirectX 10/11/12 installieren bzw. die vorhandene Installation
mit `dxgi.dll`, Shadern und Presets beibehalten. Unsere `dinput8.dll` nicht
durch ReShade ersetzen. Der Mod bindet ReShade lokal für FS25 ein; eine globale
OpenXR-Layer-Installation ist dafür nicht erforderlich. ReShade-Effekte gelten
für die räumliche Welt, nicht für unsere flache Menüfläche. VR-Einstellungen
liegen separat in `ReShadeVR.ini`; gegebenenfalls dort denselben Preset wählen.
Zuerst einen einfachen Farbeffekt in beiden Augen vergleichen. Einzelne Shader
melden Fehler; Tiefen-Effekte wie AO/DoF/SSR sind nicht allgemein bestätigt.
[Ausführlicher Testumfang](docs/RESHade_TEST_0.1.0.4.md).

## Spielen

- VR startet ausgeschaltet. **F11** aktiviert VR; ein weiteres F11 fordert das Ende an.
- Der Meta-Start dauerte im bisherigen Test etwa 30 Sekunden; FS25 kann dabei
  vorübergehend nicht reagieren.
- Zuerst im Headset ein klares Hauptmenü als Fläche prüfen.
- Im Spiel die Ich-Perspektive oder Fahrzeugkabine wählen. **F10** zentriert neu.
- Menüs und Außenansicht werden auf einer flachen Fläche dargestellt.
- **F9** ist ausschließlich Desktop-Stereo-Diagnose, für VR nicht erforderlich.
- Bei Flackern oder unangenehmer Darstellung Brille absetzen und FS25 beenden.
  Nach Sitzungsverlust kann ein kompletter Spielneustart nötig sein.

## Rückmeldung und Entfernen

Für einen Fehler Spiel-/Mod-Version, Headset, Verbindung, GPU/Treiber, andere
Mods und Schritte zur Wiederholung nennen. Hilfreich sind
`x64/fs25-mr_scarfacevr.log` und das normale FS25-Profillog. Persönliche Daten
vor öffentlichem Teilen entfernen. `fs25-mr_scarfacevr-eye.bmp` kann ein Spielbild
enthalten; nur bei Bedarf teilen.

FS25 beenden, dann nur die hier installierte eigene `x64/dinput8.dll`,
`x64/openxr_loader.dll`, den Ordner `x64/MR_ScarfaceVR_OpenXR/` und
`mods/FS25_MR_ScarfaceVR.zip` entfernen. Separate ReShade-Dateien belassen. Eigene Diagnoselog-/BMP-Dateien können
ebenfalls entfernt werden. Keine originale Spiel-EXE ersetzen oder löschen.
