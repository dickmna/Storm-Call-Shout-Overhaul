"""Patch the fixed-size SCSO controller VMAD values without xEdit.

The tool intentionally changes only existing Int properties, so record and group
sizes remain byte-for-byte stable. It is used to build the v2.1.0 runtime ESP
from the v1.6.1 release ESP.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


COMPRESSED_FLAG = 0x00040000
SCRIPT_NAME = "ultrastormcallunified"

PASS_COUNTS = {
    0x000A1A58: 1,
    0x000A1A5C: 1,
    0x000A1A5B: 1,
    0x000E3F0A: 1,
    0x000E3F09: 2,
    0x000D5E81: 3,
}


def u16(data: bytes | bytearray, pos: int) -> int:
    return struct.unpack_from("<H", data, pos)[0]


def u32(data: bytes | bytearray, pos: int) -> int:
    return struct.unpack_from("<I", data, pos)[0]


def read_wstring(data: bytes | bytearray, pos: int) -> tuple[str, int]:
    length = u16(data, pos)
    pos += 2
    value = bytes(data[pos : pos + length]).decode("utf-8")
    return value, pos + length


def skip_vmad_value(
    data: bytes | bytearray, pos: int, type_code: int
) -> int:
    if type_code == 1:
        return pos + 8
    if type_code == 2:
        _, pos = read_wstring(data, pos)
        return pos
    if type_code in (3, 4):
        return pos + 4
    if type_code == 5:
        return pos + 1
    if 11 <= type_code <= 15:
        count = u32(data, pos)
        pos += 4
        item_type = type_code - 10
        for _ in range(count):
            pos = skip_vmad_value(data, pos, item_type)
        return pos
    raise ValueError(f"Unsupported VMAD property type {type_code} at 0x{pos:X}")


def iter_records(data: bytes | bytearray, start: int = 0, end: int | None = None):
    if end is None:
        end = len(data)
    pos = start
    while pos + 24 <= end:
        signature = bytes(data[pos : pos + 4])
        size = u32(data, pos + 4)
        if signature == b"GRUP":
            if size < 24 or pos + size > end:
                raise ValueError(f"Malformed GRUP at 0x{pos:X}")
            yield from iter_records(data, pos + 24, pos + size)
            pos += size
            continue

        record_end = pos + 24 + size
        if record_end > end:
            raise ValueError(f"Malformed {signature!r} record at 0x{pos:X}")
        yield {
            "signature": signature,
            "form_id": u32(data, pos + 12),
            "flags": u32(data, pos + 8),
            "payload_start": pos + 24,
            "payload_end": record_end,
        }
        pos = record_end


def iter_subrecords(data: bytes | bytearray, start: int, end: int):
    pos = start
    extended_size = None
    while pos + 6 <= end:
        signature = bytes(data[pos : pos + 4])
        size = u16(data, pos + 4)
        pos += 6
        if signature == b"XXXX":
            if size != 4:
                raise ValueError(f"Malformed XXXX at 0x{pos - 6:X}")
            extended_size = u32(data, pos)
            pos += 4
            continue
        if extended_size is not None:
            size = extended_size
            extended_size = None
        value_start = pos
        value_end = pos + size
        if value_end > end:
            raise ValueError(f"Malformed {signature!r} subrecord at 0x{pos - 6:X}")
        yield signature, value_start, value_end
        pos = value_end


def patch_vmad_ints(
    data: bytearray, start: int, end: int, form_id: int
) -> dict[str, tuple[int, int]]:
    pos = start
    _version = u16(data, pos)
    _object_format = u16(data, pos + 2)
    script_count = u16(data, pos + 4)
    pos += 6
    changes: dict[str, tuple[int, int]] = {}

    for _ in range(script_count):
        script_name, pos = read_wstring(data, pos)
        pos += 1  # script status
        property_count = u16(data, pos)
        pos += 2
        for _ in range(property_count):
            property_name, pos = read_wstring(data, pos)
            type_code = data[pos]
            pos += 2  # type and property status
            value_pos = pos
            pos = skip_vmad_value(data, pos, type_code)

            if script_name.lower() != SCRIPT_NAME:
                continue
            if property_name == "iTargetsPerUpdate":
                new_value = 0
            elif property_name == "iActiveSearchPasses":
                new_value = PASS_COUNTS[form_id]
            else:
                continue
            if type_code != 3:
                raise ValueError(
                    f"{property_name} on {form_id:08X} is not a VMAD Int"
                )
            old_value = struct.unpack_from("<i", data, value_pos)[0]
            struct.pack_into("<i", data, value_pos, new_value)
            changes[property_name] = (old_value, new_value)

    if pos > end:
        raise ValueError(f"VMAD parser exceeded {form_id:08X} subrecord")
    return changes


def patch_esp(source: Path, destination: Path) -> None:
    data = bytearray(source.read_bytes())
    patched: dict[int, dict[str, tuple[int, int]]] = {}

    for record in iter_records(data):
        form_id = record["form_id"]
        if form_id not in PASS_COUNTS or record["signature"] != b"MGEF":
            continue
        if record["flags"] & COMPRESSED_FLAG:
            raise ValueError(f"Target MGEF {form_id:08X} is compressed")
        for signature, start, end in iter_subrecords(
            data, record["payload_start"], record["payload_end"]
        ):
            if signature == b"VMAD":
                patched[form_id] = patch_vmad_ints(data, start, end, form_id)
                break

    if set(patched) != set(PASS_COUNTS):
        missing = set(PASS_COUNTS) - set(patched)
        raise ValueError(
            "Missing controller VMAD records: "
            + ", ".join(f"{form_id:08X}" for form_id in sorted(missing))
        )
    for form_id, changes in patched.items():
        if set(changes) != {"iTargetsPerUpdate", "iActiveSearchPasses"}:
            raise ValueError(f"Incomplete VMAD properties on {form_id:08X}: {changes}")

    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(data)
    for form_id in sorted(patched):
        print(f"{form_id:08X}: {patched[form_id]}")
    print(destination)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    patch_esp(args.source, args.destination)


if __name__ == "__main__":
    main()
