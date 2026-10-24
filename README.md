# MR_ScarfaceVR for FS25

**Play Farming Simulator 25 in VR with stereo rendering and head tracking.**
A free, unofficial, experimental **Mod for Farming Simulator**, maintained by **MR_Scarface**.

[**Download the test package**](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/releases/tag/v0.1.0.1-experimental) ·
[**Report a test or problem**](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/issues/new?template=test_report.md) ·
[**Deutsch**](README.de.md)

## Testers wanted

Looking for FS25 players with a Quest 2 and Meta Link to try the same build on
other PCs. Successful tests are useful too: please share your setup and results
using the [test report](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/issues/new?template=test_report.md).
The [short test guide](TESTING.md#english) covers menus, walking, vehicle cabs,
head movement, normal controls and recentering. No automatic telemetry.

**Version 0.1.0.1 is unsigned and experimental.** One user on one PC confirmed
these functions on 24 October 2026. Other PCs/headsets and longer sessions remain
unverified. This is not a claim of general compatibility.

## What you get

- Stereo views for both eyes from the same simulation frame.
- Head rotation and positional tracking in first-person and vehicle-cab views.
- Your normal keyboard/mouse, gamepad or steering-wheel controls; no VR hands.
- A flat menu panel and a recenter key. Single-player only.

## Requirements

| Component | Current tested setup |
| --- | --- |
| Game | **FS25 1.25.0.0**, exact supported executable; other builds are rejected |
| PC | Windows x64, Direct3D 12 |
| Headset/runtime | **Quest 2 connected through Meta Quest Link** |
| Installation | Own DLL beside the game executable **and** a ZIP in the mods folder |

Meta Quest Link must be installed separately and working. No SteamVR or existing
VR mod is required for this setup. Quest standalone, consoles and other runtimes
are not confirmed. Use your own legitimate game installation; no game files are distributed.

## Install and play

1. Close FS25. Download `MR_ScarfaceVR_FS25_v0.1.0.1_Experimental.zip` from the
   [release page](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/releases/tag/v0.1.0.1-experimental)
   and extract the outer package. Read its `START_HIER.txt`.
2. Copy `x64/dinput8.dll` beside `x64/FarmingSimulator2025Game.exe` in the game
   installation. **Do not overwrite a DLL belonging to another mod.**
3. Copy `mods/FS25_MR_ScarfaceVR.zip`, still zipped, into your personal FS25
   `mods` folder, normally `Documents/My Games/FarmingSimulator2025/mods/`.
   When upgrading from our old pilot, remove or disable `FS25_OwnVR.zip`;
   do not enable both variants.
4. Connect Quest Link, start FS25 and enable **MR_ScarfaceVR Experimentell**
   when loading a save. Start with the main menu before testing the world.

| Key | Action |
| --- | --- |
| **F11** | Start VR; press again to request stopping it. VR starts **off**. |
| **F10** | Recenter head position and direction. |
| F9 | Optional desktop stereo diagnostic; not needed for headset VR. |

Runtime startup took about **30 seconds** in the pilot; the game window can stop
responding temporarily. First-person and cabs use the VR world view; menus and
external views use a flat panel. If the image flickers or feels uncomfortable,
remove the headset and exit FS25 normally. Session loss can require restarting the game.

For the full German instructions see [Installation und Bedienung](INSTALLATION.md).
For a practical checklist in either language see [TESTING.md](TESTING.md).

## Remove

Close FS25 and remove only this project's installed `x64/dinput8.dll` and
`mods/FS25_MR_ScarfaceVR.zip`. Do not remove original game files or another mod's DLL.
The adapter does not patch the game executable on disk. Optional local diagnostics
are `fs25-mr_scarfacevr.log` and `fs25-mr_scarfacevr-eye.bmp` in the game `x64` folder.

## Code signing policy

The pilot DLL is **unsigned**. The SignPath Foundation application was submitted
on 24 October 2026; acceptance and signing are **pending**. Windows Smart App
Control may block the DLL. Do not disable security protections as an installation step.
[Code signing policy](CODE_SIGNING_POLICY.md) · [Privacy](PRIVACY.md).

## Source and development

Own sources use the [MIT license](LICENSE). Official Khronos OpenXR headers retain
their MIT notices; see [third-party notices](THIRD_PARTY.md). No game or Meta
binaries are included. This project was developed with AI assistance; the maintainer
is responsible for review, maintenance and releases. No GIANTS or Meta endorsement
or official partnership is claimed.

The current release links its source commit and includes `source.zip`, a manifest
and SHA256 checksums. Release assets are not silently replaced. Public builds run
[on GitHub Actions](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/actions).

Build with an existing MSVC x64/Windows SDK, PowerShell and Python:

```powershell
./build.ps1
python -m unittest discover -s tests -v
```

`./build.ps1 -Target gpu` performs a GPU pixel-copy check without starting VR.
Native/automated tests do not replace actual game/headset tests. The longer German
[project notes](README.de.md#bauen-und-prüfen) describe the diagnostic build targets.
