# MR_ScarfaceVR: test guide / Testablauf

[Report a test / Testergebnis melden](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/issues/new?template=test_report.md)
· [Download 0.1.0.1](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/releases/tag/v0.1.0.1-experimental)

## English

**First check:** Windows/D3D12, exactly FS25 **1.25.0.0**, Quest 2 connected through
Meta Quest Link. The current DLL rejects other game builds. Other headsets are
unverified. Version 0.1.0.1 is experimental and unsigned; do not disable Windows
security protections to run it. Follow [installation](README.md#install-and-play)
and start with a backed-up save or a separate test save.

### Short test

1. Start FS25 with VR off. Press **F11** in the main menu; allow the runtime to
   start (about 30 seconds in the pilot). Check for a clear, stable menu panel.
2. Load the test save with **MR_ScarfaceVR Experimentell** enabled. Use first-person
   on foot. Check that the world surrounds you; rotate your head and gently move
   it sideways to check that the view follows both movements.
3. Check depth and scale in the headset. Two eye images on a monitor alone are
   not enough to confirm comfortable stereo.
4. Walk using your normal controls, enter a vehicle cab and drive briefly.
   Check the view and normal controls in both places.
5. Sit/look in your preferred neutral pose and press **F10**. Check recentering.
6. Open and close a menu. Check that the flat menu and world views switch correctly.
7. If the short test is comfortable, try a longer session (for example 15 minutes).
   Report actual minutes played and any flicker, crashes, tracking or HUD issues.

If flicker or discomfort occurs, remove the headset and exit normally. A lost VR
session can require a game restart; automatic reconnect is not promised. No need
to deliberately provoke a disconnection during the first test.

### Send a result

Successful tests are as useful as failures. Include the mod/game versions, headset,
connection/runtime, GPU/driver, Windows version, other active mods, time played,
each check's result and steps for any problem. Say **not tested** where appropriate.
No public display of your real name or email is needed; GitHub reports are public.

For log attachments, remove personal information first. The adapter log is
`x64/fs25-mr_scarfacevr.log`; the regular FS25 log is in your personal game folder.
Attach an eye-image diagnostic only if relevant; it can show your gameplay.
There is no automatic upload or telemetry in the adapter.

## Deutsch

**Zuerst prüfen:** Windows/D3D12, exakt FS25 **1.25.0.0**, Quest 2 über Meta Quest
Link verbunden. Andere Spielversionen werden abgelehnt, andere Headsets sind
ungeprüft. Version 0.1.0.1 ist experimentell und unsigniert. Windows-Schutzfunktionen
für den Test nicht abschalten. Nach der [Installationsanleitung](INSTALLATION.md)
vorgehen und einen gesicherten oder eigenen Testspielstand verwenden.

### Kurzer Test

1. FS25 zunächst ohne VR starten. Im Hauptmenü **F11** drücken und den Laufzeitstart
   abwarten (bisher etwa 30 Sekunden). Ist die Menüfläche klar und stabil?
2. Testspielstand mit **MR_ScarfaceVR Experimentell** laden und zu Fuß in die
   Ich-Perspektive gehen. Umgibt dich die Welt? Folgt die Sicht vorsichtigen
   Drehungen und seitlichen Kopfbewegungen?
3. Tiefe und Größenverhältnisse im Headset prüfen. Zwei Augenbilder am Monitor
   allein bestätigen noch keine angenehme räumliche Darstellung.
4. Mit normaler Steuerung gehen, eine Fahrzeugkabine betreten und kurz fahren.
   Bild und Bedienung in beiden Ansichten prüfen.
5. Die gewünschte neutrale Kopfhaltung einnehmen und mit **F10** neu zentrieren.
6. Ein Menü öffnen und schließen. Wechseln Menüfläche und Weltansicht korrekt?
7. Wenn alles angenehm bleibt, länger spielen (beispielsweise 15 Minuten).
   Tatsächliche Dauer und Flackern, Abstürze, Tracking- oder HUD-Probleme notieren.

Bei Flackern oder Unwohlsein die Brille absetzen und FS25 normal beenden. Nach
Sitzungsverlust kann ein Spielneustart nötig sein. Ein absichtlicher Verbindungsabbruch
ist für den ersten Test nicht erforderlich; automatisches Wiederverbinden ist nicht zugesagt.

### Ergebnis melden

Bitte auch erfolgreiche Tests melden: Spiel-/Mod-Version, Brille, Verbindung/
Laufzeit, GPU/Treiber, Windows, andere aktive Mods, Spieldauer und Ergebnisse der
einzelnen Schritte. Nicht geprüfte Punkte ausdrücklich so kennzeichnen.
GitHub-Meldungen sind öffentlich; echter Name und E-Mail werden nicht benötigt.

Logs vor dem Anhängen auf persönliche Angaben prüfen. Der eigene Adapter schreibt
`x64/fs25-mr_scarfacevr.log`; das normale FS25-Log liegt im persönlichen Spielordner.
Ein Diagnose-Augenbild kann Spielinhalte zeigen und wird nur bei Bedarf benötigt.
Der Adapter lädt nichts automatisch hoch und enthält keine Telemetrie.
