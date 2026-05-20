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

SLIDES = {
    3: ["Requirements Analysis", "围绕角色、功能、约束和可行性展开需求分析。", "需求分析", "PART.01"],
    4: [
        "业务痛点",
        "预约分散、接诊信息不统一、库存变化难追踪。",
        "使用对象",
        "普通用户、医生、仓库管理员、超级管理员。",
        "建设目标",
        "打通预约、接诊、开单、库存和后台管理流程。",
        "从实际业务出发，先确定角色，再拆分功能。",
        "Requirements Analysis",
        "需求分析",
    ],
    5: [
        "用户端",
        "注册登录、宠物档案、在线预约、订单查询。",
        "医生端",
        "查看预约、接诊处理、创建诊疗订单。",
        "仓库端",
        "物品维护、库存调整、低库存和临期预警。",
        "后台端",
        "用户管理、权限分配、考勤与日志审计。",
        "四类角色共同支撑宠物医院的主要业务流程。",
    ],
    6: [
        "安全要求",
        "登录认证、权限控制、密码加密。",
        "运行要求",
        "异常提示、输入校验、数据一致性。",
        "可行性",
        "Vue3、C++ Crow、MySQL 技术成熟，成本可控。",
        "系统需求明确，适合中小型宠物医院基础管理场景。",
    ],
    7: ["System Design", "将需求转化为架构、核心流程、接口和数据库设计。", "系统设计", "PART.02"],
    8: [
        "架构设计",
        ["Vue3 前端 + C++ Crow 后端 + MySQL 数据库。", "四类角色通过 RESTful 接口完成业务交互。"],
        "System Architecture",
        "系统总体架构",
    ],
    9: [
        "身份认证",
        "Token 识别身份，角色限制访问范围。",
        "预约校验",
        "按医生、日期、时间段检查冲突。",
        "订单库存",
        "开单生成明细，同步扣减库存。",
        "日志追踪",
        "记录关键操作，便于问题排查。",
    ],
    10: [
        "数据库设计",
        "用户、宠物、预约、订单、库存、工时、日志等表。",
        "表关系设计",
        "通过编号字段连接用户、宠物、医生、药品和订单。",
        "接口设计",
        "内部接口处理业务交互，外部接口调用短信验证码。",
    ],
    11: ["System Realization", "按用户端、医生端、仓库端和后台端展示主要实现结果。", "系统实现", "PART.03"],
    12: [
        "用户端实现",
        ["注册登录：账号、密码、验证码校验。", "个人中心：维护用户和宠物信息。", "在线预约：选择医生、日期和时间段。"],
        ["预约同步医生端。", "订单关联宠物、诊疗记录和药品明细。", "用户可查看历史诊疗结果。"],
        "预约与订单查询",
    ],
    13: [
        "医生端",
        "查看预约、接诊、录入诊疗内容。",
        "仓库端",
        "维护物品、调整库存、查看预警。",
        "后台端",
        "管理用户、权限、考勤和日志。",
        "数据联动",
        "订单生成后同步影响库存。",
        "4类",
        "角色",
        "7组",
        "模块",
    ],
    14: [
        "认证接口",
        "校验账号密码，返回 Token 和角色。",
        "预约接口",
        "保存前检查时间冲突。",
        "库存接口",
        "支持增删改查和库存预警。",
        "权限接口",
        "授予或移除角色权限。",
    ],
    15: ["System Test", "通过功能测试和性能测试验证系统可用性。", "系统测试", "PART.04"],
    16: [
        "测试目标",
        "验证登录、预约、开单、库存、权限和日志。",
        "测试方法",
        "采用黑盒测试，按角色模拟真实操作流程。",
    ],
    17: [
        "用户模块测试",
        "注册登录、宠物档案、预约、订单查询。",
        "01",
        "医生模块测试",
        "预约查看、接诊处理、诊疗订单。",
        "02",
        "仓库模块测试",
        "库存维护、预警、订单联动。",
        "03",
        "后台模块测试",
        "用户管理、权限、考勤、日志。",
        "04",
    ],
    18: [
        "异常输入",
        "错误密码、重复预约、非法库存。",
        "权限控制",
        "不同角色访问对应模块。",
        "数据联动",
        "预约、订单、库存同步变化。",
        "性能测试",
        "观察响应时间、吞吐量和错误率。",
    ],
    19: [
        "总结与展望",
        "完成：预约、接诊、订单、库存、权限、考勤和日志。",
        "特点：多角色协同，业务数据可联动追踪。",
        "展望：完善排班提醒、数据统计和性能优化。",
    ],
}


def text_shapes(root):
    return [sp for sp in root.findall(".//p:sp", NS) if sp.findall(".//a:t", NS)]


def set_text(sp, text):
    lines = text if isinstance(text, list) else str(text).split("\n")
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


def apply_slide(root, slide_no, replacements):
    shapes = text_shapes(root)
    for sp, text in zip(shapes, replacements):
        set_text(sp, text)
    for sp in shapes:
        text = "".join(t.text or "" for t in sp.findall(".//a:t", NS))
        if len(text) > 22:
            set_font(sp, 11, False)
    if slide_no in (3, 7, 11, 15):
        if len(shapes) > 2:
            set_font(shapes[2], 60, True)
        if len(shapes) > 1:
            set_font(shapes[1], 12, False)
    if slide_no == 8 and len(shapes) >= 4:
        set_font(shapes[0], 16, True)
        set_font(shapes[1], 12, False)
        set_font(shapes[2], 14, False)
        set_font(shapes[3], 32, True)
    if slide_no == 12 and len(shapes) >= 4:
        set_font(shapes[0], 16, True)
        set_font(shapes[1], 11, False)
        set_font(shapes[2], 11, False)
        set_font(shapes[3], 16, True)


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: make_defense_concise_version.py <input.pptx> <output.pptx>")
    src = Path(sys.argv[1])
    dst = Path(sys.argv[2])
    shutil.copyfile(src, dst)
    with zipfile.ZipFile(src, "r") as zin:
        infos = zin.infolist()
        entries = {info.filename: zin.read(info.filename) for info in infos}
    for slide_no, replacements in SLIDES.items():
        name = f"ppt/slides/slide{slide_no}.xml"
        root = ET.fromstring(entries[name])
        apply_slide(root, slide_no, replacements)
        entries[name] = ET.tostring(root, encoding="utf-8", xml_declaration=True)
    with zipfile.ZipFile(dst, "w") as zout:
        for info in infos:
            zout.writestr(info, entries[info.filename])
    print(dst)


if __name__ == "__main__":
    main()
