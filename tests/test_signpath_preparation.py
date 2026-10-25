import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile
import xml.etree.ElementTree as ET

from tools.prepare_signpath import PUBLIC_FILES, public_files, create_archive


ROOT = Path(__file__).resolve().parents[1]


class SignPathPreparationTests(unittest.TestCase):
    def test_public_brand_and_signing_metadata_agree(self):
        config = ET.fromstring((ROOT / 'signing/artifact-configuration.xml').read_bytes())
        pe = config.find('.//{http://signpath.io/artifact-configuration/v1}pe-file')
        mod = ET.fromstring((ROOT / 'mod/FS25_MR_ScarfaceVR/modDesc.xml').read_bytes())
        self.assertEqual(pe.get('product-name'), 'MR_ScarfaceVR for FS25')
        self.assertEqual(pe.get('company-name'), 'MR_Scarface')
        self.assertEqual(pe.get('product-version'), mod.findtext('version'))
        self.assertEqual(pe.get('file-version'), mod.findtext('version'))
        self.assertEqual(mod.findtext('author'), 'MR_Scarface')
        self.assertIn('MR_ScarfaceVR', mod.findtext('title/en'))

    def test_public_snapshot_is_explicit_and_has_no_private_files(self):
        files = public_files(ROOT)
        self.assertEqual(set(files), set(PUBLIC_FILES))
        self.assertIn('native/version.rc', files)
        self.assertIn('native/terrain_camera.h', files)
        self.assertIn('tools/fetch_openxr_loader.ps1', files)
        loader = json.loads(files['third_party/openxr-loader/ORIGIN.json'])
        self.assertEqual(loader['tag'], 'release-1.1.65')
        layer = json.loads(files['mod/MR_ScarfaceVR_OpenXR/ReShade.json'])['api_layer']
        self.assertEqual(layer['name'], 'XR_APILAYER_reshade')
        self.assertEqual(layer['library_path'], '..\\dxgi.dll')
        self.assertFalse(any(name.lower().endswith(('dxgi.dll', 'reshade.dll')) for name in files))
        self.assertIn('LICENSE', files)
        self.assertIn('.github/workflows/build-and-sign.yml', files)
        self.assertFalse(any(name.startswith(('.analysis/', 'build/', 'dist/')) for name in files))
        origin = json.loads(files['third_party/openxr/ORIGIN.json'])
        self.assertTrue(all(not Path(item['Path']).is_absolute() for item in origin['files']))

    def test_missing_public_file_is_an_error(self):
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(FileNotFoundError):
                public_files(Path(folder))

    def test_public_file_cannot_escape_root(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            external = root.parent / 'external.txt'
            real_resolve = Path.resolve
            # Model a link resolved outside the tree without requiring Windows symlink privileges.
            def resolve(path, *args, **kwargs):
                if path == root / PUBLIC_FILES[0]:
                    return external
                return real_resolve(path, *args, **kwargs)
            with patch.object(Path, 'resolve', resolve):
                with self.assertRaises(ValueError):
                    public_files(root)

    def test_archive_has_only_public_source_and_application_documents(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / 'preparation.zip'
            create_archive(ROOT, output)
            expected = public_files(ROOT)
            with zipfile.ZipFile(output) as archive:
                self.assertIsNone(archive.testzip())
                self.assertEqual(set(archive.namelist()),
                                 {'repository/' + name for name in expected} |
                                 {'START_HIER_SIGNPATH.md', 'APPLICATION_DRAFT.md', 'SHA256SUMS.txt'})
                for name, content in expected.items():
                    self.assertEqual(archive.read('repository/' + name), content)
                self.assertFalse(any('..' in Path(name).parts for name in archive.namelist()))


if __name__ == '__main__':
    unittest.main()
