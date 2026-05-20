#!/usr/bin/env python3
import shutil
import sys
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET

NS = {
    "p": "http://schemas.openxmlformats.org/presentationml/2006/main",
    "a": "http://schemas.openxmlformats.org/drawingml/2006/main",
    "r": "http://schemas.openxmlformats.org/officeDocument/2006/relationships",
}
for prefix, uri in NS.items():
    ET.register_namespace(prefix, uri)

A = NS["a"]


def text_shapes(root):
    return [sp for sp in root.findall(".//p:sp", NS) if sp.findall(".//a:t", NS)]


def set_text(sp, lines):
    if isinstance(lines, str):
        lines = lines.split("\n")
    tx_body = sp.find("p:txBody", NS)
    first_p = tx_body.find("a:p", NS)
    first_r = first_p.find("a:r", NS) if first_p is not None else None
    ppr = first_p.find("a:pPr", NS) if first_p is not None else None
    rpr = first_r.find("a:rPr", NS) if first_r is not None else None
    for p in list(tx_body.findall("a:p", NS)):
        tx_body.remove(p)
    for line in lines:
        p = ET.SubElement(tx_body, f"{{{A}}}p")
        if ppr is not None:
            p.append(ET.fromstring(ET.tostring(ppr)))
        r = ET.SubElement(p, f"{{{A}}}r")
        if rpr is not None:
            r.append(ET.fromstring(ET.tostring(rpr)))
        else:
            ET.SubElement(r, f"{{{A}}}rPr", {"lang": "zh-CN", "sz": "1200"})
        t = ET.SubElement(r, f"{{{A}}}t")
        t.text = line


def set_font(sp, pt=None, bold=None):
    for rpr in sp.findall(".//a:rPr", NS):
        if pt is not None:
            rpr.set("sz", str(int(pt * 100)))
        if bold is True:
            rpr.set("b", "1")
        elif bold is False:
            rpr.attrib.pop("b", None)


def set_align(sp, align):
    for ppr in sp.findall(".//a:pPr", NS):
        ppr.set("algn", align)


def edit_slide_8(root):
    shapes = text_shapes(root)
    # Current order in this template: body title, body text, English subtitle, main title.
    set_text(shapes[3], "系统总体架构")
    set_font(shapes[3], 32, True)
    set_align(shapes[3], "l")

    set_text(shapes[2], "System Architecture")
    set_font(shapes[2], 14, False)
    set_align(shapes[2], "l")

    set_text(shapes[0], "架构说明")
    set_font(shapes[0], 16, True)
    set_align(shapes[0], "l")

    set_text(
        shapes[1],
        [
            "前端采用 Vue3 负责页面展示和用户交互；后端采用 C++ 与 Crow 提供 RESTful 接口。",
            "MySQL 保存用户、宠物、预约、订单、库存和日志等业务数据。",
            "系统按用户端、医生端、仓库端和后台端划分功能，各端通过统一接口完成数据交互。",
        ],
    )
    set_font(shapes[1], 11, False)
    set_align(shapes[1], "l")


def edit_slide_12(root):
    shapes = text_shapes(root)
    # Left block.
    set_text(shapes[0], "用户端实现")
    set_font(shapes[0], 16, True)
    set_align(shapes[0], "l")
    set_text(
        shapes[1],
        [
            "用户端包含注册登录、个人信息维护、宠物信息管理、在线预约和订单查询。",
            "登录成功后保存 Token，后续请求根据身份信息访问对应功能。",
        ],
    )
    set_font(shapes[1], 11, False)
    set_align(shapes[1], "l")

    # Right block: the template's upper small box is the title, lower big box is the body.
    set_text(shapes[3], "预约与订单查询")
    set_font(shapes[3], 16, True)
    set_align(shapes[3], "l")
    set_text(
        shapes[2],
        [
            "用户提交预约后，后端保存预约记录，并同步到医生端预约列表。",
            "订单查询时，系统根据宠物信息关联诊疗订单和药品明细，方便用户查看诊疗结果。",
        ],
    )
    set_font(shapes[2], 11, False)
    set_align(shapes[2], "l")


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: fix_slides_8_12.py <pptx>")
    target = Path(sys.argv[1])
    tmp = target.with_suffix(".slides812.tmp.pptx")
    with zipfile.ZipFile(target, "r") as zin:
        infos = zin.infolist()
        entries = {info.filename: zin.read(info.filename) for info in infos}

    for slide_no, editor in [(8, edit_slide_8), (12, edit_slide_12)]:
        name = f"ppt/slides/slide{slide_no}.xml"
        root = ET.fromstring(entries[name])
        editor(root)
        entries[name] = ET.tostring(root, encoding="utf-8", xml_declaration=True)

    with zipfile.ZipFile(tmp, "w") as zout:
        for info in infos:
            zout.writestr(info, entries[info.filename])
    shutil.move(tmp, target)
    print(target)


if __name__ == "__main__":
    main()
