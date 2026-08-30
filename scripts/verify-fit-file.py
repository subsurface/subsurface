#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
# AI-generated (Claude)
"""
Standalone validator for Garmin-family FIT files produced by Subsurface's
FIT encoder (core/save-fit.cpp).

Checks the file header and framing against the wire-format facts documented
in core/save-fit.cpp: header_size, the ".FIT" magic, data_size consistency,
the header CRC (when present), and the trailing file CRC. The CRC-16 is
re-derived independently here (same table/algorithm as save-fit.cpp) rather
than importing anything from the encoder, so this script has no build-time
dependency on the C++ tree.

Usage:
    python3 scripts/verify-fit-file.py <path-to-fit-file>
    python3 scripts/verify-fit-file.py --selftest
"""

import struct
import sys

# Dynastream FIT CRC-16: a reflected nibble-table algorithm, NOT a generic
# CCITT CRC-16. Same table as core/save-fit.cpp's fit_crc_table.
FIT_CRC_TABLE = [
    0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
    0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400,
]


def crc16_byte(crc, byte):
    tmp = FIT_CRC_TABLE[crc & 0xF]
    crc = (crc >> 4) & 0x0FFF
    crc = crc ^ tmp ^ FIT_CRC_TABLE[byte & 0xF]
    tmp = FIT_CRC_TABLE[crc & 0xF]
    crc = (crc >> 4) & 0x0FFF
    crc = crc ^ tmp ^ FIT_CRC_TABLE[(byte >> 4) & 0xF]
    return crc


def crc16(data):
    crc = 0
    for byte in data:
        crc = crc16_byte(crc, byte)
    return crc


class FitValidationError(Exception):
    pass


def decode_manufacturer(data, header_size, data_size):
    """
    Best-effort decode of the manufacturer field (field 1) from the first
    file_id message (global message number 0). Returns the raw uint16
    value, or None if the message framing could not be decoded (e.g. a
    compressed-timestamp header or developer-data fields were encountered).
    Not required for validity -- this only serves the milestone's open
    question about which manufacturer id real devices produce (MEM002).
    """
    pos = header_size
    end = header_size + data_size
    definitions = {}  # local_type -> (global_msg, [(num, size), ...], little_endian)
    try:
        while pos < end:
            rec_header = data[pos]
            pos += 1
            if rec_header & 0x80:
                # Compressed timestamp header: not produced by our own
                # encoder and fiddly to interpret generically here.
                return None
            local_type = rec_header & 0x0F
            if rec_header & 0x40:
                has_dev_fields = bool(rec_header & 0x20)
                pos += 1  # reserved
                arch = data[pos]
                pos += 1
                little_endian = (arch == 0)
                fmt = "<H" if little_endian else ">H"
                global_msg = struct.unpack_from(fmt, data, pos)[0]
                pos += 2
                num_fields = data[pos]
                pos += 1
                fields = []
                for _ in range(num_fields):
                    num, size = data[pos], data[pos + 1]
                    fields.append((num, size))
                    pos += 3
                if has_dev_fields:
                    num_dev = data[pos]
                    pos += 1
                    pos += num_dev * 3
                definitions[local_type] = (global_msg, fields, little_endian)
            else:
                if local_type not in definitions:
                    return None  # data message before its definition: malformed or unsupported
                global_msg, fields, little_endian = definitions[local_type]
                field_start = pos
                total_size = sum(size for _, size in fields)
                if global_msg == 0:  # file_id
                    off = field_start
                    for num, size in fields:
                        if num == 1 and size == 2:
                            fmt = "<H" if little_endian else ">H"
                            return struct.unpack_from(fmt, data, off)[0]
                        off += size
                pos += total_size
        return None
    except (IndexError, struct.error):
        return None


def validate_bytes(data):
    """
    Run all framing checks against the raw bytes of a FIT file. Returns a
    dict of reported facts on success; raises FitValidationError naming the
    first failed check otherwise.
    """
    if len(data) < 12:
        raise FitValidationError(f"file too short to contain a FIT header ({len(data)} bytes)")

    header_size = data[0]
    if header_size not in (12, 14):
        raise FitValidationError(f"invalid header_size byte: {header_size} (expected 12 or 14)")

    if len(data) < header_size + 2:
        raise FitValidationError(
            f"file too short for declared header_size plus trailing CRC "
            f"({len(data)} < {header_size + 2})"
        )

    protocol_version = data[1]
    profile_version = struct.unpack_from("<H", data, 2)[0]
    data_size = struct.unpack_from("<I", data, 4)[0]
    magic = data[8:12]
    if magic != b".FIT":
        raise FitValidationError(f"missing .FIT magic at bytes 8..11: got {magic!r}")

    expected_total = header_size + data_size + 2
    if expected_total != len(data):
        raise FitValidationError(
            f"data_size inconsistent with file length: "
            f"header_size({header_size}) + data_size({data_size}) + 2 == {expected_total}, "
            f"but file is {len(data)} bytes"
        )

    if header_size == 14:
        header_crc = struct.unpack_from("<H", data, 12)[0]
        computed_header_crc = crc16(data[0:12])
        if header_crc != 0 and header_crc != computed_header_crc:
            raise FitValidationError(
                f"header CRC mismatch: file has 0x{header_crc:04x}, "
                f"computed 0x{computed_header_crc:04x}"
            )

    trailing_crc = struct.unpack_from("<H", data, len(data) - 2)[0]
    computed_file_crc = crc16(data[0:len(data) - 2])
    if trailing_crc != computed_file_crc:
        raise FitValidationError(
            f"file CRC mismatch: file has 0x{trailing_crc:04x}, "
            f"computed 0x{computed_file_crc:04x}"
        )

    manufacturer = decode_manufacturer(data, header_size, data_size)

    return {
        "file_size": len(data),
        "header_size": header_size,
        "protocol_version": protocol_version,
        "profile_version": profile_version,
        "data_size": data_size,
        "manufacturer": manufacturer,
    }


def validate_file(path):
    with open(path, "rb") as f:
        data = f.read()
    return validate_bytes(data)


def build_valid_fit_body():
    """A minimal file_id definition + data message pair, field layout
    matching core/save-fit.cpp's file_id message (type, manufacturer,
    product, serial_number, time_created)."""
    body = bytearray()
    body += bytes([0x40, 0x00, 0x00])  # def header (local type 0), reserved, architecture (LE)
    body += struct.pack("<H", 0)  # global message number: file_id
    fields = [(0, 1, 0x00), (1, 2, 0x84), (2, 2, 0x84), (3, 4, 0x8C), (4, 4, 0x86)]
    body.append(len(fields))
    for num, size, base_type in fields:
        body += bytes([num, size, base_type])

    body.append(0x00)  # data header (local type 0)
    body.append(4)  # type: activity
    body += struct.pack("<H", 23)  # manufacturer: Suunto
    body += struct.pack("<H", 0xFFFF)  # product: unknown (invalid sentinel)
    body += struct.pack("<I", 12345)  # serial_number
    body += struct.pack("<I", 1000000)  # time_created
    return bytes(body)


def build_fit_bytes(body):
    header_size = 14
    data_size = len(body)
    header = bytearray()
    header.append(header_size)
    header.append(16)  # protocol version
    header += struct.pack("<H", 2132)  # profile version (arbitrary)
    header += struct.pack("<I", data_size)
    header += b".FIT"
    header_crc = crc16(bytes(header))
    header += struct.pack("<H", header_crc)
    assert len(header) == 14

    payload = bytes(header) + body
    file_crc = crc16(payload)
    return payload + struct.pack("<H", file_crc)


def selftest():
    body = build_valid_fit_body()
    good = build_fit_bytes(body)

    facts = validate_bytes(good)
    assert facts["manufacturer"] == 23, f"expected manufacturer 23, got {facts['manufacturer']}"
    print("selftest: valid FIT bytes accepted, manufacturer decoded correctly (23)")

    # Flip one body byte (inside the data message's time_created field)
    # without fixing the trailing file CRC, which must now fail to match.
    header_size = 14
    corrupt = bytearray(good)
    flip_index = header_size + len(body) - 3
    corrupt[flip_index] ^= 0xFF
    try:
        validate_bytes(bytes(corrupt))
    except FitValidationError as e:
        if "CRC" not in str(e):
            print(f"selftest FAIL: rejected for the wrong reason: {e}", file=sys.stderr)
            return 1
        print(f"selftest: corrupted FIT bytes correctly rejected ({e})")
    else:
        print("selftest FAIL: corrupted FIT bytes were incorrectly accepted", file=sys.stderr)
        return 1

    print("selftest: PASS")
    return 0


def main(argv):
    if len(argv) == 2 and argv[1] == "--selftest":
        return selftest()

    if len(argv) != 2:
        print(f"usage: {argv[0]} <path-to-fit-file>", file=sys.stderr)
        print(f"       {argv[0]} --selftest", file=sys.stderr)
        return 2

    path = argv[1]
    try:
        with open(path, "rb") as f:
            data = f.read()
    except OSError as e:
        print(f"FAIL: cannot read {path}: {e}", file=sys.stderr)
        return 1

    try:
        facts = validate_bytes(data)
    except FitValidationError as e:
        print(f"FAIL: {e}", file=sys.stderr)
        return 1

    print(f"OK: {path}")
    print(f"  file_size: {facts['file_size']}")
    print(f"  header_size: {facts['header_size']}")
    print(f"  protocol_version: {facts['protocol_version']}")
    print(f"  profile_version: {facts['profile_version']}")
    print(f"  data_size: {facts['data_size']}")
    manufacturer = facts["manufacturer"]
    if manufacturer is None:
        print("  manufacturer: could not decode from message framing")
    elif manufacturer == 0xFFFF:
        print(f"  manufacturer: {manufacturer} (FIT invalid-value sentinel 0xFFFF)")
    else:
        print(f"  manufacturer: {manufacturer}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
