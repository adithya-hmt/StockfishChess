#!/usr/bin/env python3
from __future__ import annotations
import argparse
import pathlib

OLD_PACKAGE = 'org.yourorg.cnfgtest'
NEW_PACKAGE = 'com.local.sfchessapp'
OLD_LABEL = 'cnfgtest'
NEW_LABEL = 'SF Chess'


def _replace_exact(data: bytes, old: bytes, new: bytes, expected: int, label: str) -> bytes:
    if len(old) != len(new):
        raise ValueError(f'{label}: replacement must preserve byte length')
    count = data.count(old)
    if count != expected:
        raise ValueError(f'{label}: expected {expected} occurrence(s), found {count}')
    return data.replace(old, new)


def patch_resources(source: pathlib.Path, output: pathlib.Path) -> None:
    data = source.read_bytes()
    data = _replace_exact(data, OLD_PACKAGE.encode(), NEW_PACKAGE.encode(), 1, 'ASCII package')
    data = _replace_exact(data, OLD_PACKAGE.encode('utf-16le'), NEW_PACKAGE.encode('utf-16le'), 1, 'UTF-16 package')
    data = _replace_exact(data, OLD_LABEL.encode(), NEW_LABEL.encode(), 1, 'ASCII label')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path)
    args = parser.parse_args()
    patch_resources(args.source, args.output)

if __name__ == '__main__':
    main()
