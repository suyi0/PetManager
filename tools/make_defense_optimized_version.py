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
    3: [
        "Requirements Analysis",
        "本章主要回答三个问题：系统面向哪些角色、需要解决哪些业务问题、项目是否具备实现条件。",
        "需求分析",
        "PART.01",
    ],
    4: [
        "业务痛点",
        "预约登记依赖人工，接诊信息分散，库存变化不容易及时追踪。",
        "使用对象",
        "系统围绕普通用户、医生、仓库管理员和超级管理员四类角色展开。",
        "建设目标",
        "把预约、接诊、开单、库存和后台管理串成一条完整业务流程。",
        "需求分析部分先从宠物医院日常工作出发，再把业务拆分为多角色、多模块的系统需求。",
        "Requirements Analysis",
        "需求分析",
    ],
    5: [
        "用户端",
        "注册登录、维护宠物档案、提交预约、查看订单和诊疗记录。",
        "医生端",
        "查看预约、查询用户与宠物信息、录入诊疗内容并创建订单。",
        "仓库端",
        "维护药品物品信息，处理库存数量、价格、低库存和临期提醒。",
        "后台端",
        "进行用户管理、权限分配、考勤查看和操作日志审计。",
        "四类角色不是孤立页面，预约、订单、库存和日志会在业务过程中相互关联。",
    ],
    6: [
        "安全要求",
        "登录认证、角色权限、密码加密和越权访问控制。",
        "运行要求",
        "异常提示、输入校验、数据一致性和基本并发访问能力。",
        "可行性",
        "Vue3、C++ Crow、MySQL、JWT 等技术成熟，开发成本和维护成本可控。",
        "从需求和可行性看，本系统适合中小型宠物医院的基础数字化管理场景。",
    ],
    7: [
        "System Design",
        "本章将需求转化为系统结构，重点说明总体架构、核心流程、接口设计和数据库设计。",
        "系统设计",
        "PART.02",
    ],
    8: [
        "架构设计",
        [
            "前端 Vue3，后端 C++ Crow，数据库采用 MySQL。",
            "四类角色通过 RESTful 接口完成预约、订单、库存和后台管理。",
        ],
        "System Architecture",
        "系统总体架构",
    ],
    9: [
        "身份认证",
        "登录后生成 Token，接口层解析身份和角色，控制不同端口访问范围。",
        "预约校验",
        "提交预约前检查医生、日期和时间段，避免同一时间重复预约。",
        "订单库存",
        "医生开单后生成药品明细，同时扣减仓库库存数量。",
        "日志追踪",
        "记录登录、修改、授权和异常操作，便于后期排查问题。",
    ],
    10: [
        "数据库设计",
        "围绕用户、宠物、预约、订单、药品明细、仓库、工时和日志等表展开。",
        "表关系设计",
        "通过用户编号、宠物编号、医生编号和药品编号连接不同业务数据。",
        "接口设计",
        "内部接口负责前后端通信，外部接口主要用于短信验证码服务。",
    ],
    11: [
        "System Realization",
        "本章展示系统主要模块的实现结果，重点说明用户端、医生端、仓库端和后台端的业务闭环。",
        "系统实现",
        "PART.03",
    ],
    12: [
        "用户端实现",
        [
            "注册登录：手机号、邮箱、密码和验证码校验。",
            "个人中心：维护个人信息和宠物档案。",
            "预约功能：选择医生、日期和时间段后提交预约。",
        ],
        [
            "预约记录同步到医生端，方便医生安排接诊。",
            "订单查询根据宠物关联诊疗订单和药品明细。",
            "用户可以在线查看诊疗结果，减少线下反复咨询。",
        ],
        "预约与订单查询",
    ],
    13: [
        "医生端",
        "查看预约列表，确认到院用户，录入诊疗内容并创建订单。",
        "仓库端",
        "维护药品和物品资料，处理库存数量、价格和预警信息。",
        "后台端",
        "维护用户、角色权限、考勤记录和系统日志。",
        "数据联动",
        "订单创建后生成药品明细，并同步影响库存数量。",
        "4类",
        "角色",
        "7组",
        "模块",
    ],
    14: [
        "认证接口",
        "查询 users 表并校验密码，登录成功后返回 Token 和角色信息。",
        "预约接口",
        "保存预约前先进行时间冲突校验，保证医生接诊时间不被重复占用。",
        "库存接口",
        "warehouse 表支持物品新增、修改、删除、查询和库存预警。",
        "权限接口",
        "后台可授予或移除医生、仓库管理员等角色权限。",
    ],
    15: [
        "System Test",
        "本章通过功能测试和性能测试，验证系统主要流程能否稳定完成。",
        "系统测试",
        "PART.04",
    ],
    16: [
        "测试目标",
        "检查登录注册、预约提交、订单创建、库存维护、权限授予和日志查询是否符合预期。",
        "测试方法",
        "以黑盒测试为主，按照不同角色的实际操作路径逐项验证页面、接口和数据结果。",
    ],
    17: [
        "用户模块测试",
        "验证注册、登录、宠物档案、预约提交和订单查询。",
        "01",
        "医生模块测试",
        "验证预约查看、接诊处理、诊疗订单创建和诊单查询。",
        "02",
        "仓库模块测试",
        "验证库存维护、预警查看、数量价格调整和订单联动。",
        "03",
        "后台模块测试",
        "验证用户管理、权限授予、考勤记录和操作日志。",
        "04",
    ],
    18: [
        "异常输入",
        "测试错误密码、重复预约、库存非法输入等情况。",
        "权限控制",
        "检查不同角色是否只能进入自己对应的功能模块。",
        "数据联动",
        "验证预约同步医生端，订单创建后库存数量同步变化。",
        "性能测试",
        "使用 JMeter 观察响应时间、吞吐量和错误率。",
    ],
    19: [
        "总结与展望",
        "完成工作：实现了预约、接诊、订单、库存、权限、考勤和日志等核心功能。",
        "系统特点：多角色分工清晰，预约、订单和库存之间能够形成数据联动。",
        "后续方向：继续完善排班提醒、就诊记录追踪、数据统计和并发性能优化。",
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


def set_align(sp, align):
    for ppr in sp.findall(".//a:pPr", NS):
        ppr.set("algn", align)


def apply_slide(root, slide_no, replacements):
    shapes = text_shapes(root)
    for sp, text in zip(shapes, replacements):
        set_text(sp, text)

    # Fix known reversed template slots.
    if slide_no == 8 and len(shapes) >= 4:
        for idx, pt, bold, align in [(0, 16, True, "l"), (1, 11, False, "l"), (2, 14, False, "l"), (3, 32, True, "l")]:
            set_font(shapes[idx], pt, bold)
            set_align(shapes[idx], align)
    elif slide_no == 12 and len(shapes) >= 4:
        set_font(shapes[0], 16, True)
        set_font(shapes[1], 11, False)
        set_font(shapes[2], 11, False)
        set_font(shapes[3], 16, True)
        for sp in shapes:
            set_align(sp, "l")
    else:
        # Lightly compress body text so slides read like speaking notes, not paper paragraphs.
        for sp in shapes:
            text = "".join(t.text or "" for t in sp.findall(".//a:t", NS))
            if len(text) > 28:
                set_font(sp, 11 if slide_no not in (3, 7, 11, 15, 19) else 12, False)


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: make_defense_optimized_version.py <input.pptx> <output.pptx>")
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
