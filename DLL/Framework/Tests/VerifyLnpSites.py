"""Check every Learn & Play span in DLL/LnpSites.hpp against a Learn & Play Dec 2024 image.
Usage: python VerifyLnpSites.py path/to/image.exe
Requires pefile. Never modifies the image. Exits non-zero if any span differs.
"""
import re
import sys
from pathlib import Path
import pefile

root = Path(__file__).resolve().parents[3]
assert Path(sys.argv[1]).stat().st_size == 53509632, "Wrong LP image size"
pe = pefile.PE(sys.argv[1], fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
assert base == 0x1D0000 and pe.OPTIONAL_HEADER.SizeOfImage == 0x330C000, "Wrong LP image"
assert pe.FILE_HEADER.TimeDateStamp == 0x67497D00, "Wrong LP image"


def expand(text):
    out = bytearray()
    for token in text.split():
        if token.startswith("@"):
            out += ((base + int(token[1:], 16)) & 0xFFFFFFFF).to_bytes(4, "little")
        else:
            out.append(int(token, 16))
    return bytes(out)


source = (root / "DLL/LnpSites.hpp").read_text()
spans = re.findall(r"Span (\w+) = \{ 0x([0-9A-Fa-f]+), \"([^\"]+)\" \};", source)
assert len(spans) == source.count("Span k"), "a span did not parse"
assert pe.get_data(0x2A9C20, 3) == bytes.fromhex("55 8B EC"), "RVA positive control"
assert pe.get_data(0x2A9C21, 3) != bytes.fromhex("55 8B EC"), "RVA negative control"
failed = 0
for name, rva, text in spans:
    expected = expand(text)
    actual = pe.get_data(int(rva, 16), len(expected))
    ok = actual == expected
    failed += not ok
    print(f"{'MATCH' if ok else 'DIFF '} {name:28} RVA 0x{int(rva, 16):06X} {len(expected):2} bytes"
          + ("" if ok else f"\n      expected {expected.hex(' ')}\n      actual   {actual.hex(' ')}"))

# The VFS natives must be the table's entries, and each relative call must land where its mod expects.
natives = [int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]+)", re.search(r"kVfsNatives\[8\] = \{([^}]+)\}", source).group(1))]
table = re.search(r"kVfsTable = \{ 0x[0-9A-Fa-f]+, \"([^\"]+)\"", source).group(1)
assert natives == [int(t[1:], 16) for t in table.split()], "kVfsNatives differs from kVfsTable"


def call_target(rva):
    rel = int.from_bytes(pe.get_data(rva + 1, 4), "little", signed=True)
    return rva + 5 + rel


checks = [("SetLoftState", call_target(0x375A74 + 13), 0x375AE0),
          ("AddReference", call_target(0x9E995), 0x9E250),
          ("LoftPostFxSetState", call_target(0x375B5A), 0x377360),
          ("PitchReference", call_target(0xEA072), 0xDD930),
          ("uiDialogManagerHide", call_target(0x1A48F0 + 17), 0x1DE3E0)]
for name, actual, expected in checks:
    ok = actual == expected
    failed += not ok
    print(f"{'MATCH' if ok else 'DIFF '} call target {name:22} RVA 0x{actual:06X}")

manager = int(re.search(r"kPitchManagerGlobal = 0x([0-9A-Fa-f]+)", source).group(1), 16)
for offset in (0x2C, 0x3C):
    actual = int.from_bytes(pe.get_data(0xEA020 + offset, 4), "little")
    ok = actual == base + manager
    failed += not ok
    print(f"{'MATCH' if ok else 'DIFF '} pitch manager operand +0x{offset:X}")
print(f"{len(spans) + len(checks) + 2 - failed} of {len(spans) + len(checks) + 2} checks match")
sys.exit(1 if failed else 0)
