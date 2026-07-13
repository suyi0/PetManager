export interface PermissionMeta {
  label: string;
  desc: string;
  group: string;
  danger?: boolean;
}

export const PERMISSION_META: Record<string, PermissionMeta> = {
  "portal:boss": { group: "端口入口", label: "总裁端入口", desc: "可登录进入总裁端。只控制入口，不包含端内具体数据权限。" },
  "portal:super-admin": { group: "端口入口", label: "管理端入口", desc: "可进入超级管理员后台，登录会话按管理端短时效处理。" },
  "portal:finance": { group: "端口入口", label: "财务端入口", desc: "可进入财务端并加载财务首页数据。" },
  "portal:personnel": { group: "端口入口", label: "人事端入口", desc: "可进入人事端的权限授予与岗位管理页面。" },
  "portal:medical": { group: "端口入口", label: "医疗端入口", desc: "可进入医疗端；具体工作台由职工工种决定。" },
  "portal:warehouse": { group: "端口入口", label: "仓库端入口", desc: "可进入仓库管理端的库存、预警与出入库页面。" },
  "portal:user": { group: "端口入口", label: "用户端入口", desc: "可进入普通用户端。" },
  "medical-record:read": { group: "诊疗文书", label: "查看诊疗单", desc: "查看诊疗文书、处方明细与历史版本。" },
  "medical-record:write": { group: "诊疗文书", label: "填写诊疗单", desc: "保存诊疗单草稿；仅草稿可修改。" },
  "medical-record:finalize": { group: "诊疗文书", label: "定稿诊疗单", desc: "定稿并生成 PDF 归档，业务内容不可原地修改。" },
  "medical-record:print": { group: "诊疗文书", label: "打印诊疗单", desc: "下载或打印已归档的诊疗单 PDF。" },
  "medical-record:amend": { group: "诊疗文书", label: "补充修订诊疗单", desc: "对已定稿诊疗单追加修订版本并保留原因。" },
  "medical-record:void": { group: "诊疗文书", label: "作废诊疗单", desc: "作废已定稿诊疗单，业务上不可逆。", danger: true },
  "report-template:read": { group: "打印模板", label: "查看打印模板", desc: "查看诊疗单打印模板、版本与字段目录。" },
  "report-template:manage": { group: "打印模板", label: "编辑打印模板", desc: "新建或编辑模板草稿并预览，不含发布。" },
  "report-template:publish": { group: "打印模板", label: "发布打印模板", desc: "发布或回滚模板版本，影响后续新定稿文书。", danger: true },
  "doctor-work:write": { group: "医生工作台", label: "医生工作操作", desc: "允许接诊、叫号、创建订单等写操作。" },
  "attendance:read": { group: "考勤管理", label: "查看考勤", desc: "查看考勤记录、统计与设备状态。" },
  "attendance:manage": { group: "考勤管理", label: "管理考勤", desc: "补卡、调整班次并管理考勤设备。" },
  "salary:read": { group: "薪资与股份", label: "查看薪资", desc: "查看员工薪资记录与月度报表。" },
  "salary:write": { group: "薪资与股份", label: "管理薪资", desc: "新增或调整员工薪资记录，涉及资金发放。", danger: true },
  "salary:review": { group: "薪资与股份", label: "审核薪资", desc: "执行工资初审并提交主管复审。", danger: true },
  "salary:lock": { group: "薪资与股份", label: "锁定工资周期", desc: "锁定并归档已完成复审的工资周期。", danger: true },
  "equity:read": { group: "薪资与股份", label: "查看股份", desc: "查看股份与权益数据。" },
  "equity:write": { group: "薪资与股份", label: "管理股份", desc: "调整股份与权益数据。", danger: true },
  "stock:read": { group: "库存物资", label: "查看库存", desc: "查看库存、台账与预警。" },
  "stock:write": { group: "库存物资", label: "管理库存", desc: "入库、出库及调整库存和药品资料。" },
  "staff-role:write": { group: "组织与用户", label: "员工任职管理", desc: "给员工派岗或调岗。" },
  "user:delete": { group: "组织与用户", label: "删除用户", desc: "软删除用户账号，可审计但影响账号使用。", danger: true },
  "rbac:manage": { group: "组织与用户", label: "权限组织管理", desc: "管理部门、职位、权限与用户派岗。", danger: true },
  "scope:all": { group: "数据范围", label: "全组织数据范围", desc: "放大到全部分院与部门，影响列表与统计过滤。", danger: true },
  "scope:medical-assigned": { group: "数据范围", label: "仅本人接诊范围", desc: "医疗数据仅限分配给本人的患者与订单。" },
  "logs:read": { group: "审计日志", label: "查看审计日志", desc: "查看全系统操作日志与敏感操作记录。" },
};

export const GROUP_ORDER = ["端口入口", "诊疗文书", "打印模板", "医生工作台", "考勤管理", "薪资与股份", "库存物资", "组织与用户", "数据范围", "审计日志", "其他"];

export const permissionLabel = (key: string) => PERMISSION_META[key]?.label ?? key;

export const permissionGroups = (keys: string[]) => {
  const buckets = new Map<string, string[]>();
  keys.forEach((key) => {
    const group = PERMISSION_META[key]?.group ?? "其他";
    buckets.set(group, [...(buckets.get(group) ?? []), key]);
  });
  return GROUP_ORDER.filter((group) => buckets.has(group)).map((name) => ({ name, keys: buckets.get(name) ?? [] }));
};
