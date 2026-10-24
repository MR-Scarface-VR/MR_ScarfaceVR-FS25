# Privacy

MR_ScarfaceVR's own native and Lua code implements no telemetry, account service or
network upload. Logs and captured images remain on the user's PC unless the
user chooses to share them.

The adapter writes `fs25-mr_scarfacevr.log` beside its DLL and can write a diagnostic
eye image named `fs25-mr_scarfacevr-eye.bmp` there. The image can show game content.
FS25 also produces its normal profile log. Before sharing logs or images,
remove personal details such as player names and local paths.

MR_ScarfaceVR calls the separately installed Meta runtime and runs inside FS25.
Network activity and data processing by Meta Quest Link, Farming Simulator,
Steam or Windows are outside the adapter's own implementation and subject to
those products' privacy policies, including the [Meta privacy policy](https://www.meta.com/legal/privacy-policy/).

If a user submits an issue or chooses to download from GitHub, GitHub processes
that interaction under its [privacy statement](https://docs.github.com/en/site-policy/privacy-policies/github-general-privacy-statement).
No logs or images are automatically attached to an issue.
