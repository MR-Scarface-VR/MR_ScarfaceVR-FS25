# Third-party notices and external requirements

The project's own source code and documentation are licensed under [MIT](LICENSE).
This license does not grant rights to Farming Simulator, Meta software or their
trademarks. MR_ScarfaceVR is an unofficial **Mod for Farming Simulator**.

## Bundled API headers

`third_party/openxr/include/openxr/` contains official Khronos OpenXR API headers
from OpenXR-SDK release-1.0.34, commit
`288d3a7ebc1ad959f62d51da75baa3d27438c499`.
Their existing notices and [MIT license](third_party/openxr/LICENSE-MIT.txt)
are preserved. [Provenance and checksums](third_party/openxr/ORIGIN.json).
No third-party VR mod implementation is included.

## Bundled OpenXR loader (0.1.0.4)

Version 0.1.0.4 includes the unmodified official Khronos x64 `openxr_loader.dll`
from OpenXR-SDK release-1.1.65. The build fetches the pinned official package and
checks both package and DLL SHA256 values. See
[provenance](third_party/openxr-loader/ORIGIN.json),
[MIT license](third_party/openxr-loader/LICENSE-MIT.txt) and
[jsoncpp license](third_party/openxr-loader/LICENSE-jsoncpp.txt).
This is a bundled runtime library, not a separate program installation.

ReShade remains optional, separately installed by the player and not redistributed.
The local layer manifest refers to that existing `dxgi.dll`. No ReShade shaders,
presets or binaries are included in our mod package. Its license remains separate.

## External software, not redistributed

- Farming Simulator 25 is the proprietary host application, purchased separately.
- Meta Quest Link and its installed OpenXR runtime are required for VR output.
- Windows provides DirectInput and D3D12. The adapter forwards DirectInput to the
  actual Windows System32 DLL; it does not replace Windows system files.
- Development uses separately licensed MSVC/Windows SDK tools and Python.

The mod requires exactly the supported FS25 executable identity. It does not
distribute the executable, decrypted engine captures, game assets or Meta DLLs.
Runtime integration uses guarded pointers/slots in the loaded game process;
the game executable on disk is not patched. See [the application disclosure](signing/APPLICATION_DRAFT.md).
