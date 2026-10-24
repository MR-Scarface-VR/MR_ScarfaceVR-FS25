# MR_ScarfaceVR für FS25

Von **MR_Scarface**. Inoffizieller experimenteller VR-Mod.

**Version 0.1.0.3 — Gelände-Korrektur:** Die Gelände-Texturen folgen jetzt dem
VR-Kopftracking. Ein Nutzer bestätigte den Vergleich mit Quest 2 / Meta Link am
25.10.2026; das Log bestätigt die Übergabe an den Gelände-Thread. Automatisierte
Prüfungen bestanden; weitere Regressionstests und andere PCs/Headsets bleiben offen.
In 0.1.0.1 konnten Boden-/Feldtexturen außerhalb der Maus-Blickrichtung
unscharf bleiben. 0.1.0.3 übergibt das Kopftracking an den Gelände-Thread.
[Gezielter Vergleichstest](TESTING.md#terrain-comparison--gelände-vergleich).

[English](README.md) · [Testablauf](TESTING.md#deutsch) ·
[Testergebnis melden](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/issues/new?template=test_report.md)

**Vorheriger Stand 0.1.0.1:** Neue Projektkennzeichnung und DLL-Metadaten für die
SignPath-Vorbereitung. Noch unsigniert. Auch dieser Build bestand am
24.10.2026 den kurzen Quest-2-Praxistest: Menü, räumliche Welt, Kopftracking,
Spielen zu Fuß und in der Kabine, normale Steuerung und F10-Neuzentrierung.
Bestätigt auf einem PC durch einen Nutzer; kein Langzeit- oder Mehr-PC-Test.

[Testpaket 0.1.0.3 herunterladen](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/releases/tag/v0.1.0.3-experimental).
Die verteilte DLL stammt aus dem lokalen MSVC-Build; die getesteten Bytes sind
unverändert. Quellen und automatische Prüfungen sind öffentlich.

[Installation und Bedienung](INSTALLATION.md).

Eigener nativer VR-Adapter für FS25 **1.25.0.0**, Windows/D3D12 und Quest 2
über die installierte Meta-Quest-Link-Laufzeit. Keine vorhandenen VR-Mods,
kein SteamVR und keine zusätzliche Programminstallation.

Inoffizieller **Mod for Farming Simulator**; keine Verbindung oder Freigabe durch
GIANTS oder Meta wird behauptet. Eigene Quellen: [MIT-Lizenz](LICENSE).
[Fremdsoftware und Abhängigkeiten](THIRD_PARTY.md), [Datenschutz](PRIVACY.md).

## Code signing policy

Derzeit sind die Pilot-Builds **unsigniert**. Der Antrag bei SignPath wurde am
24.10.2026 eingereicht; die Website bestätigte den Versand. Eine Aufnahme oder
Signatur ist noch nicht erfolgt.
[Code signing policy](CODE_SIGNING_POLICY.md) und [SignPath-Einrichtung](signing/SETUP.md).

**Vorheriger Funktionsstand 0.1.0.0: kurzer Praxistest bestanden.** Am 24.10.2026
bestätigte der Nutzer räumliche Spielwelt und Kopftracking auf seiner Quest 2.
Auch Spielen zu Fuß und in einer Fahrzeugkabine, normale Steuerung, Menü,
Neuzentrieren und plausible Größenverhältnisse wurden positiv geprüft.
Beide Augen verwenden nachweislich denselben Simulationsframe. Zusätzlich
bestanden automatisierte Pose-/Protokolltests, ein echter GPU-Pixelvergleich
und Bildmitschnitte aus FS25. Langzeit- und Ausfalltests stehen noch aus.

## Bedienung des Test-Builds

- VR ist beim Spielstart ausgeschaltet. **F11** schaltet die Meta-Sitzung ein;
  erneutes F11 fordert deren Ende an. Der Laufzeitstart benötigt hier etwa
  30 Sekunden; währenddessen kann das Spielfenster vorübergehend nicht reagieren.
- **F10** zentriert die aktuelle Kopfposition und Blickrichtung neu.
- **F9** schaltet ausschließlich die Desktop-Stereo-Diagnose um, standardmäßig aus.
- Spielsteuerung bleibt bei Tastatur/Maus, Gamepad oder Lenkrad.
- Der Lua-Teil wählt VR-Weltansicht für Ich-Perspektive/Kabine und eine flache
  Menüfläche für Menüs/Außenansicht. Beide Augen im Spielstand verwenden nachweislich
  denselben Simulationsframe; die räumliche Darstellung und das Kopftracking wurden
  in der Quest 2 praktisch bestätigt.

Zuerst nur das Hauptmenü prüfen. Bei erneutem Flackern die Brille absetzen und
FS25 normal beenden. Längerer Spielbetrieb, detaillierte HUD-Lesbarkeit und
Tracking-/Verbindungsverlust benötigen noch weitere praktische Tests.
Sitzungsverlust oder ein Austausch des nativen Grafikfensters kann derzeit
einen Neustart von FS25 erfordern. Mehrspieler und VR-Hände sind nicht enthalten.

## Dateien und Rückbau

Das Gesamtpaket enthält `START_HIER.txt` mit der Installationsanleitung.
[Kurzer Testablauf und Rückmeldung](TESTING.md#deutsch).
Das Gesamtpaket zuerst entpacken: nur `mods/FS25_MR_ScarfaceVR.zip` kommt unentpackt
in den persönlichen Mods-Ordner; `x64/dinput8.dll` kommt neben die Spiel-EXE
im Installationsordner `x64`. Eine vorhandene fremde `dinput8.dll` nicht
überschreiben. Beim Laden des Spielstands **MR_ScarfaceVR Experimentell** aktivieren.

Der native Teil ist eine eigene `dinput8.dll` neben
`x64/FarmingSimulator2025Game.exe`. Er lädt Windows' echtes DirectInput aus dem
absoluten System32-Pfad nach. EXE-Identität und native Eingriffspunkte werden
geprüft. Bei einer anderen Spielversion bleiben die Eingriffe deaktiviert.

Der Lua-Teil liegt unter `mod/FS25_MR_ScarfaceVR/`; ein ZIP enthält `modDesc.xml` und
`VR.lua` direkt im Archiv und gehört ins persönliche FS25-Verzeichnis `mods`.
Vorhandene gleichnamige Dateien dürfen nur nach Prüfung ihrer Herkunft ersetzt
werden. Spielstände und Original-EXE/DLLs werden nicht bearbeitet.

Zum Rückbau bei geschlossenem Spiel ausschließlich die eigene geprüfte
`x64/dinput8.dll` und `mods/FS25_MR_ScarfaceVR.zip` entfernen. Diagnoseartefakte heißen
`fs25-mr_scarfacevr.log` und `fs25-mr_scarfacevr-eye.bmp`. Private Engine-Abbilder unter
`.analysis/` gehören nicht in ein verteilbares Mod-Paket.

## Bauen und prüfen

Vorhandenes MSVC x64/Windows SDK und PowerShell verwenden:

```powershell
.\build.ps1
.\build.ps1 -Target gpu
python -m unittest discover -s tests -v
```

`-Target gpu` verwendet die echte Grafikkarte für den Pixelvergleich und öffnet
keine VR-Sitzung. `-Target capture` baut ausschließlich eine Desktop-Diagnose
mit Bildmitschnitt und verhindert die OpenXR-Aktivierung vollständig.
`-Target runtime` prüft die installierte Meta-Laufzeit und kann eine Sitzung
anlegen; es ist ein Entwicklungswerkzeug, kein notwendiger Launcher.

Offizielle OpenXR-Header: `third_party/openxr/ORIGIN.json`, MIT-Lizenz daneben.
Grundlagen: [OpenXR-Bildpuffer](https://registry.khronos.org/OpenXR/specs/1.0-khr/html/xrspec.html#rendering),
[Microsoft CopyTextureRegion](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-copytextureregion).
