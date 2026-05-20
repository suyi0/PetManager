#!/usr/bin/env python3
import json
import sys
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET

NS = {"w": "http://schemas.openxmlformats.org/wordprocessingml/2006/main"}
W = NS["w"]


def node_text(node):
    parts = []
    for child in node.iter():
        if child.tag == f"{{{W}}}t":
            parts.append(child.text or "")
        elif child.tag == f"{{{W}}}tab":
            parts.append("\t")
        elif child.tag == f"{{{W}}}br":
            parts.append("\n")
    return "".join(parts)


def run_mark(run):
    rpr = run.find("w:rPr", NS)
    if rpr is None:
        return {}
    mark = {}
    color = rpr.find("w:color", NS)
    if color is not None:
        val = color.attrib.get(f"{{{W}}}val")
        if val:
            mark["color"] = val
    highlight = rpr.find("w:highlight", NS)
    if highlight is not None:
        val = highlight.attrib.get(f"{{{W}}}val")
        if val:
            mark["highlight"] = val
    shd = rpr.find("w:shd", NS)
    if shd is not None:
        val = shd.attrib.get(f"{{{W}}}fill")
        if val:
            mark["shading"] = val
    return mark


def paragraph_style(p):
    ppr = p.find("w:pPr", NS)
    if ppr is None:
        return ""
    pstyle = ppr.find("w:pStyle", NS)
    if pstyle is None:
        return ""
    return pstyle.attrib.get(f"{{{W}}}val", "")


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: extract_docx_colored_runs.py <docx>")
    with zipfile.ZipFile(Path(sys.argv[1])) as zf:
        root = ET.fromstring(zf.read("word/document.xml"))
    rows = []
    paragraphs = root.findall(".//w:p", NS)
    for pi, p in enumerate(paragraphs):
        full = node_text(p).strip()
        if not full:
            continue
        marked_parts = []
        all_runs = []
        for r in p.findall(".//w:r", NS):
            txt = node_text(r)
            if not txt:
                continue
            mark = run_mark(r)
            all_runs.append({"text": txt, "mark": mark})
            if mark:
                marked_parts.append({"text": txt, "mark": mark})
        if marked_parts:
            rows.append(
                {
                    "index": pi,
                    "style": paragraph_style(p),
                    "text": full,
                    "marked_text": "".join(part["text"] for part in marked_parts).strip(),
                    "marks": marked_parts,
                    "runs": all_runs,
                }
            )
    print(json.dumps(rows, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
