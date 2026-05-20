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


SLIDES = {
    3: [
        "Requirements Analysis",
        "需求分析不是简单列功能，而是从宠物医院实际流程出发，确定系统需要支撑的角色、业务边界和运行约束。",
        "需求分析",
        "PART.01",
    ],
    4: [
        "问题来源",
        "线下预约容易出现时间冲突，接诊信息和库存数据分散，后期追溯也不方便。",
        "角色定位",
        "系统把日常使用者分为普通用户、医生、仓库管理员和超级管理员。",
        "设计目标",
        "让预约、接诊、开单、库存变化和后台审计形成一条连续业务链。",
        "本系统需求的重点不是单个页面能否使用，而是不同岗位之间的数据能否顺利流转。",
        "Requirements Analysis",
        "需求分析",
    ],
    5: [
        "普通用户",
        "维护个人和宠物资料，提交预约，查看预约记录与诊疗订单。",
        "医生",
        "查看预约列表，确认接诊对象，录入诊疗信息并创建订单。",
        "仓库管理员",
        "维护药品物品资料，处理库存数量、价格、低库存和临期预警。",
        "超级管理员",
        "维护用户、角色权限、考勤记录和系统操作日志。",
        "四类角色对应宠物医院的主要岗位，数据从用户预约开始，最终流向订单、库存和后台日志。",
    ],
    6: [
        "安全约束",
        "使用 Token 识别身份，结合角色权限限制不同用户的访问范围。",
        "数据约束",
        "预约、订单、库存等数据需要保持关联，避免重复预约和库存扣减错误。",
        "实现可行",
        "Vue3、C++ Crow、MySQL、JWT 和短信验证码均已在项目中落地。",
        "从需求、技术和业务场景看，系统具备完成中小型宠物医院基础管理的条件。",
    ],
    7: [
        "System Design",
        "系统设计围绕业务闭环展开：前端负责操作入口，后端负责业务规则，数据库负责保存和关联核心数据。",
        "系统设计",
        "PART.02",
    ],
    8: [
        "架构设计",
        [
            "前端：Vue3 负责用户端、医生端、仓库端和后台端页面。",
            "后端：C++ Crow 提供 RESTful 接口，集中处理认证、预约、订单、库存和日志逻辑。",
            "数据层：MySQL 保存用户、宠物、预约、订单、药品明细、库存和工时日志等数据。",
        ],
        "System Architecture",
        "系统总体架构",
    ],
    9: [
        "身份认证",
        "登录后生成 Token，接口层解析身份与角色，减少越权访问风险。",
        "预约冲突",
        "创建预约前按医生、日期、时间段查询记录，避免同一医生重复占用。",
        "订单库存",
        "医生开单时生成订单明细，并根据药品使用情况同步扣减库存。",
        "日志审计",
        "记录登录、修改、授权和异常操作，为后期维护提供追踪依据。",
    ],
    10: [
        "数据库设计",
        "围绕用户、宠物、预约、订单、药品明细、仓库、排班、工时和日志等表展开。",
        "关联关系",
        "用户关联宠物和预约，订单关联宠物、医生和药品明细，药品明细再关联库存。",
        "接口设计",
        "内部接口支撑前后端业务交互，外部接口主要服务于短信验证码发送。",
    ],
    11: [
        "System Realization",
        "系统实现阶段重点验证设计是否落地：页面操作、接口处理和数据库变化需要相互对应。",
        "系统实现",
        "PART.03",
    ],
    12: [
        "用户端实现",
        [
            "注册登录：校验账号、密码和验证码，登录后保存 Token。",
            "宠物档案：用户维护宠物基础信息，作为预约和订单查询的基础。",
            "在线预约：选择医生、日期和时间段，后端完成冲突校验后保存记录。",
        ],
        [
            "预约记录进入医生端，医生可根据列表安排接诊。",
            "诊疗订单与宠物信息关联，用户可以回到用户端查看诊疗结果。",
            "用户端解决的是“从预约到查看诊疗结果”的入口问题。",
        ],
        "预约与订单查询",
    ],
    13: [
        "医生端",
        "承接用户预约，完成接诊确认、诊疗信息录入和订单创建。",
        "仓库端",
        "维护库存基础资料，并根据订单药品明细观察库存变化。",
        "后台端",
        "负责人事权限、用户管理、考勤记录和操作日志审计。",
        "数据联动",
        "预约记录推动接诊流程，订单明细推动库存变化，日志记录后台操作。",
        "4类",
        "角色",
        "7组",
        "模块",
    ],
    14: [
        "注册登录",
        "认证接口查询 users 表，校验密码后返回 Token、用户信息和角色类型。",
        "预约管理",
        "预约接口先检查医生时间段是否冲突，再决定是否写入预约记录。",
        "库存维护",
        "warehouse 表保存物品数量、价格和有效期，支持维护与预警展示。",
        "权限授予",
        "后台根据用户编号调整角色权限，并通过日志记录关键操作。",
    ],
    15: [
        "System Test",
        "测试部分用于验证系统是否真正可用，重点关注功能结果、异常处理、权限边界和数据联动。",
        "系统测试",
        "PART.04",
    ],
    16: [
        "测试目标",
        "验证登录注册、预约提交、医生开单、库存维护、权限授予和日志查询等核心流程。",
        "测试方法",
        "以黑盒测试为主，从普通用户、医生、仓库管理员和超级管理员四类角色分别操作。",
    ],
    17: [
        "用户模块测试",
        "验证注册登录、宠物档案维护、预约提交和订单查询是否符合预期。",
        "01",
        "医生模块测试",
        "验证预约查看、接诊处理、诊疗订单创建和诊单查询。",
        "02",
        "仓库模块测试",
        "验证物品维护、库存预警、数量价格调整和订单联动。",
        "03",
        "后台模块测试",
        "验证用户管理、权限授予、考勤记录和操作日志查询。",
        "04",
    ],
    18: [
        "异常输入",
        "检查错误密码、重复预约、库存非法输入等场景下的提示和拦截。",
        "权限控制",
        "验证普通用户、医生、仓库管理员和管理员只能访问对应模块。",
        "数据联动",
        "观察预约同步医生端、订单创建后库存扣减是否正确。",
        "性能测试",
        "使用 JMeter 观察核心接口在并发访问下的响应时间、吞吐量和错误率。",
    ],
    19: [
        "总结与展望",
        "本文完成了宠物医院管理系统的需求分析、系统设计、功能实现和测试验证。",
        "系统价值在于将预约、接诊、订单、库存、权限和日志连接起来，形成较完整的业务闭环。",
        "后续可继续完善排班提醒、诊疗记录追踪、统计分析和高并发性能优化。",
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
    for sp in shapes:
        text = "".join(t.text or "" for t in sp.findall(".//a:t", NS))
        if len(text) > 34:
            set_font(sp, 11, False)
    if slide_no in (3, 7, 11, 15):
        if len(shapes) > 2:
            set_font(shapes[2], 60, True)
        if len(shapes) > 1:
            set_font(shapes[1], 12, False)
    if slide_no == 8 and len(shapes) >= 4:
        set_font(shapes[0], 16, True)
        set_font(shapes[1], 10, False)
        set_font(shapes[2], 14, False)
        set_font(shapes[3], 32, True)
        for sp in shapes:
            set_align(sp, "l")
    if slide_no == 12 and len(shapes) >= 4:
        set_font(shapes[0], 16, True)
        set_font(shapes[1], 10, False)
        set_font(shapes[2], 10, False)
        set_font(shapes[3], 16, True)
        for sp in shapes:
            set_align(sp, "l")


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: make_defense_substantive_version.py <input.pptx> <output.pptx>")
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
