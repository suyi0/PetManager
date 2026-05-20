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
EMU = 914400


def get_text_shapes(root):
    return [sp for sp in root.findall(".//p:sp", NS) if sp.findall(".//a:t", NS)]


def set_box(sp, x, y, w, h):
    xfrm = sp.find("p:spPr/a:xfrm", NS)
    if xfrm is None:
        sppr = sp.find("p:spPr", NS)
        if sppr is None:
            sppr = ET.SubElement(sp, f"{{{NS['p']}}}spPr")
        xfrm = ET.SubElement(sppr, f"{{{A}}}xfrm")
    off = xfrm.find("a:off", NS)
    ext = xfrm.find("a:ext", NS)
    if off is None:
        off = ET.SubElement(xfrm, f"{{{A}}}off")
    if ext is None:
        ext = ET.SubElement(xfrm, f"{{{A}}}ext")
    off.set("x", str(int(x * EMU)))
    off.set("y", str(int(y * EMU)))
    ext.set("cx", str(int(w * EMU)))
    ext.set("cy", str(int(h * EMU)))


def set_font(sp, pt, bold=None):
    for rpr in sp.findall(".//a:rPr", NS):
        rpr.set("sz", str(int(pt * 100)))
        if bold is not None:
            if bold:
                rpr.set("b", "1")
            else:
                rpr.attrib.pop("b", None)


def set_align(sp, align="l"):
    for ppr in sp.findall(".//a:pPr", NS):
        ppr.set("algn", align)


def set_text(sp, text):
    tx_body = sp.find("p:txBody", NS)
    first_p = tx_body.find("a:p", NS)
    first_r = first_p.find("a:r", NS) if first_p is not None else None
    ppr = first_p.find("a:pPr", NS) if first_p is not None else None
    rpr = first_r.find("a:rPr", NS) if first_r is not None else None
    for p in list(tx_body.findall("a:p", NS)):
        tx_body.remove(p)
    for line in text.split("\n"):
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


def edit_slide(root, slide_no):
    shapes = get_text_shapes(root)
    if slide_no == 6:
        # The three small cards have limited width; reduce body length and font.
        titles = ["非功能需求", "可行性分析", "业务价值"]
        bodies = [
            "支持登录认证、角色权限、异常提示、数据一致性和基本并发访问。",
            "采用 Vue3、C++ Crow、MySQL、JWT 和短信验证码，成本较低、实现路径清楚。",
            "功能对应预约、接诊、开单、库存和后台管理流程，具备实际使用场景。",
        ]
        for i, title in enumerate(titles):
            title_idx = i * 2
            body_idx = title_idx + 1
            if title_idx < len(shapes):
                set_text(shapes[title_idx], title)
                set_font(shapes[title_idx], 16, True)
                set_align(shapes[title_idx], "c")
            if body_idx < len(shapes):
                set_text(shapes[body_idx], bodies[i])
                set_font(shapes[body_idx], 11, False)
                set_align(shapes[body_idx], "l")
        if len(shapes) > 6:
            set_text(shapes[6], "本章从实际业务出发，将宠物医院日常流程拆分为多角色协同操作，为后续设计提供依据。")
            set_font(shapes[6], 12, False)
            set_align(shapes[6], "l")

    if slide_no == 8 and len(shapes) >= 4:
        set_text(shapes[3], "系统总体架构")
        set_box(shapes[3], 0.86, 0.88, 4.8, 0.58)
        set_font(shapes[3], 30, True)
        set_align(shapes[3], "l")

        set_text(shapes[2], "System Design")
        set_box(shapes[2], 0.90, 1.48, 2.8, 0.30)
        set_font(shapes[2], 13, False)
        set_align(shapes[2], "l")

        set_text(shapes[0], "技术选型")
        set_box(shapes[0], 1.02, 2.45, 1.8, 0.34)
        set_font(shapes[0], 18, True)
        set_align(shapes[0], "l")

        set_text(
            shapes[1],
            "前端采用 Vue3 完成页面展示和交互，后端采用 C++ 与 Crow 提供 RESTful 接口。\n"
            "MySQL 用于保存用户、宠物、预约、订单、库存和日志等数据，Python 脚本辅助短信验证码发送。\n"
            "系统按用户端、医生端、仓库端和后台端划分页面，后端按认证、预约、订单、仓库和后台等模块组织接口。",
        )
        set_box(shapes[1], 1.02, 2.94, 11.2, 2.35)
        set_font(shapes[1], 12, False)
        set_align(shapes[1], "l")

    if slide_no == 9:
        # Four-column slide: titles were narrower than the body boxes.
        for header_i, body_i in [(0, 1), (2, 3), (4, 5), (6, 7)]:
            if body_i < len(shapes):
                bx = shapes[body_i].find("p:spPr/a:xfrm/a:off", NS)
                be = shapes[body_i].find("p:spPr/a:xfrm/a:ext", NS)
                if bx is not None and be is not None:
                    set_box(
                        shapes[header_i],
                        int(bx.get("x")) / EMU,
                        int(bx.get("y")) / EMU - 0.34,
                        int(be.get("cx")) / EMU,
                        0.30,
                    )
            set_font(shapes[header_i], 13, True)
            set_font(shapes[body_i], 11, False)
            set_align(shapes[header_i], "c")
            set_align(shapes[body_i], "l")

    if slide_no == 11 and len(shapes) >= 3:
        set_text(shapes[2], "系统实现")

    if slide_no == 12 and len(shapes) >= 4:
        set_text(shapes[0], "用户端实现")
        set_box(shapes[0], 0.95, 1.30, 4.0, 0.42)
        set_font(shapes[0], 22, True)
        set_align(shapes[0], "l")

        set_text(
            shapes[1],
            "用户端包含注册、登录、个人信息维护、宠物信息管理、在线预约和订单查询。\n"
            "注册与登录流程结合短信验证码、密码校验和 Token 保存，用于完成用户身份识别。",
        )
        set_box(shapes[1], 0.95, 1.86, 5.25, 1.50)
        set_font(shapes[1], 12, False)
        set_align(shapes[1], "l")

        set_text(shapes[2], "预约与订单查询")
        set_box(shapes[2], 7.05, 1.30, 4.4, 0.42)
        set_font(shapes[2], 22, True)
        set_align(shapes[2], "l")

        set_text(
            shapes[3],
            "用户提交预约后，后端保存记录并同步到医生端。\n"
            "订单查询时，根据用户名下宠物关联诊疗订单和药品明细，方便用户查看后续诊疗信息。",
        )
        set_box(shapes[3], 7.05, 1.86, 5.35, 1.50)
        set_font(shapes[3], 12, False)
        set_align(shapes[3], "l")

    if slide_no == 15 and shapes:
        set_text(shapes[0], "System Test")

    if slide_no == 19 and len(shapes) >= 4:
        set_text(shapes[0], "总结与展望")
        set_box(shapes[0], 0.95, 1.05, 3.8, 0.50)
        set_font(shapes[0], 28, True)
        set_align(shapes[0], "l")

        set_text(
            shapes[1],
            "完成内容：系统实现了用户预约、医生接诊、订单创建、库存维护、权限管理、考勤和日志等功能。",
        )
        set_box(shapes[1], 0.95, 1.85, 5.25, 1.05)
        set_font(shapes[1], 12, False)
        set_align(shapes[1], "l")

        set_text(
            shapes[3],
            "应用价值：通过多角色分工和数据联动，将预约、接诊、订单和库存流程连接起来。",
        )
        set_box(shapes[3], 0.95, 3.35, 5.25, 1.05)
        set_font(shapes[3], 12, False)
        set_align(shapes[3], "l")

        set_text(
            shapes[2],
            "后续展望：可继续完善医生排班、预约提醒、就诊记录追踪、性能优化和数据分析等功能。",
        )
        set_box(shapes[2], 7.15, 2.38, 5.10, 1.25)
        set_font(shapes[2], 12, False)
        set_align(shapes[2], "l")


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: fix_defense_ppt_layout.py <pptx>")
    target = Path(sys.argv[1])
    tmp = target.with_suffix(".layoutfix.tmp.pptx")
    shutil.copyfile(target, tmp)
    with zipfile.ZipFile(target, "r") as zin:
        infos = zin.infolist()
        entries = {info.filename: zin.read(info.filename) for info in infos}

    for slide_no in range(1, 21):
        name = f"ppt/slides/slide{slide_no}.xml"
        root = ET.fromstring(entries[name])
        edit_slide(root, slide_no)
        entries[name] = ET.tostring(root, encoding="utf-8", xml_declaration=True)

    with zipfile.ZipFile(tmp, "w") as zout:
        for info in infos:
            zout.writestr(info, entries[info.filename])
    shutil.move(tmp, target)
    print(target)


if __name__ == "__main__":
    main()
