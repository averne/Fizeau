#!/usr/bin/env python3

import sys, ctypes, hashlib
from pathlib import Path
from lz4.block import decompress
import capstone as cs


class NsoHeader(ctypes.Structure):
    TextCompress   = 1 << 0
    RodataCompress = 1 << 1
    DataCompress   = 1 << 2
    TextHash       = 1 << 3
    RodataHash     = 1 << 4
    DataHash       = 1 << 5

    class SegmentHeader(ctypes.Structure):
        _fields_ = [
            ("file_off",    ctypes.c_uint32),
            ("memory_off",  ctypes.c_uint32),
            ("size",        ctypes.c_uint32),
        ]

    class SegmentHeaderRelative(ctypes.Structure):
        _fields_ = [
            ("offset",      ctypes.c_uint32),
            ("size",        ctypes.c_uint32),
        ]

    _fields_ = [
        ("magic",           ctypes.c_uint32),
        ("version",         ctypes.c_uint32),
        ("reserved1",       ctypes.c_uint32),
        ("flags",           ctypes.c_uint32),
        ("text_hdr",        SegmentHeader),
        ("module_name_off", ctypes.c_uint32),
        ("rodata_hdr",      SegmentHeader),
        ("module_name_sz",  ctypes.c_uint32),
        ("data_hdr",        SegmentHeader),
        ("bss_sz",          ctypes.c_uint32),
        ("build_id",        ctypes.c_uint8 * 0x20),
        ("text_comp_sz",    ctypes.c_uint32),
        ("rodata_comp_sz",  ctypes.c_uint32),
        ("data_comp_sz",    ctypes.c_uint32),
        ("reserved2",       ctypes.c_uint8 * 0x1c),
        ("api_info_hdr",    SegmentHeaderRelative),
        ("dynstr_info_hdr", SegmentHeaderRelative),
        ("dynsym_info_hdr", SegmentHeaderRelative),
        ("text_hash",       ctypes.c_uint8 * 0x20),
        ("rodata_hash",     ctypes.c_uint8 * 0x20),
        ("data_hash",       ctypes.c_uint8 * 0x20),
    ]

assert(ctypes.sizeof(NsoHeader) == 0x100)


def find_ioctl(text):
    d        = cs.Cs(cs.CS_ARCH_ARM64, cs.CS_MODE_ARM | cs.CS_MODE_LITTLE_ENDIAN)
    d.detail = True

    # Io code: 0xC99A020E
    # Expected instruction sequences:
    #   mov       w1,#0x20e
    #   movk      w1,#0xc99a, LSL #16
    #   ...
    #   bl        NvOsDrvIoctl
    # -------------------------------
    #   mov       w1,#0xc99a0000
    #   movk      w1,#0x20e
    #   ...
    #   bl        NvOsDrvIoctl

    state = 0
    for i in range(0, len(text), 4):
        try:
            inst = next(d.disasm(text[i:i+4], 0x7100000000))
        except StopIteration:
            continue

        if len(inst.operands) == 2:
            dst, src = inst.operands[0], inst.operands[1]

        match state:
            case 0:
                if inst.mnemonic == "mov" and \
                        dst.type == cs.arm64.ARM64_OP_REG and dst.value.reg == cs.arm64.ARM64_REG_W1 and \
                        src.type == cs.arm64.ARM64_OP_IMM and src.value.imm in (0x20e, -0x36660000):
                    state = 1
            case 1:
                if inst.mnemonic == "movk" and \
                        dst.type == cs.arm64.ARM64_OP_REG and dst.value.reg == cs.arm64.ARM64_REG_W1:
                    state = 2 if src.type == cs.arm64.ARM64_OP_IMM and src.value.imm in (0xc99a, 0x20e) else 0
            case 2:
                if inst.mnemonic == "bl":
                    return i


def main(argc, argv):
    if argc != 2:
        print(f"Usage: {argv[0]} nso")
        return

    print(f"Patching {argv[1]}")

    p = Path(argv[1])
    fp = p.open("rb")

    hdr = NsoHeader()
    fp.readinto(hdr)

    build_id = bytes(hdr.build_id).rstrip(b"\0").hex()
    print(f"Build id: {build_id}")

    if hdr.module_name_sz > 1:
        fp.seek(hdr.module_name_off)
        print(f"Module name: {fp.read(hdr.module_name_sz).decode()}")

    fp.seek(hdr.text_hdr.file_off)
    text = fp.read(hdr.text_comp_sz)

    if hdr.flags & NsoHeader.TextCompress:
        text = decompress(text, uncompressed_size=hdr.text_hdr.size)
        assert(len(text) == hdr.text_hdr.size)

    if hdr.flags & NsoHeader.TextHash:
        h = hashlib.sha256(text).digest()
        assert(h == bytes(hdr.text_hash))

    off = find_ioctl(text)
    assert(off is not None)

    seq = b"\xe0\x03\x1f\x2a" # mov w0, wzr

    patch = b"PATCH"
    patch += (off + ctypes.sizeof(NsoHeader)).to_bytes(3, byteorder="big")
    patch += len(seq).to_bytes(2, byteorder="big")
    patch += seq
    patch += b"EOF"

    dest = p.parent / (build_id + ".ips")
    with open(dest, "wb") as out:
        out.write(patch)


if __name__ == "__main__":
    main(len(sys.argv), sys.argv)
