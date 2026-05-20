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

P = NS["p"]
A = NS["a"]
EMU = 914400


BOTTOM_NOTES = {
    3: "讲解线索：先说明为什么要做，再说明系统要给谁用、实现哪些功能。",
    4: "承接下一页：由业务痛点推导出四类使用角色及其功能边界。",
    5: "承接下一页：角色功能确定后，还需要补充安全、运行和可行性要求。",
    6: "小结：需求分析明确了系统建设范围，下一步转入系统如何设计。",
    7: "讲解线索：本章把需求转成架构、流程、接口和数据库。",
    8: "承接下一页：总体架构确定后，继续说明系统中最关键的业务处理流程。",
    9: "承接下一页：核心流程需要数据库表和接口设计提供数据支撑。",
    10: "小结：系统设计完成了结构、流程和数据层面的准备，下一章进入功能实现。",
    11: "讲解线索：实现部分按照用户端、医生端、仓库端和后台端展开。",
    12: "承接下一页：用户预约产生数据后，会继续流转到医生端和仓库端。",
    13: "承接下一页：各端功能落地后，需要通过接口说明关键操作如何实现。",
    14: "小结：实现部分形成了从登录、预约、开单到库存变化的业务闭环。",
    15: "讲解线索：测试部分验证功能是否可用、权限是否正确、数据是否联动。",
    16: "承接下一页：测试目标和方法确定后，按模块设计测试用例。",
    17: "承接下一页：正常流程通过后，还需要检查异常输入、权限和性能表现。",
    18: "承接下一页：测试结果用于说明系统当前可用性，并指出后续优化方向。",
    19: "答辩收束：总结已完成工作、系统价值和后续可改进方向。",
}

EXTRA_BOXES = {
    4: [
        (4.35, 1.08, 4.65, 0.50, "问题来源：预约、接诊、库存、后台管理"),
        (4.35, 1.72, 4.65, 0.50, "分析结果：拆分角色与业务流程"),
        (4.35, 2.36, 4.65, 0.50, "设计方向：建立多角色协同管理系统"),
    ],
    8: [
        (7.10, 2.05, 4.70, 0.42, "页面层：用户端 / 医生端 / 仓库端 / 后台端"),
        (7.10, 2.70, 4.70, 0.42, "接口层：认证、预约、订单、库存、日志"),
        (7.10, 3.35, 4.70, 0.42, "数据层：用户、宠物、预约、订单、库存"),
        (7.10, 4.00, 4.70, 0.42, "外部服务：短信验证码辅助身份校验"),
    ],
    9: [
        (1.05, 1.10, 11.15, 0.48, "核心逻辑主线：登录认证 -> 预约校验 -> 医生开单 -> 库存扣减 -> 日志追踪"),
    ],
    12: [
        (1.35, 2.42, 10.75, 0.46, "用户端业务流：注册登录 -> 维护宠物档案 -> 提交预约 -> 查看订单记录"),
        (1.35, 3.08, 10.75, 0.46, "数据流向：预约记录同步医生端，诊疗订单再关联宠物与药品明细"),
    ],
    13: [
        (1.05, 1.08, 11.15, 0.44, "实现主线：用户发起预约，医生完成接诊开单，仓库同步库存，后台负责权限和日志。"),
    ],
    16: [
        (1.20, 1.18, 10.85, 0.46, "测试思路：先验证核心业务能跑通，再检查异常输入和权限边界。"),
    ],
    19: [
        (1.25, 5.95, 10.85, 0.50, "最终结论：系统能够覆盖宠物医院从预约到接诊、从开单到库存、从权限到日志的主要流程。"),
    ],
}


def next_shape_id(root):
    ids = []
    for cNvPr in root.findall(".//p:cNvPr", NS):
        try:
            ids.append(int(cNvPr.get("id", "0")))
        except ValueError:
            pass
    return max(ids or [1]) + 1


def emu(v):
    return str(int(v * EMU))


def add_textbox(root, x, y, w, h, text, font_size=10, fill="F7F3F3", line="D6D6D6", color="444444", bold=False):
    sp_tree = root.find(".//p:spTree", NS)
    sid = next_shape_id(root)
    sp = ET.SubElement(sp_tree, f"{{{P}}}sp")

    nv = ET.SubElement(sp, f"{{{P}}}nvSpPr")
    ET.SubElement(nv, f"{{{P}}}cNvPr", {"id": str(sid), "name": f"continuity {sid}"})
    ET.SubElement(nv, f"{{{P}}}cNvSpPr", {"txBox": "1"})
    ET.SubElement(nv, f"{{{P}}}nvPr")

    sppr = ET.SubElement(sp, f"{{{P}}}spPr")
    xfrm = ET.SubElement(sppr, f"{{{A}}}xfrm")
    ET.SubElement(xfrm, f"{{{A}}}off", {"x": emu(x), "y": emu(y)})
    ET.SubElement(xfrm, f"{{{A}}}ext", {"cx": emu(w), "cy": emu(h)})
    geom = ET.SubElement(sppr, f"{{{A}}}prstGeom", {"prst": "roundRect"})
    ET.SubElement(geom, f"{{{A}}}avLst")
    solid = ET.SubElement(sppr, f"{{{A}}}solidFill")
    ET.SubElement(solid, f"{{{A}}}srgbClr", {"val": fill})
    ln = ET.SubElement(sppr, f"{{{A}}}ln", {"w": "6350"})
    lnfill = ET.SubElement(ln, f"{{{A}}}solidFill")
    ET.SubElement(lnfill, f"{{{A}}}srgbClr", {"val": line})

    tx = ET.SubElement(sp, f"{{{P}}}txBody")
    ET.SubElement(tx, f"{{{A}}}bodyPr", {"wrap": "square", "lIns": "91440", "rIns": "91440", "tIns": "45720", "bIns": "45720"})
    ET.SubElement(tx, f"{{{A}}}lstStyle")
    p = ET.SubElement(tx, f"{{{A}}}p")
    ppr = ET.SubElement(p, f"{{{A}}}pPr", {"algn": "c"})
    ET.SubElement(ppr, f"{{{A}}}buNone")
    r = ET.SubElement(p, f"{{{A}}}r")
    attrs = {"lang": "zh-CN", "sz": str(int(font_size * 100))}
    if bold:
        attrs["b"] = "1"
    rpr = ET.SubElement(r, f"{{{A}}}rPr", attrs)
    rfill = ET.SubElement(rpr, f"{{{A}}}solidFill")
    ET.SubElement(rfill, f"{{{A}}}srgbClr", {"val": color})
    t = ET.SubElement(r, f"{{{A}}}t")
    t.text = text


def add_bottom_note(root, text):
    add_textbox(root, 0.80, 6.92, 11.75, 0.34, text, font_size=9, fill="FBF7F7", line="E4DADA", color="666666")


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: make_defense_coherent_version.py <input.pptx> <output.pptx>")
    src = Path(sys.argv[1])
    dst = Path(sys.argv[2])
    shutil.copyfile(src, dst)
    with zipfile.ZipFile(src, "r") as zin:
        infos = zin.infolist()
        entries = {info.filename: zin.read(info.filename) for info in infos}

    for slide_no in range(3, 20):
        name = f"ppt/slides/slide{slide_no}.xml"
        root = ET.fromstring(entries[name])
        for x, y, w, h, text in EXTRA_BOXES.get(slide_no, []):
            add_textbox(root, x, y, w, h, text, font_size=10, fill="F3EEEE", line="D9C7C7", color="444444", bold=False)
        if slide_no in BOTTOM_NOTES:
            add_bottom_note(root, BOTTOM_NOTES[slide_no])
        entries[name] = ET.tostring(root, encoding="utf-8", xml_declaration=True)

    with zipfile.ZipFile(dst, "w") as zout:
        for info in infos:
            zout.writestr(info, entries[info.filename])
    print(dst)


if __name__ == "__main__":
    main()
