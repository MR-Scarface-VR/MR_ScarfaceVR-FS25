# SignPath Foundation application draft — NOT SUBMITTED

Replace all remaining bracketed fields before submitting. Do not claim
that planned account security, public releases or CI runs already exist.

## Applicant details

- Public maintainer alias: MR_Scarface
- Project contact email: provided privately in the application form; not published here.
- GitHub account: MR-Scarface-VR
- Project: MR_ScarfaceVR for FS25
- Repository: https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25
- Homepage: https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25#readme
- Release page: https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/releases
- Public successful workflow run: https://github.com/MR-Scarface-VR/MR_ScarfaceVR-FS25/actions/runs/40115625932
- License: MIT for our own code; official Khronos OpenXR headers retain MIT notices.
- Maintainers / reviewers / signing approvers: MR_Scarface (@MR-Scarface-VR), initial single-maintainer project
- GitHub MFA: enabled and confirmed. SignPath account/MFA: to configure after onboarding.

Public branding is MR_Scarface / MR_ScarfaceVR for FS25. If the application
process requires a private legal name or other personal verification data,
provide it directly to the provider, not in the public repository. The signer
under the Foundation program would remain SignPath Foundation.

## Project description

MR_ScarfaceVR for FS25 is an experimental, unofficial Windows x64 VR adapter for Farming
Simulator 25. It provides same-simulation-frame stereo and head tracking through
the separately installed Meta Quest Link OpenXR runtime, while players retain
normal keyboard/mouse, gamepad or wheel controls. It is a single-player mod.

The exact supported game is FS25 1.25.0.0. An initial Quest 2 user has confirmed
walking, vehicle-cab play, head rotation/translation, recentering and menu changes.
There is not yet an established public user community or independent multi-PC
test history. The source repository and successful hosted build are public. The current
0.1.0.1 unsigned pilot also passed the short Quest 2 menu/world/tracking/controls/
recenter test on one PC. See the release page for experimental package availability.

The project was developed with AI assistance. The maintainer is responsible for
review, maintenance and release approval; AI assistance is not independent review.

## Native integration disclosure — please assess eligibility

The artifact to sign is our own `dinput8.dll`, loaded beside the game's executable.
It forwards DirectInput to the real Windows System32 implementation. Before
enabling integration it checks the game's exact executable identity and native
entry-point expectations. It adjusts renderer view-provider slots, hooks the
game's swap-chain Present slot, and replaces a game-local QueryPerformanceCounter
import slot to register Lua callbacks at a safe VM boundary. No Windows system
DLL or game executable is modified on disk. The signature would apply to this
original adapter only, not to the host game or Meta runtime.

Farming Simulator and Meta Quest Link are proprietary external requirements,
not bundled dependencies. No game executables, engine memory captures, assets,
Meta binaries or third-party VR mod code are submitted. Local read-only engine
investigation utilities are not part of the public build or signing artifact.

Please confirm whether this proprietary-host/plugin integration is eligible.
We do not present your signature as approval by GIANTS/Meta or as a guarantee
of game stability. We understand that project reputation may be insufficient
for acceptance at this early stage.

## Build, release and privacy

The prepared workflow builds with MSVC/Windows SDK on GitHub-hosted Windows
runners and runs native tests, OpenXR protocol tests, a DirectInput smoke test,
VERSIONINFO checks and Python packaging tests without needing the game or a
headset. Real hardware/headset tests are separate and are not claimed as CI checks.

The signing input is an uploaded GitHub artifact containing only our DLL. The
artifact configuration restricts product name/version and original filename.
Each production signature will require manual approval under your signing policy.
We will verify the returned signature before distribution.

MR_ScarfaceVR itself implements no telemetry or automatic log/image upload. A local log
and diagnostic eye image may be written. The external host/runtime have their
own privacy policies. Details: repository PRIVACY.md and CODE_SIGNING_POLICY.md.

The mod download will remain free, with optional voluntary support and possibly
videos about the project; there will be no paid mod access or commercial
dual-licensing. Please advise if further information about this model is needed.

## Evidence to attach after setup

- Public MIT-licensed repository and truthful unsigned experimental release.
- Successful hosted build URL and exact artifact SHA256.
- README, installation/removal instructions, privacy and code signing policy.
- Actual maintainer roles, MFA status and available public test reports.

Application entry point: https://signpath.org/apply.html
