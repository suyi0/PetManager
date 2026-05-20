#!/usr/bin/env python3
import importlib.util
import shutil
import sys
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET

NS = {"w": "http://schemas.openxmlformats.org/wordprocessingml/2006/main"}
W = NS["w"]
ET.register_namespace("w", W)


def load_indices():
    script = Path(__file__).with_name("rewrite_docx_report_pass.py")
    spec = importlib.util.spec_from_file_location("rewrite_docx_report_pass", script)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return set(module.REPLACEMENTS)


def ensure_child(parent, tag):
    child = parent.find(f"w:{tag}", NS)
    if child is None:
        child = ET.Element(f"{{{W}}}{tag}")
        parent.insert(0, child)
    return child


def highlight_paragraph(p):
    for run in p.findall(".//w:r", NS):
        rpr = ensure_child(run, "rPr")
        highlight = rpr.find("w:highlight", NS)
        if highlight is None:
            highlight = ET.SubElement(rpr, f"{{{W}}}highlight")
        highlight.set(f"{{{W}}}val", "yellow")


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: highlight_report_pass.py <input.docx> <output.docx>")
    src = Path(sys.argv[1])
    dst = Path(sys.argv[2])
    shutil.copyfile(src, dst)
    indices = load_indices()
    with zipfile.ZipFile(src, "r") as zin:
        entries = {info.filename: zin.read(info.filename) for info in zin.infolist()}
        infos = zin.infolist()
    root = ET.fromstring(entries["word/document.xml"])
    paragraphs = root.findall(".//w:p", NS)
    for idx, p in enumerate(paragraphs):
        if idx in indices:
            highlight_paragraph(p)
    entries["word/document.xml"] = ET.tostring(root, encoding="utf-8", xml_declaration=True)
    with zipfile.ZipFile(dst, "w") as zout:
        for info in infos:
            zout.writestr(info, entries[info.filename])
    print(f"highlighted={len(indices)}")


if __name__ == "__main__":
    main()
