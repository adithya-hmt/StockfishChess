import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from patch_manifest import patch_manifest, inspect_manifest

src = ROOT / "template" / "AndroidManifest.xml"
out = ROOT / "build" / "test-AndroidManifest.xml"
out.parent.mkdir(parents=True, exist_ok=True)
patch_manifest(src, out)
info = inspect_manifest(out.read_bytes())

assert info["package"] == "com.framilton.chess", info
assert info["application_label"] == "Framilton Chess", info
assert info["activity_label"] == "Framilton Chess", info
assert info["lib_name"] == "sf_chess", info
assert info["min_sdk"] == 29, info
assert info["target_sdk"] == 29, info
assert info["debuggable"] is False, info
assert info["permissions"] == [], info
print("manifest patch test: PASS")
