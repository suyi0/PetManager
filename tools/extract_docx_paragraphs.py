#!/usr/bin/env python3
import json
import sys
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET

NS = {
    "w": "http://schemas.openxmlformats.org/wordprocessingml/2006/main",
}


def paragraph_text(p):
    parts = []
    for node in p.iter():
        if node.tag == f"{{{NS['w']}}}t":
            parts.append(node.text or "")
        elif node.tag == f"{{{NS['w']}}}tab":
            parts.append("\t")
        elif node.tag == f"{{{NS['w']}}}br":
            parts.append("\n")
    return "".join(parts)


def paragraph_style(p):
    ppr = p.find("w:pPr", NS)
    if ppr is None:
        return ""
    pstyle = ppr.find("w:pStyle", NS)
    if pstyle is None:
        return ""
    return pstyle.attrib.get(f"{{{NS['w']}}}val", "")


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: extract_docx_paragraphs.py <docx>")
    docx_path = Path(sys.argv[1])
    with zipfile.ZipFile(docx_path) as zf:
        xml = zf.read("word/document.xml")
    root = ET.fromstring(xml)
    body = root.find("w:body", NS)
    paragraphs = []
    for i, p in enumerate(body.findall(".//w:p", NS)):
        text = paragraph_text(p).strip()
        if text:
            paragraphs.append(
                {
                    "index": i,
                    "style": paragraph_style(p),
                    "chars": len(text),
                    "text": text,
                }
            )
    print(json.dumps(paragraphs, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
