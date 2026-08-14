import pathlib
import sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from patch_resources import patch_resources
src = ROOT / 'template' / 'resources.arsc'
out = ROOT / 'build' / 'test-resources.arsc'
patch_resources(src, out)
data = out.read_bytes()
assert b'org.yourorg.cnfgtest' not in data
assert 'org.yourorg.cnfgtest'.encode('utf-16le') not in data
assert data.count(b'com.cerelytic.knight') == 1
a = 'com.cerelytic.knight'.encode('utf-16le')
assert data.count(a) == 1
assert b'cnfgtest' not in data
assert 'cnfgtest'.encode('utf-16le') not in data
assert b'CerChess' in data
print('resource patch test: PASS')
