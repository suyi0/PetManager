#!/usr/bin/env python3
import re
import sys
import zlib
from pathlib import Path


def parse_objects(data: bytes):
    objects = {}
    for m in re.finditer(rb"(\d+)\s+0\s+obj(.*?)endobj", data, re.S):
        objects[int(m.group(1))] = m.group(2)
    return objects


def decode_stream(obj: bytes):
    if b"stream" not in obj:
        return None
    head, rest = obj.split(b"stream", 1)
    raw, _ = rest.split(b"endstream", 1)
    raw = raw.strip(b"\r\n")
    if b"/FlateDecode" in head:
        try:
            return zlib.decompress(raw)
        except Exception:
            return raw
    return raw


def parse_tounicode(stream: bytes):
    text = stream.decode("latin1", errors="ignore")
    mapping = {}
    for block in re.finditer(r"beginbfchar(.*?)endbfchar", text, re.S):
        for src, dst in re.findall(r"<([0-9A-Fa-f]+)>\s+<([0-9A-Fa-f]+)>", block.group(1)):
            try:
                mapping[int(src, 16)] = bytes.fromhex(dst).decode("utf-16-be", errors="ignore")
            except Exception:
                pass
    for block in re.finditer(r"beginbfrange(.*?)endbfrange", text, re.S):
        body = block.group(1)
        for start, end, dst in re.findall(r"<([0-9A-Fa-f]+)>\s+<([0-9A-Fa-f]+)>\s+<([0-9A-Fa-f]+)>", body):
            s, e, d = int(start, 16), int(end, 16), int(dst, 16)
            for code in range(s, e + 1):
                try:
                    mapping[code] = (d + code - s).to_bytes(2, "big").decode("utf-16-be", errors="ignore")
                except Exception:
                    pass
    return mapping


def page_content_refs(page_obj: bytes):
    m = re.search(rb"/Contents\s+(\d+)\s+0\s+R", page_obj)
    if m:
        return [int(m.group(1))]
    m = re.search(rb"/Contents\s*\[(.*?)\]", page_obj, re.S)
    if not m:
        return []
    return [int(x) for x in re.findall(rb"(\d+)\s+0\s+R", m.group(1))]


def page_font_maps(page_obj: bytes, objects):
    font_maps = {}
    # page resources commonly include /F1 38 0 R or /C0_0 1684 0 R
    for name, objid in re.findall(rb"/([A-Za-z0-9_]+)\s+(\d+)\s+0\s+R", page_obj):
        font = objects.get(int(objid), b"")
        if b"/Type/Font" not in font and b"/Subtype/Type0" not in font:
            continue
        tm = re.search(rb"/ToUnicode\s+(\d+)\s+0\s+R", font)
        if tm:
            stream = decode_stream(objects.get(int(tm.group(1)), b""))
            if stream:
                font_maps[name.decode("latin1")] = parse_tounicode(stream)
    return font_maps


def decode_pdf_string(token: bytes, cmap):
    if token.startswith(b"<") and token.endswith(b">") and not token.startswith(b"<<"):
        hexs = re.sub(rb"\s+", b"", token[1:-1])
        out = []
        for i in range(0, len(hexs), 4):
            part = hexs[i : i + 4]
            if len(part) < 4:
                continue
            code = int(part, 16)
            out.append(cmap.get(code, chr(code) if 32 <= code < 127 else ""))
        return "".join(out)
    if token.startswith(b"("):
        # Minimal literal-string decoding for report headings/latin text.
        s = token[1:-1]
        s = s.replace(rb"\(", b"(").replace(rb"\)", b")").replace(rb"\\", b"\\")
        return s.decode("latin1", errors="ignore")
    return ""


def extract_text_from_stream(stream: bytes, font_maps):
    content = stream.replace(b"\r", b"\n")
    font = None
    pieces = []
    tokens = re.finditer(
        rb"/([A-Za-z0-9_]+)\s+[\d.]+\s+Tf|(<[0-9A-Fa-f\s]+>|\((?:\\.|[^\\)])*\))\s*Tj|\[(.*?)\]\s*TJ|'",
        content,
        re.S,
    )
    for m in tokens:
        if m.group(1):
            font = m.group(1).decode("latin1")
            continue
        cmap = font_maps.get(font, {})
        if m.group(2):
            pieces.append(decode_pdf_string(m.group(2), cmap))
        elif m.group(3):
            arr = m.group(3)
            for st in re.findall(rb"<[0-9A-Fa-f\s]+>|\((?:\\.|[^\\)])*\)", arr, re.S):
                pieces.append(decode_pdf_string(st, cmap))
        else:
            pieces.append("\n")
    text = "".join(pieces)
    text = re.sub(r"[ \t]+", " ", text)
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text


def main():
    data = Path(sys.argv[1]).read_bytes()
    objects = parse_objects(data)
    page_ids = []
    for objid, obj in objects.items():
        if b"/Type/Page" in obj and b"/Type/Pages" not in obj:
            page_ids.append(objid)
    page_ids.sort()
    output = []
    for page_no, objid in enumerate(page_ids, 1):
        page = objects[objid]
        font_maps = page_font_maps(page, objects)
        texts = []
        for cref in page_content_refs(page):
            stream = decode_stream(objects.get(cref, b""))
            if stream:
                texts.append(extract_text_from_stream(stream, font_maps))
        output.append(f"\n===== PAGE {page_no} OBJ {objid} =====\n" + "\n".join(texts))
    print("\n".join(output))


if __name__ == "__main__":
    main()
