#!/usr/bin/env python3
import re
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


SLIDE_TEXTS = {
    1: [
        "SOUTH CHINA NORMAL UNIVERSITY",
        "宠物医院管理系统\n汇报人：杨航",
        "SC\nNU\n ",
    ],
    2: [
        "需求分析",
        "01",
        "Requirements Analysis",
        "系统设计",
        "02",
        "System Design",
        "系统实现",
        "03",
        "System Realization",
        "系统测试",
        "04",
        "System Test",
        "CONTENTS",
        "目录",
    ],
    3: [
        "Requirements Analysis",
        "第3章主要分析系统使用对象、功能需求、非功能需求和可行性，明确普通用户、医生、仓库管理员、超级管理员四类角色的业务边界。",
        "需求分析",
        "PART.01",
    ],
    4: [
        "业务背景",
        "中小型宠物医院在预约、接诊、库存和后台管理中仍存在人工沟通多、记录分散、查询不及时等问题。",
        "角色划分",
        "系统面向普通用户、医生、仓库管理员和超级管理员，不同角色进入不同页面并使用对应功能。",
        "核心需求",
        "围绕预约挂号、医生接诊、诊疗开单、库存维护、权限管理、考勤与日志等业务展开。",
        "需求分析从实际业务出发，把宠物医院日常流程拆分为用户端、医生端、仓库端和后台端四类操作，为后续系统设计提供依据。",
        "Requirements Analysis",
        "需求分析",
    ],
    5: [
        "用户端",
        "注册登录、宠物资料维护、在线预约、预约记录查看、订单与诊疗记录查询。",
        "医生端",
        "签到签退、查看预约列表、查询用户和宠物信息、创建诊疗订单。",
        "仓库端",
        "药品和物品信息维护、库存数量与价格管理、低库存和临期预警。",
        "后台端",
        "用户管理、权限授予、考勤记录查看、操作日志审计。",
        "四类角色功能相互独立，但预约、订单、库存和日志数据会在业务流程中产生关联。",
    ],
    6: [
        "非功能需求",
        "系统需要支持身份认证、角色权限控制、异常提示、数据一致性和基本并发访问能力。",
        "可行性分析",
        "技术上采用 Vue3、C++ Crow、MySQL、JWT 和短信验证码；成本主要来自服务器、短信服务和后期维护。",
        "业务价值",
        "系统功能与宠物医院预约、接诊、开单、库存和后台管理流程相对应，具备实际使用场景。",
        "通过需求分析可以看出，本系统不是单独实现某个页面功能，而是围绕宠物医院日常业务建立一套多角色管理流程。",
    ],
    7: [
        "System Design",
        "第4章从总体结构、功能模块、核心算法、接口、数据库和数据流等方面进行设计，为系统开发确定整体实现思路。",
        "系统设计",
        "PART.02",
    ],
    8: [
        "系统总体架构",
        "前端采用 Vue3 负责页面展示和交互，后端采用 C++ 与 Crow 提供 RESTful 接口，MySQL 用于保存业务数据，Python 脚本辅助完成短信验证码发送。\n整体结构按照前端页面、后端接口、数据库存储和外部短信服务展开。",
        "架构特点",
        "系统按用户端、医生端、仓库端和后台端划分页面；后端按认证、用户、预约、订单、医生、仓库、后台等模块组织路由和处理逻辑。\n各模块通过统一接口交互，便于后续维护和功能扩展。",
    ],
    9: [
        "身份认证与权限校验",
        "登录成功后生成 Token，后续接口请求先解析用户身份和角色，限制越权访问。",
        "预约冲突校验",
        "创建预约前先根据医生、日期和时间段查询已有记录，避免同一医生同一时间被重复预约。",
        "订单与库存联动",
        "医生创建诊疗订单时，系统检查药品库存并在订单成功后扣减对应数量。",
        "操作日志记录",
        "关键接口执行后记录操作用户、模块、结果和异常信息，便于后期排查问题。",
    ],
    10: [
        "数据库设计",
        "用户、宠物、预约、订单、订单药品明细、仓库、医生排班、工时记录和操作日志等数据表支撑主要业务。",
        "表关系设计",
        "用户与宠物、用户与预约、订单与药品明细、药品明细与库存之间通过编号字段建立联系。",
        "接口设计",
        "内部接口完成前后端业务通信，外部接口主要用于短信验证码服务调用。",
    ],
    11: [
        "System Realization",
        "第5章展示系统主要功能模块的实现过程，包括用户端、医生端、仓库端和后台端的页面与关键接口。",
        "\t\n系统实现",
        "PART.03",
    ],
    12: [
        "用户端实现",
        "用户端实现注册、登录、个人信息维护、宠物信息管理、在线预约和订单查询。\n注册与登录流程结合短信验证码、密码校验和 Token 保存完成身份识别。",
        "预约与订单查询",
        "用户提交预约后，后端保存预约记录并同步到医生端；订单查询时根据用户名下宠物关联查询诊疗订单和药品明细。\n该模块重点解决用户自主预约和查看诊疗记录的问题，减少线下反复咨询。",
    ],
    13: [
        "医生端",
        "医生查看预约列表、确认到院用户、录入诊疗内容并创建订单。",
        "仓库端",
        "仓库管理员维护物品资料，处理库存数量、价格和预警信息。",
        "后台端",
        "超级管理员维护用户、角色权限、考勤记录和系统日志。",
        "数据联动",
        "订单创建后同步生成药品明细并扣减库存数量。",
        "4类",
        "角色",
        "7组",
        "模块",
    ],
    14: [
        "注册登录",
        "认证接口查询 users 表并校验密码，登录成功后返回 Token 和角色信息。",
        "预约管理",
        "预约提交前进行医生、日期、时间段校验，避免重复占用医生时间。",
        "库存维护",
        "warehouse 表保存物品名称、类型、价格、数量和有效期，支持增删改查。",
        "权限授予",
        "后台可授予或移除医生、仓库管理员等角色权限，并刷新用户列表。",
    ],
    15: [
        "System\n \nTest",
        "第6章对系统测试环境、测试方法、功能测试和性能测试进行说明，验证系统主要流程是否能够按预期运行。",
        "系统测试",
        "PART.04",
    ],
    16: [
        "测试目标",
        "检查系统在注册登录、预约提交、订单创建、库存维护、权限授予和日志查询等场景下的处理结果。",
        "测试方法",
        "采用功能测试为主，按照普通用户、医生、仓库管理员和超级管理员四类角色的实际操作流程进行验证。",
    ],
    17: [
        "用户模块测试",
        "验证注册、登录、个人信息维护、宠物资料和预约记录等功能是否正常。",
        "01",
        "医生模块测试",
        "验证预约列表、接诊、诊疗订单创建和诊单查询等功能。",
        "02",
        "仓库模块测试",
        "验证物品维护、库存预警、数量价格调整和订单联动。",
        "03",
        "后台模块测试",
        "验证用户管理、权限授予、考勤记录和操作日志查询。",
        "04",
    ],
    18: [
        "输入校验",
        "对错误密码、重复预约、库存非法输入等异常情况进行验证。",
        "权限控制",
        "检查不同角色是否只能访问对应页面和接口。",
        "数据联动",
        "验证预约同步医生端、订单创建后关联库存变化。",
        "性能观察",
        "使用 JMeter 对核心接口进行并发测试，观察响应时间、吞吐量和错误率。",
    ],
    19: [
        "总结与展望",
        "本文完成了一套基于 C++ 与 Vue3 的宠物医院管理系统，实现用户预约、医生接诊、订单创建、库存维护、权限管理、考勤和日志等功能。",
        "系统当前能够覆盖中小型宠物医院的基本业务流程。后续可继续完善医生排班、预约提醒、就诊记录追踪、性能优化和数据分析等功能。",
        "系统通过多角色分工和数据联动，将预约、接诊、订单和库存流程连接起来，提高日常管理效率。",
    ],
    20: ["谢谢观看"],
}


def get_text_shapes(root):
    shapes = []
    for sp in root.findall(".//p:sp", NS):
        if sp.findall(".//a:t", NS):
            shapes.append(sp)
    return shapes


def set_shape_text(sp, text):
    tx_body = sp.find("p:txBody", NS)
    if tx_body is None:
        return
    first_p = tx_body.find("a:p", NS)
    first_r = first_p.find("a:r", NS) if first_p is not None else None
    rpr = first_r.find("a:rPr", NS) if first_r is not None else None
    ppr = first_p.find("a:pPr", NS) if first_p is not None else None

    for p in list(tx_body.findall("a:p", NS)):
        tx_body.remove(p)

    lines = text.split("\n")
    for line in lines:
        p = ET.SubElement(tx_body, f"{{{A}}}p")
        if ppr is not None:
            p.append(ET.fromstring(ET.tostring(ppr)))
        r = ET.SubElement(p, f"{{{A}}}r")
        if rpr is not None:
            r.append(ET.fromstring(ET.tostring(rpr)))
        else:
            ET.SubElement(r, f"{{{A}}}rPr", {"lang": "zh-CN", "sz": "1800"})
        t = ET.SubElement(r, f"{{{A}}}t")
        t.text = line


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: fill_defense_ppt.py <template.pptx> <output.pptx>")
    src = Path(sys.argv[1])
    dst = Path(sys.argv[2])
    shutil.copyfile(src, dst)
    with zipfile.ZipFile(src, "r") as zin:
        entries = {info.filename: zin.read(info.filename) for info in zin.infolist()}
        infos = zin.infolist()

    for slide_no, replacements in SLIDE_TEXTS.items():
        name = f"ppt/slides/slide{slide_no}.xml"
        root = ET.fromstring(entries[name])
        shapes = get_text_shapes(root)
        if len(shapes) != len(replacements):
            print(f"slide {slide_no}: shapes={len(shapes)} replacements={len(replacements)}")
        for sp, text in zip(shapes, replacements):
            set_shape_text(sp, text)
        entries[name] = ET.tostring(root, encoding="utf-8", xml_declaration=True)

    with zipfile.ZipFile(dst, "w") as zout:
        for info in infos:
            zout.writestr(info, entries[info.filename])
    print(dst)


if __name__ == "__main__":
    main()
