#!/usr/bin/env python3
"""Patch the NativeActivity template manifest for Framilton Chess."""
from __future__ import annotations

import argparse
import pathlib
import struct
from typing import Any

RES_XML_TYPE = 0x0003
RES_STRING_POOL_TYPE = 0x0001
RES_XML_RESOURCE_MAP_TYPE = 0x0180
RES_XML_START_ELEMENT_TYPE = 0x0102
RES_XML_END_ELEMENT_TYPE = 0x0103
TYPE_STRING = 0x03
TYPE_INT_DEC = 0x10
TYPE_INT_BOOLEAN = 0x12
NO_INDEX = 0xFFFFFFFF


def _read_utf16_length(data: bytes, pos: int) -> tuple[int, int]:
    first = struct.unpack_from("<H", data, pos)[0]
    pos += 2
    if first & 0x8000:
        second = struct.unpack_from("<H", data, pos)[0]
        pos += 2
        return ((first & 0x7FFF) << 16) | second, pos
    return first, pos


def _encode_utf16_length(length: int) -> bytes:
    if length <= 0x7FFF:
        return struct.pack("<H", length)
    return struct.pack("<HH", 0x8000 | ((length >> 16) & 0x7FFF), length & 0xFFFF)


def _parse_string_pool(data: bytes, offset: int = 8) -> tuple[list[str], int]:
    chunk_type, header_size, chunk_size = struct.unpack_from("<HHI", data, offset)
    if chunk_type != RES_STRING_POOL_TYPE or header_size != 28:
        raise ValueError("expected UTF-16 Android string pool")
    string_count, style_count, flags, strings_start, styles_start = struct.unpack_from(
        "<IIIII", data, offset + 8
    )
    if flags & 0x100:
        raise ValueError("template string pool unexpectedly uses UTF-8")
    if style_count != 0 or styles_start != 0:
        raise ValueError("template string pool unexpectedly contains styles")
    offsets = struct.unpack_from(f"<{string_count}I", data, offset + header_size)
    strings: list[str] = []
    for rel in offsets:
        pos = offset + strings_start + rel
        length, pos = _read_utf16_length(data, pos)
        raw = data[pos : pos + length * 2]
        strings.append(raw.decode("utf-16le"))
    return strings, chunk_size


def _build_string_pool(strings: list[str]) -> bytes:
    encoded = bytearray()
    offsets: list[int] = []
    for value in strings:
        offsets.append(len(encoded))
        raw = value.encode("utf-16le")
        encoded += _encode_utf16_length(len(value))
        encoded += raw
        encoded += b"\x00\x00"
    while len(encoded) % 4:
        encoded.append(0)

    header_size = 28
    strings_start = header_size + 4 * len(strings)
    chunk_size = strings_start + len(encoded)
    out = bytearray()
    out += struct.pack("<HHI", RES_STRING_POOL_TYPE, header_size, chunk_size)
    out += struct.pack("<IIIII", len(strings), 0, 0, strings_start, 0)
    out += struct.pack(f"<{len(offsets)}I", *offsets)
    out += encoded
    return bytes(out)


def _string(strings: list[str], index: int) -> str | None:
    if index == NO_INDEX:
        return None
    return strings[index]


def _start_element_info(chunk: bytearray | bytes, strings: list[str]) -> tuple[str, list[dict[str, int]]]:
    name_index = struct.unpack_from("<I", chunk, 20)[0]
    attr_start, attr_size, attr_count = struct.unpack_from("<HHH", chunk, 24)
    attrs: list[dict[str, int]] = []
    base = 16 + attr_start
    for i in range(attr_count):
        pos = base + i * attr_size
        ns, name, raw = struct.unpack_from("<III", chunk, pos)
        value_size, res0, data_type, data_value = struct.unpack_from("<HBBI", chunk, pos + 12)
        attrs.append(
            {
                "pos": pos,
                "ns": ns,
                "name": name,
                "raw": raw,
                "data_type": data_type,
                "data_value": data_value,
            }
        )
    return strings[name_index], attrs


def _patch_attr_string(chunk: bytearray, attr: dict[str, int], index: int) -> None:
    struct.pack_into("<I", chunk, attr["pos"] + 8, index)
    struct.pack_into("<I", chunk, attr["pos"] + 16, index)


def _patch_attr_int(chunk: bytearray, attr: dict[str, int], value: int) -> None:
    struct.pack_into("<I", chunk, attr["pos"] + 16, value & 0xFFFFFFFF)


def patch_manifest(source: pathlib.Path, output: pathlib.Path) -> None:
    data = source.read_bytes()
    root_type, root_header, root_size = struct.unpack_from("<HHI", data, 0)
    if root_type != RES_XML_TYPE or root_header != 8 or root_size != len(data):
        raise ValueError("invalid binary Android manifest template")

    strings, old_pool_size = _parse_string_pool(data, 8)
    expected = {
        19: "org.yourorg.cnfgtest",
        22: "uses-sdk",
        23: "uses-permission",
        25: "application",
        26: "cnfgtest",
        27: "activity",
        29: "meta-data",
        30: "android.app.lib_name",
    }
    for index, value in expected.items():
        if strings[index] != value:
            raise ValueError(f"unexpected template string {index}: {strings[index]!r}")

    strings[19] = "com.framilton.knight"
    strings[26] = "sf_chess"
    label_index = len(strings)
    strings.append("Framilton Chess")
    new_pool = _build_string_pool(strings)

    old_rest = data[8 + old_pool_size :]
    patched_chunks: list[bytes] = []
    offset = 0
    skip_permission_end = False
    while offset < len(old_rest):
        chunk_type, header_size, chunk_size = struct.unpack_from("<HHI", old_rest, offset)
        if chunk_size < header_size or chunk_size <= 0:
            raise ValueError("invalid XML chunk size")
        chunk = bytearray(old_rest[offset : offset + chunk_size])
        offset += chunk_size

        if chunk_type == RES_XML_START_ELEMENT_TYPE:
            element, attrs = _start_element_info(chunk, strings)
            if element == "uses-permission":
                skip_permission_end = True
                continue

            for attr in attrs:
                attr_name = _string(strings, attr["name"])
                if element == "uses-sdk" and attr_name in {"minSdkVersion", "targetSdkVersion"}:
                    _patch_attr_int(chunk, attr, 29)
                elif element in {"application", "activity"} and attr_name == "label":
                    _patch_attr_string(chunk, attr, label_index)
                elif element == "application" and attr_name == "debuggable":
                    _patch_attr_int(chunk, attr, 0)

        elif chunk_type == RES_XML_END_ELEMENT_TYPE and skip_permission_end:
            name_index = struct.unpack_from("<I", chunk, 20)[0]
            if strings[name_index] != "uses-permission":
                raise ValueError("uses-permission start was not followed by its end element")
            skip_permission_end = False
            continue

        patched_chunks.append(bytes(chunk))

    if skip_permission_end:
        raise ValueError("unterminated uses-permission element")

    out = bytearray()
    out += struct.pack("<HHI", RES_XML_TYPE, 8, 0)
    out += new_pool
    out += b"".join(patched_chunks)
    struct.pack_into("<I", out, 4, len(out))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(out)


def inspect_manifest(data: bytes) -> dict[str, Any]:
    root_type, root_header, root_size = struct.unpack_from("<HHI", data, 0)
    if root_type != RES_XML_TYPE or root_header != 8 or root_size != len(data):
        raise ValueError("invalid binary Android manifest")
    strings, pool_size = _parse_string_pool(data, 8)
    info: dict[str, Any] = {
        "package": None,
        "application_label": None,
        "activity_label": None,
        "lib_name": None,
        "min_sdk": None,
        "target_sdk": None,
        "debuggable": None,
        "permissions": [],
    }
    offset = 8 + pool_size
    while offset < len(data):
        chunk_type, header_size, chunk_size = struct.unpack_from("<HHI", data, offset)
        chunk = data[offset : offset + chunk_size]
        offset += chunk_size
        if chunk_type != RES_XML_START_ELEMENT_TYPE:
            continue
        element, attrs = _start_element_info(chunk, strings)
        values: dict[str, tuple[str | None, int, int]] = {}
        for attr in attrs:
            name = _string(strings, attr["name"])
            raw_value = _string(strings, attr["raw"])
            typed_value: str | int | bool | None
            if attr["data_type"] == TYPE_STRING:
                typed_value = _string(strings, attr["data_value"])
            elif attr["data_type"] == TYPE_INT_BOOLEAN:
                typed_value = bool(attr["data_value"])
            else:
                typed_value = attr["data_value"]
            values[name or ""] = (raw_value, attr["data_type"], attr["data_value"])

        if element == "manifest" and "package" in values:
            info["package"] = values["package"][0]
        elif element == "uses-sdk":
            if "minSdkVersion" in values:
                info["min_sdk"] = values["minSdkVersion"][2]
            if "targetSdkVersion" in values:
                info["target_sdk"] = values["targetSdkVersion"][2]
        elif element == "uses-permission" and "name" in values:
            raw = values["name"][0]
            if raw is not None:
                info["permissions"].append(raw)
        elif element == "application":
            if "label" in values:
                info["application_label"] = values["label"][0]
            if "debuggable" in values:
                info["debuggable"] = bool(values["debuggable"][2])
        elif element == "activity":
            if "label" in values:
                info["activity_label"] = values["label"][0]
        elif element == "meta-data":
            name_raw = values.get("name", (None, 0, 0))[0]
            if name_raw == "android.app.lib_name":
                info["lib_name"] = values.get("value", (None, 0, 0))[0]
    return info


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--inspect", action="store_true")
    args = parser.parse_args()
    patch_manifest(args.source, args.output)
    if args.inspect:
        print(inspect_manifest(args.output.read_bytes()))


if __name__ == "__main__":
    main()
