"""Create a public source/application snapshot using an explicit file allowlist."""
import argparse
import hashlib
from pathlib import Path
import zipfile


PUBLIC_FILES = (
    '.gitignore', 'LICENSE', 'README.md', 'README.de.md', 'TESTING.md',
    'INSTALLATION.md', 'THIRD_PARTY.md', 'PRIVACY.md',
    'CODE_SIGNING_POLICY.md', 'build.ps1',
    '.github/workflows/build-and-sign.yml',
    '.github/ISSUE_TEMPLATE/test_report.md',
    'signing/artifact-configuration.xml', 'signing/SETUP.md', 'signing/APPLICATION_DRAFT.md',
    'native/proxy.cpp', 'native/dinput8.def', 'native/version.rc',
    'native/engine_probe.cpp', 'native/engine_probe.h', 'native/engine_addresses.h',
    'native/engine_hooks.h', 'native/stereo.h', 'native/d3d12_copy.h', 'native/terrain_camera.h',
    'native/openxr_bridge.cpp', 'native/openxr_bridge.h',
    'mod/FS25_MR_ScarfaceVR/modDesc.xml', 'mod/FS25_MR_ScarfaceVR/VR.lua',
    'tests/native_core_tests.cpp', 'tests/openxr_flow_tests.cpp',
    'tests/proxy_smoke.cpp', 'tests/runtime_probe.cpp', 'tests/gpu_copy_tests.cpp',
    'tests/version_info_tests.ps1', 'tests/test_signpath_preparation.py',
    'tools/prepare_signpath.py',
    'third_party/openxr/ORIGIN.json', 'third_party/openxr/LICENSE-MIT.txt',
    'third_party/openxr/include/openxr/openxr.h',
    'third_party/openxr/include/openxr/openxr_platform.h',
    'third_party/openxr/include/openxr/openxr_platform_defines.h',
    'third_party/openxr/include/openxr/openxr_loader_negotiation.h',
)


def public_files(root: Path) -> dict[str, bytes]:
    root = root.resolve()
    result = {}
    for name in PUBLIC_FILES:
        path = root / name
        if not path.resolve().is_relative_to(root):
            raise ValueError(f'Public source path escapes root: {name}')
        result[name] = path.read_bytes()
    return result


def create_archive(root: Path, output: Path) -> None:
    files = {'repository/' + name: data for name, data in public_files(root).items()}
    files['START_HIER_SIGNPATH.md'] = files['repository/signing/SETUP.md']
    files['APPLICATION_DRAFT.md'] = files['repository/signing/APPLICATION_DRAFT.md']
    sums = ''.join(f'{hashlib.sha256(data).hexdigest()}  {name}\n'
                   for name, data in sorted(files.items()))
    files['SHA256SUMS.txt'] = sums.encode('ascii')
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(output.suffix + '.new')
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            archive.writestr(name, data)
    with zipfile.ZipFile(temporary) as archive:
        if archive.testzip() is not None:
            raise ValueError('Prepared archive failed ZIP integrity check')
        for name, data in files.items():
            if archive.read(name) != data:
                raise ValueError(f'Prepared archive content mismatch: {name}')
    temporary.replace(output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('dist/MR_ScarfaceVR_FS25_SignPath_Preparation.zip'))
    args = parser.parse_args()
    create_archive(Path(__file__).resolve().parents[1], args.output)
    print(f'Prepared source/application snapshot: {args.output.resolve()}')
