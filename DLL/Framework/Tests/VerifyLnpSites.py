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

# An LP literal that bakes the dump's preferred base into an absolute operand still matches the dump,
# because the dump loads at that base, so the byte compares above cannot see that class of defect at
# all. This scan can: any 4-byte window of an LP pattern that reads as an address inside the LP image
# is almost certainly an absolute operand that should have been written as an @RVA token. Windows a
# token produced are correct by construction and exempt, as are the zero holes an Enumeration Span()
# fills from live Offsets.
#
# Opcode, immediate, displacement, branch and string bytes do form such values by coincidence. Each
# one is reviewed and listed here rather than loosening the test. A key is the pattern's scope plus
# the window's offset and value, so editing the bytes retires the entry and the window is re-reviewed.
reviewed = {
    ("kUplayDialogIdRead", 0, 0x02C0B98D): "lea edi,[ecx+2C0]: opcode, modrm and the low disp32 bytes",
    ("kUplayTransitionBegin", 10, 0x02C0C181): "add ecx,2C0: opcode, modrm and the low imm32 bytes",
    ("kUplayConnectSkip", 1, 0x00908B01): "mov eax,[ecx] running into the next modrm",
    ("kUplayConnectSkip", 2, 0x0200908B): "mov edx,[eax+200]: opcode, modrm and disp32",
    ("kUplayWin32Skip", 0, 0x00CC81C7): "mov dword [ecx+CC]: opcode, modrm and disp32",
    ("kMenuMovieSetup", 12, 0x0121BE80): "cmp byte [esi+121],0: opcode, modrm and the low disp32 bytes",
    ("kAdvancedDisplayCrash", 7, 0x012B1088): "mov [eax],dl running into sub eax,[ecx]",
    ("kPortAudioInCrash", 0, 0x010C7B83): "cmp dword [ebx+0C],1: opcode, modrm, disp8 and imm8",
    ("kPortAudioInCrash", 4, 0x02758C0F): "jl rel32: opcode and the low rel32 bytes",
    ("kInvalidInputTreeCheck", 2, 0x0098840F): "jz rel32: opcode and the low rel32 bytes",
    ("kInvalidInputTreeEmpty", 2, 0x0278C78B): "mov eax,edi running into js +2",
    ("kCalibrationSampleCount", 16, 0x018B0774): "jz +7 running into mov eax,[ecx]",
    ("kSampleRateRequirement", 6, 0x00BB80B8): "mov eax,48000: opcode and imm32",
    ("kSampleRateRequirement", 13, 0x005DC0B8): "mov eax,24000: opcode and imm32",
    ("kSongPreviewEvent", 12, 0x00776569): "ASCII 'iew' and the terminator of a Wwise event name",
    ("kSongPreviewEvent", 25, 0x00776569): "ASCII 'iew' and the terminator of a Wwise event name",
    ("kRdvMetadataEntry", 1, 0x03047E83): "cmp dword [esi+04],3: modrm, disp8 and imm8",
    ("kRdvMetadataNoop", 1, 0x010A3744): "mov byte [edi+esi+0A],1: modrm, sib, disp8 and imm8",
    ("kDlcInstalledCheck", 14, 0x015F2444): "mov byte [esp+5F],1: modrm, sib, disp8 and imm8",
    ("kFrameBudgetLoad", 9, 0x00B098DD): "fstp qword [eax+B0]: opcode, modrm and the low disp32 bytes",
    ("kBootLoftLight", 4, 0x006A0C75): "jnz +0C running into push 0",
    ("kBootLoftLight", 6, 0x006A006A): "two push 0 instructions",
    ("kBootLoftLight", 12, 0x005AE8C0): "xor eax,eax running into call rel32",
    ("kGfxTell", 1, 0x018B0849): "mov ecx,[ecx+08] running into mov eax,[ecx]",
    ("kGfxLength", 1, 0x018B0849): "mov ecx,[ecx+08] running into mov eax,[ecx]",
    ("kPitchReference", 50, 0x007DC9E8): "call rel32: opcode and the low rel32 bytes",
    ("kPitchRoute", 12, 0x00498D03): "jmp +3 running into the lea ecx,[ecx+0] pad",
    ("kPitchRoute", 52, 0x02F9830E): "jmp +0E running into cmp ecx,2",
    ("AudioDevices<5>", 0, 0x00BB80B8): "mov eax,48000: opcode and imm32",
    ("BugPrevention<6>", 0, 0x0218830F): "jnb rel32: opcode and the low rel32 bytes",
    ("BugPrevention<10>", 0, 0x010C7B83): "cmp dword [ebx+0C],1: opcode, modrm, disp8 and imm8",
    ("BugPrevention<10>", 4, 0x02758C0F): "jl rel32: opcode and the low rel32 bytes",
    ("BugPrevention<hookLength>", 2, 0x0098840F): "jz rel32: opcode and the low rel32 bytes",
    ("TwoRTCBypassMod<25>", 14, 0x014CB38D): "lea esi,[ebx+14C]: opcode, modrm and the low disp32 bytes",
    ("EnumerationCandidateIndex.locateNormalise", 5, 0x01E1801C): "mov cl,[eax+1C] running into and cl,1",
    ("EnumerationMapperHashChecks.hash", 55, 0x0193DB69): "imul ebx,ebx,1000193: opcode, modrm and the low imm32 bytes",
    ("EnumerationMapperHashChecks.hash", 57, 0x01000193): "the FNV-1a prime 1000193 as an imul immediate",
    ("EnumerationMapperHashChecks.length", 6, 0x02741451): "cmp [ecx+14],edx running into jz +2",
    ("EnumerationMapperHashChecks.character", 102, 0x00388009): "jz +9 running into cmp byte [eax],0",
    ("EnumerationMapperHashChecks.character", 106, 0x0088840F): "jz rel32: opcode and the low rel32 bytes",
    ("EnumerationMapperHashChecks.character", 114, 0x0080870F): "ja rel32: opcode and the low rel32 bytes",
    ("EnumerationMapperHashChecks.character", 123, 0x00FFE781): "and edi,0FF: opcode, modrm and imm32",
    ("EnumerationMapperHashChecks.character", 152, 0x0274144A): "cmp [edx+14],ecx running into jz +2",
    ("EnumerationMapperHashChecks.character", 155, 0x03128B02): "jz +2 running into mov edx,[edx] and add edx,eax",
    ("EnumerationMapperHashChecks.character", 166, 0x02BE0F04): "mov [ebx+04],al running into movsx eax,byte [edx]",
    ("EnumerationMapperHashChecks.character", 169, 0x00FF2502): "and eax,0FF: opcode and imm32",
    ("EnumerationMapperHashChecks.character", 224, 0x006A1F04): "lea eax,[edi+ebx] running into push 0",
}


def in_image_windows(data, expanded):
    """Offset and value of every 4-byte window that reads as an address inside the LP image.
    A None byte is a hole an Enumeration Span() fills at runtime; expanded holds @ token offsets."""
    found = []
    for i in range(len(data) - 3):
        window = data[i:i + 4]
        if any(b is None for b in window) or any(j in expanded for j in range(i, i + 4)):
            continue
        value = window[0] | window[1] << 8 | window[2] << 16 | window[3] << 24
        if base <= value < base + pe.OPTIONAL_HEADER.SizeOfImage:
            found.append((i, value))
    return found


def expand_marked(text):
    """Like expand, but returns a byte list and the offsets an @ token produced."""
    data, expanded = [], set()
    for token in text.split():
        if token.startswith("@"):
            value = (base + int(token[1:], 16)) & 0xFFFFFFFF
            for shift in range(4):
                expanded.add(len(data))
                data.append((value >> (8 * shift)) & 0xFF)
        else:
            data.append(int(token, 16))
    return data, expanded


# The scan has to see the bytes a C++ pattern holds, so these read the sources rather than the image.
def strip_comments(text):
    out, i, n = [], 0, len(text)
    while i < n:
        if text[i] == '"':
            out.append(text[i])
            i += 1
            while i < n and text[i] != '"':
                if text[i] == "\\":
                    out.append(text[i])
                    i += 1
                out.append(text[i])
                i += 1
            if i < n:
                out.append(text[i])
                i += 1
            continue
        if text[i] == "/" and i + 1 < n and text[i + 1] in "/*":
            if text[i + 1] == "/":
                while i < n and text[i] != "\n":
                    i += 1
            else:
                i += 2
                while i + 1 < n and not (text[i] == "*" and text[i + 1] == "/"):
                    i += 1
                i += 2
            continue
        out.append(text[i])
        i += 1
    return "".join(out)


def bracketed(text, open_at):
    depth = 0
    for i in range(open_at, len(text)):
        if text[i] in "{(":
            depth += 1
        elif text[i] in "})":
            depth -= 1
            if depth == 0:
                return text[open_at + 1:i]
    return ""


def top_level_split(body):
    parts, depth, current = [], 0, ""
    for c in body:
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        if c == "," and depth == 0:
            parts.append(current)
            current = ""
        else:
            current += c
    parts.append(current)
    return parts


def slot_bytes(slot, text):
    """Bytes of a VersionedPattern's Learn & Play slot. None when it holds no literal bytes:
    an unavailable or masked pattern, or an array built elsewhere."""
    slot = slot.strip()
    literal = re.search(r'ExpandLnpPattern<\d+>\s*\(\s*"([^"]+)"', slot)
    if literal:
        return expand_marked(literal.group(1))
    if "UnavailablePattern" in slot or "MaskedPattern" in slot:
        return None, None
    if "MakeBytes" in slot:
        return [int(b, 16) for b in re.findall(r"0x([0-9A-Fa-f]{1,2})\b", slot)], set()
    name = re.fullmatch(r"(?:ExactPattern\()?([A-Za-z_]\w*)\)?", slot)
    if name:
        indirect = re.search(r"\b" + name.group(1) + r'\s*=\s*ExpandLnpPattern<\d+>\s*\(\s*"([^"]+)"', text)
        if indirect:
            return expand_marked(indirect.group(1))
    return None, None


def lnp_byte_sources():
    """Every place a Learn & Play byte string is written: the LnpSites spans, the LP slot of each
    VersionedPattern, and the byte array of each Enumeration Span()."""
    sources = [(name, *expand_marked(text)) for name, _, text in spans]
    unparsed = []
    for path in sorted(set(root.glob("DLL/**/*.cpp")) | set(root.glob("DLL/**/*.hpp"))
                       | set(root.glob("DLL/**/*.inc"))):
        relative = path.relative_to(root).as_posix()
        if relative.startswith("DLL/Lib/") or relative.startswith("DLL/Framework/Tests/"):
            continue
        text = strip_comments(path.read_text(errors="replace"))

        for match in re.finditer(r"VersionedPattern<([^>]*)>\s*\{", text):
            slots = top_level_split(bracketed(text, match.end() - 1))
            if len(slots) != 2:
                unparsed.append(f"{relative} VersionedPattern<{match.group(1)}>")
                continue
            data, expanded = slot_bytes(slots[1], text)
            if data:
                sources.append((f"{path.stem}<{match.group(1)}>", data, expanded))

        for match in re.finditer(r"=\s*Span\(", text):
            body = bracketed(text, text.index("{", match.end()))
            if "0x" not in body:
                continue
            name = re.search(r"(\w+)\s*$", text[:match.start()])
            data = [None if b == "0" else int(b, 16)
                    for b in re.findall(r"0x[0-9A-Fa-f]{1,2}|\b0\b", body)]
            sources.append((f"{path.stem}.{name.group(1) if name else '?'}", data, set()))
    return sources, unparsed


# Controls: the literal this scan exists to catch must fire, and its @RVA fix must not.
assert in_image_windows([0x8B, 0x0D, 0xFC, 0x0B, 0x13, 0x01], set()), "scan positive control"
assert not in_image_windows(*expand_marked("8B 0D @F60BFC")), "scan negative control"

byte_sources, unparsed_patterns = lnp_byte_sources()
suspects = len(unparsed_patterns)
for pattern in unparsed_patterns:
    print(f"UNPARSED Learn & Play slot, the scan cannot see it: {pattern}")

for scope, data, expanded in byte_sources:
    for offset, value in in_image_windows(data, expanded):
        if (scope, offset, value) in reviewed:
            continue
        suspects += 1
        print(f"SUSPECT {scope} offset {offset} reads 0x{value:08X}, LP RVA 0x{value - base:X}."
              " Write it as an @RVA token, or add it to the reviewed list with a reason.")
print(f"absolute-operand scan: {len(byte_sources)} Learn & Play byte strings,"
      f" {len(reviewed)} reviewed windows, {suspects} unreviewed")
print(f"{len(spans) + len(checks) + 2 - failed} of {len(spans) + len(checks) + 2} checks match")
sys.exit(1 if failed or suspects else 0)
