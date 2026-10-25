# MR_ScarfaceVR for FS25

**Play Farming Simulator 25 in VR with stereo rendering and head tracking.**
A free, unofficial, experimental **Mod for Farming Simulator**, maintained by **MR_Scarface**.

**Version 0.1.0.4 adds optional ReShade support in VR.** On 25 October 2026
one Quest 2 / Meta Link user confirmed the selected effect in both eyes and
correct head tracking. ReShade 6.8.0.2155 was used; other shaders and PCs remain
unverified. Some installed shader packs report sampler-descriptor errors. [Test scope](docs/RESHade_TEST_0.1.0.4.md).

**Version 0.1.0.3 — terrain fix:** terrain virtual-texture detail follows VR head
tracking. One user confirmed the terrain comparison on Quest 2 / Meta Link on
25 October 2026; live logs confirm tracking reached the terrain worker. Automated
checks pass. Broader regression testing and other PCs/headsets remain unverified.
In 0.1.0.1, terrain could stay blurry outside the mouse-camera direction.
Version 0.1.0.3 supplies headset tracking to the terrain worker. See the [terrain comparison test](TESTING.md#terrain-comparison--gelände-vergleich).

[**Download the test package**](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/releases/tag/v0.1.0.4-experimental) ·
[**Report a test or problem**](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/issues/new?template=test_report.md) ·
[**Deutsch**](README.de.md)

## Testers wanted

Looking for FS25 players with a Quest 2 and Meta Link to try the same build on
other PCs. Successful tests are useful too: please share your setup and results
using the [test report](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/issues/new?template=test_report.md).
The [short test guide](TESTING.md#english) covers menus, walking, vehicle cabs,
head movement, normal controls and recentering. No automatic telemetry.

**Version 0.1.0.4 is unsigned and experimental.** The earlier 0.1.0.1 passed
a short check of the functions below on one PC on 24 October 2026. The 0.1.0.3
headset test specifically confirmed the terrain fix on the same setup. Other PCs/headsets and longer sessions remain
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
| Installation | Own DLL, bundled OpenXR loader and layer folder beside the game executable **and** a ZIP in the mods folder |

Meta Quest Link must be installed separately and working. No SteamVR or existing
VR mod is required for this setup. Quest standalone, consoles and other runtimes
are not confirmed. Use your own legitimate game installation; no game files are distributed.

## Install and play

1. Close FS25. Download `MR_ScarfaceVR_FS25_v0.1.0.4_Experimental.zip` from the
   [release page](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/releases/tag/v0.1.0.4-experimental)
   and extract the outer package. Read its `START_HIER.txt`.
2. Copy `x64/dinput8.dll`, `x64/openxr_loader.dll` and the
   `x64/MR_ScarfaceVR_OpenXR/` folder beside `x64/FarmingSimulator2025Game.exe`.
   **Do not overwrite files belonging to another mod.** When upgrading, update
   both DLLs, the layer folder and the Lua ZIP below.
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

## Optional ReShade

ReShade is separately installed by the player and is not bundled. The tested
installation uses its `dxgi.dll` beside the game executable. Keep that DLL, its
shaders and presets; do not replace our `dinput8.dll` with ReShade. No global
ReShade OpenXR-layer installation is needed for our local integration.

Effects apply to the stereo world on foot/in cabs; flat menus are not processed
through this path. ReShade keeps VR settings in `ReShadeVR.ini`; select the
desired preset there if it differs from the desktop preset. Start with a simple
color effect and check both eyes. Depth effects and arbitrary screen-space
shaders are not generally confirmed. [Detailed test notes](docs/RESHade_TEST_0.1.0.4.md).

## Remove

Close FS25 and remove only this project's installed `x64/dinput8.dll`,
`x64/openxr_loader.dll`, `x64/MR_ScarfaceVR_OpenXR/` folder and
`mods/FS25_MR_ScarfaceVR.zip`. Keep the separately installed ReShade files. Do not remove original game files or another mod's DLL.
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
and SHA256 checksums. Release assets are not silently replaced. The distributed DLL was built locally with MSVC; its exact tested bytes are
preserved. Source builds and automated checks also run
[on GitHub Actions](https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/actions).

Build with an existing MSVC x64/Windows SDK, PowerShell and Python. The build
fetches a pinned official Khronos loader package and verifies its checksums;
this requires network access on the first build:

```powershell
./build.ps1
python -m unittest discover -s tests -v
```

`./build.ps1 -Target gpu` performs a GPU pixel-copy check without starting VR.
Native/automated tests do not replace actual game/headset tests. The longer German
[project notes](README.de.md#bauen-und-prüfen) describe the diagnostic build targets.
