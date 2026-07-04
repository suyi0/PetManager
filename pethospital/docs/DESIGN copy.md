# FactoryERP 最终方案（v1 — 员工信息管理）

> 状态：**已定稿（v2，含 YANG 追加的工号编码需求）**。初稿见 `docs/DESIGN-v1-draft.md`；本版为 Claude（主导设计）与 Codex（评审）两轮讨论收敛后的最终方案，评审改动见文末「评审记录」。实现分工见 `docs/WORKPLAN.md`。
> 借鉴项目：`~/Code/PetManager`（只借工程结构与实现模式，不复制任何密钥值）。

## 0. 结论先行

- 新建独立项目 `~/Code/FactoryERP`：**后端 C++17 + Crow + MySQL Connector/C++ + OpenSSL**，**前端 Vite + Vue 3 + TypeScript + Pinia + Element Plus**。
- v1 只做：**管理员登录 + 员工信息列表管理**（增删改查、按月薪工/时薪工筛选、分页、搜索）。
- 月薪工/时薪工：**单表 `employees` + `pay_type` 枚举**区分，不拆表。
- 身份证号**加密落库**（AES-256-GCM）+ HMAC 哈希做唯一校验，接口默认脱敏，查看完整号/编辑身份证写审计日志。
- 工号编码入职快照：`{分厂码 1 字母}{车间码 1 字母}{入职年份 2 位}{序号 4 位零填充}`（如 `AB260007`），按前缀分序列、事务取号，禁止 `MAX+1`；支持前缀搜索。
- 权限演进方向：v1 只做单管理员；后期 ERP 权限采用 **RBAC（角色/功能权限）+ Scope（组织数据范围）+ 敏感动作单独授权**，详见 §4.6。

## 1. 要解决的问题与边界

**本质问题**：工厂需要员工花名册系统，HR/管理员维护员工基本信息；月薪工与时薪工薪酬口径不同，录入、展示、筛选需区分。

**v1 范围（做）**：
- 管理员账号登录（JWT）。
- 员工列表：分页、按姓名/工号/手机号搜索、按薪酬类型与在职状态筛选。
- 员工新增 / 编辑 / 离职（软删除）/ 复职 / 详情。
- 字段校验：身份证 18 位含校验位、手机号 11 位、邮箱格式。
- 最小审计日志（仅覆盖身份证查看/编辑两类高敏感操作）。

**v1 范围补充**：
- 分厂/车间最小组织数据（`factories` / `workshops` 种子表 + 下拉接口），支撑工号编码与录入表单。

**v1 不做（明确排除）**：
- 考勤、排班、计时、工资核算与发放（时薪工工时记录属 v2）。
- 多角色权限体系（`admin_users.role` 仅保留 v1 兼容值 `'admin'`；v2+ 不继续扩展字符串角色，改走 §4.6 的角色/权限/scope 关系表）。
- 组织管理 UI（分厂/车间由 migration 种子维护，v1 不做增删改页面）。
- 部门树/组织架构（部门为文本字段）。
- 字典表管理页、短信/邮件通知、身份证加密**密钥轮换**（已知限制，见 4.4）。

**后期权限边界（v2+ 方向，v1 不实现）**：
- 权限边界按 §4.6 设计：**身份认证 → 功能权限 → 组织 Scope → 敏感动作授权 → 审计**。
- `admin_users.role` 只作为 v1 兼容字段，不继续扩成大字符串判断；v2+ 迁移到角色、权限、scope 关系表。
- 若员工后期支持转厂/转车间，权限过滤基于「当前管理归属」，不能基于工号或 `factory_code`/`workshop_code` 入职快照。

## 2. 技术栈（已定）

| 层 | 选型 | 为什么 |
|---|---|---|
| 后端 | C++17 + Crow + MySQL Connector/C++ + OpenSSL | 与 PetManager 一致，YANG 能读懂，构建脚本、中间件、DB 封装模式可直接借鉴 |
| 数据库 | MySQL 8，本机已有实例，新建库 `factory_erp` | 零新增运维；靠库名与 PetManager 隔离 |
| 前端 | Vite + Vue 3 + TS + Vue Router + Pinia + Element Plus | Vue CLI 已停维护，新项目不背历史包袱；Pinia 为 Vuex 官方继任；Element Plus 让后台表格/表单开箱即用。借鉴 PetManager 的**模块分层**而非其构建链 |
| 鉴权 | JWT + `requireAdmin` middleware（`backend/middleware/Auth.*`，借鉴 PetManager） | 已验证模式 |
| 缓存/消息 | 不引入 Redis | v1 单实例，无跨实例广播需求；多实例部署时再补 |

## 3. 目录结构

比 PetManager 少一层嵌套（其 `pethospital/` 包裹层是历史产物）：

```
FactoryERP/
├── CLAUDE.md / AGENTS.md          # 双入口 → .agents/context/*
├── .agents/context/               # PROJECT / ARCHITECTURE / COMMANDS / BACKEND / FRONTEND
├── .env.example                   # 模板；真实 .env 不进 git
├── bin/
│   ├── build.sh                   # cmake 配置 + 编译（-DBUILD_TESTING=OFF）
│   └── start.sh                   # 加载 .env → build → 启动后端（启动时按需 seed 管理员）
├── docs/                          # DESIGN-v1-draft.md / DESIGN.md（本文件）
├── backend/
│   ├── CMakeLists.txt             # 借鉴 PetManager 的 Homebrew 探测；删去 curl/短信/Python 桥接
│   ├── main.cpp
│   ├── routes/
│   │   ├── Routes.h
│   │   └── Routes.cpp             # 注册 /api/auth/* 与 /api/admin/*
│   ├── controllers/
│   │   ├── auth/                  # AuthHandlers：登录、JWT 签发、管理员 seed
│   │   └── admin/                 # EmployeeHandlers：员工与组织接口
│   ├── middleware/
│   │   ├── Auth.{h,cpp}           # requireAdmin：JWT 校验
│   │   └── Cors.h
│   ├── database/
│   │   ├── Db.{h,cpp}             # X DevAPI session + 启动迁移
│   │   └── Migrations.{h,cpp}     # 内嵌迁移 SQL，按序执行，历史迁移不可改
│   └── utils/                     # 身份证/手机号校验、JWT、PBKDF2、AES-GCM/HMAC、响应封装
└── frontend/
    ├── vite.config.ts
    └── src/
        ├── main.ts / App.vue      # bootstrap 与根组件
        ├── app/                   # router 与主布局
        ├── core/auth/             # 登录页、token 管理、路由守卫
        ├── modules/hr/            # api / store(Pinia) / views
        │   └── views/
        │       ├── EmployeeListPage.vue
        │       └── components/
        │           ├── EmployeeFormDialog.vue
        │           └── EmployeeDetailDrawer.vue
        └── shared/                # http 封装、枚举映射、校验、组织预设
```

## 4. 数据库设计

### 4.1 月薪工 vs 时薪工：单表 + 枚举（核心决策）

一张 `employees` 表，`pay_type ENUM('monthly','hourly')` 区分，薪酬字段按类型二选一。

**为什么不拆两张表**：两类员工 90% 字段相同，差异仅 1-2 个薪酬字段；拆表让最高频的"全员列表"变 UNION，增删改写两套。**为什么不做 EAV/继承子表**：v1 差异极少，过度设计；v2 时薪工挂 `work_time_logs` 子表即可，不影响本表。

**代价**：互斥字段并存一表，靠 CHECK 约束 + 应用层校验兜底。

### 4.2 表结构

```sql
-- 管理员账号
CREATE TABLE admin_users (
    id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    username      VARCHAR(64)  NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,   -- $scheme$iterations$salt$hash，沿用 PetManager 格式
    role          VARCHAR(32)  NOT NULL DEFAULT 'admin',   -- v1 兼容字段；v2+ 应迁移到 roles/permissions/scopes
    created_at    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 分厂 / 车间（最小组织数据，migration 种子初始化，v1 无管理 UI）
CREATE TABLE factories (
    code    CHAR(1)      PRIMARY KEY,      -- 大写字母，进工号
    name    VARCHAR(64)  NOT NULL,
    address VARCHAR(255) NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE workshops (
    factory_code CHAR(1)     NOT NULL,
    code         CHAR(1)     NOT NULL,     -- 大写字母，进工号
    name         VARCHAR(64) NOT NULL,
    PRIMARY KEY (factory_code, code),
    FOREIGN KEY (factory_code) REFERENCES factories(code)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 工号序列：按「分厂+车间+年份」前缀隔离取号（防并发撞号，见 4.3）
CREATE TABLE employee_no_sequences (
    prefix     VARCHAR(8) PRIMARY KEY,     -- 如 'AB26'
    next_value BIGINT UNSIGNED NOT NULL
) ENGINE=InnoDB;

-- 员工
CREATE TABLE employees (
    id             BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    employee_no    VARCHAR(16)  NOT NULL UNIQUE,        -- {分厂}{车间}{YY}{4位序号}，见 4.3
    factory_code   CHAR(1)      NOT NULL,               -- 入职快照，调岗/转厂不改（与工号一致）
    workshop_code  CHAR(1)      NOT NULL,               -- 入职快照，调岗/转车间不改（与工号一致）
    name           VARCHAR(64)  NOT NULL,
    gender         ENUM('male','female') NOT NULL,
    -- 身份证号：AES-256-GCM 加密落库；HMAC-SHA256 哈希做唯一校验（见 4.4）
    -- 密文相关列以 hex 字符串存储（实现取舍：X DevAPI 绑定与排查更简单）
    id_card_ciphertext VARCHAR(128) NOT NULL,
    id_card_nonce      CHAR(24)     NOT NULL,
    id_card_tag        CHAR(32)     NOT NULL,
    id_card_hash       CHAR(64)     NOT NULL UNIQUE,
    phone          VARCHAR(16)  NOT NULL,
    email          VARCHAR(128) NULL,
    marital_status ENUM('single','married','divorced','widowed') NOT NULL DEFAULT 'single',
    pay_type       ENUM('monthly','hourly') NOT NULL,
    monthly_salary_cents BIGINT UNSIGNED NULL,          -- 月薪（分）
    hourly_rate_cents    BIGINT UNSIGNED NULL,          -- 时薪（分）
    department     VARCHAR(64)  NULL,
    position       VARCHAR(64)  NULL,
    hire_date      DATE         NOT NULL,
    status         ENUM('active','resigned') NOT NULL DEFAULT 'active',
    resigned_at    DATE         NULL,
    created_at     TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at     TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    CONSTRAINT chk_pay_fields CHECK (
        (pay_type = 'monthly' AND monthly_salary_cents IS NOT NULL AND hourly_rate_cents IS NULL) OR
        (pay_type = 'hourly'  AND hourly_rate_cents    IS NOT NULL AND monthly_salary_cents IS NULL)
    ),
    CONSTRAINT chk_resigned CHECK (
        (status = 'active'   AND resigned_at IS NULL) OR
        (status = 'resigned' AND resigned_at IS NOT NULL)
    ),
    INDEX idx_pay_type_status (pay_type, status),
    INDEX idx_factory_workshop (factory_code, workshop_code),
    INDEX idx_name (name),
    INDEX idx_phone (phone)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 最小审计日志（v1 仅两类动作：view_id_card / update_id_card）
CREATE TABLE audit_logs (
    id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    operator_id BIGINT UNSIGNED NOT NULL,       -- admin_users.id
    employee_id BIGINT UNSIGNED NOT NULL,       -- employees.id
    action      ENUM('view_id_card','update_id_card') NOT NULL,
    field       VARCHAR(32)  NULL,              -- 'id_card_no'
    old_masked  VARCHAR(32)  NULL,              -- 只存脱敏值，绝不存完整号
    new_masked  VARCHAR(32)  NULL,
    created_at  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_employee (employee_id),
    INDEX idx_operator (operator_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

- **金额用整数分**，避免浮点误差。
- **后续加字段**走 `backend/database/Migrations.cpp` 追加迁移 id，不改历史迁移 SQL。

### 4.3 工号规则与并发安全（YANG 追加需求后的 v2 规则）

**格式**：`{分厂码 1 位大写字母}{车间码 1 位大写字母}{入职年份后 2 位}{序号 4 位零填充}`

示例：`AB260007` = A 分厂 B 车间 2026 年入职的第 7 名员工。

- **工号编码的是入职时快照**：调岗、转厂、月薪转时薪均**不改工号**——工号仍是终身稳定标识，"稳定性"由"不编码可变属性"降级为"编码但永不变更"。
- **权限边界不能用工号前缀替代**：工号和 `factory_code`/`workshop_code` 表示入职快照；后期若员工转厂/转车间，应新增 `current_factory_code` / `current_workshop_code` 或员工组织归属历史表，权限 scope 和管理列表基于当前归属过滤。
- **解析规则按定长位置**：右起 4 位是序号，其余是前缀（分厂+车间+年份）。YANG 计划的"找第一个 '0'、向前取前缀"搜索**只能作为 UI 便捷近似**，不可作为后端正式规则——该启发式在前缀含 '0' 时失效（2030 年代起年份"30""40"必含 0）。前缀筛选一律走结构化列（`factory_code`/`workshop_code`/`hire_date` 年份）或 `employee_no LIKE '<定长前缀>%'`。
- **取号并发安全**：同一事务里 `SELECT next_value FROM employee_no_sequences WHERE prefix='AB26' FOR UPDATE` → 组装工号 → `next_value + 1` → 插入员工；前缀首次出现时 `INSERT ... ON DUPLICATE KEY` 初始化。**禁止 `SELECT MAX(employee_no)+1`**。
- **容量**：每车间每年 9999 人，溢出直接报错（v1 接受此上限，触及时升级序号位数走迁移）。
- 前端不可编辑工号；分厂/车间在新增表单中从 `GET /api/admin/org` 下拉选择。

### 4.4 身份证号：加密落库 + 脱敏访问（核心安全决策）

**存储**：
- 明文经 AES-256-GCM 加密，存 `id_card_ciphertext` / `id_card_nonce` / `id_card_tag`。
- 另存 `id_card_hash = HMAC-SHA256(id_card_no, ID_CARD_HMAC_KEY)` 做 UNIQUE 重复校验。**必须用 HMAC + 独立密钥而非裸 SHA256**：身份证号空间可枚举（地区码+生日+序号），裸哈希可被暴力反推。
- `.env` 中 `ID_CARD_ENC_KEY` 与 `ID_CARD_HMAC_KEY` 分开配置，各 32 字节（hex），用 `openssl rand -hex 32` 生成。

**为什么不用初稿的"明文 + 接口脱敏"**：v1 不需要按身份证模糊搜索，加密不损失任何核心功能；现在做比日后迁移明文存量数据干净得多。

**访问规则**：
- 列表、详情接口一律返回脱敏值（`3301**********1234`）。
- 查看完整号走显式端点（见第 5 节），并写 `view_id_card` 审计日志。
- 编辑时 `id_card_no` 字段可选：不传则不动；传了才校验 + 重新加密 + 写 `update_id_card` 审计日志（同一事务）。表单显示脱敏占位，要改就重新输入完整号。

**已知限制（v1 接受）**：
- 不做密钥轮换；密钥泄露或需换钥时须停机跑迁移脚本。
- **密钥丢失 = 全部身份证号不可解密**。`.env` 不进 git，YANG 须自行把两把密钥备份到密码管理器。

### 4.5 枚举取值清单（前后端共享，防漂移）

C++ 校验、SQL ENUM、Vue 下拉选项统一以此表为准：

| 字段 | 值 | 中文展示 |
|---|---|---|
| gender | male / female | 男 / 女 |
| marital_status | single / married / divorced / widowed | 未婚 / 已婚 / 离异 / 丧偶 |
| pay_type | monthly / hourly | 月薪工 / 时薪工 |
| status | active / resigned | 在职 / 离职 |

不做字典表：取值封闭且极少变化，字典表会引入管理页面、缓存与前后端同步成本。部门/岗位为自由文本。

### 4.6 权限边界设计（v2+，v1 不实现）

**结论**：ERP 权限不能只按菜单或角色字符串分。后期采用 **RBAC + Scope + Sensitive Permission**：

- **RBAC** 回答“这个账号能做什么”。
- **Scope** 回答“这个账号能看/管哪些组织数据”。
- **Sensitive Permission** 把身份证完整号、薪酬、导出、删除/离职等高风险动作从普通员工管理权限中拆出来。

#### 4.6.1 为什么这样做

- **为什么**：同样有“员工管理”功能的人，权限边界可能完全不同。HR 可能管全公司，分厂经理只能管本分厂，车间主管只能管本车间，财务能看薪酬但不一定能改身份证。把这些都塞进单个 `role` 会迅速膨胀成 `factory_a_hr_salary_viewer` 这类不可维护组合。
- **用户影响**：管理层只看到自己负责的数据，跨分厂员工、薪酬、身份证不会被误展示；权限申请和审计也能说清“谁因为什么权限访问了什么范围”。
- **代价与风险**：每个后端读写路径都要统一接入权限过滤；测试必须覆盖越权场景。前端隐藏按钮只改善体验，不能替代后端权限。

#### 4.6.2 数据模型（第五轮定稿：两张表 + 代码常量）

v2+ 不继续扩展 `admin_users.role` 的字符串分支。**角色→权限映射是代码常量**（`backend/utils/Permissions.h`），不建 `roles`/`permissions`/`role_permissions` 表——权限 key 本质是代码里的检查点，放数据库会造成"代码检查的 key"与"库里配的 key"两个真相源；没有权限编辑 UI 时 DB 映射是死数据。运行时可配置的只有两件事，各一张表：

```sql
-- 用户 ← 角色（多角色叠加：权限 = 各角色包并集，如 factory_manager + finance）
CREATE TABLE admin_user_roles (
    user_id  BIGINT UNSIGNED NOT NULL,
    role_key VARCHAR(64) NOT NULL,      -- 取值由代码校验：super_admin/hr_manager/...
    PRIMARY KEY (user_id, role_key),
    FOREIGN KEY (user_id) REFERENCES admin_users(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 用户 ← 组织 Scope（多行叠加；无任何 scope 行 = 看不到任何数据，安全默认）
CREATE TABLE admin_user_scopes (
    id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id       BIGINT UNSIGNED NOT NULL,
    scope_type    ENUM('company','factory','workshop') NOT NULL,
    factory_code  CHAR(1) NULL,         -- factory/workshop 型必填
    workshop_code CHAR(1) NULL,         -- workshop 型必填
    FOREIGN KEY (user_id) REFERENCES admin_users(id),
    INDEX idx_scope_user (user_id, scope_type, factory_code, workshop_code)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

另加 `admin_users.is_active TINYINT(1) NOT NULL DEFAULT 1`：权限、scope、is_active **每请求从 DB 加载，不进 JWT**——进 JWT 会滞留到 token 过期才生效；每请求加载让撤角色/停账号立即生效，也部分弥补了 JWT 无吊销的限制（停用账号即封禁）。将来若需要运行时自定义角色包，再把映射提升为表，迁移是纯追加。

员工组织归属要拆清楚：

- `employee_no`、`factory_code`、`workshop_code`：**入职快照**，服务工号稳定性和历史追溯。
- `current_factory_code`、`current_workshop_code`：**当前管理归属**，服务权限过滤、管理列表和转厂/转车间后的日常操作。
- 若需要完整调动历史，再追加 `employee_org_assignments(employee_id, factory_code, workshop_code, effective_from, effective_to, reason)`；列表和权限仍优先看当前归属字段。

#### 4.6.3 权限 Key 与角色包

基础权限粒度：

| 权限 key | 说明 |
|---|---|
| `employee:read` | 查看员工列表/详情（身份证仍脱敏，薪酬按权限裁剪） |
| `employee:create` | 新增员工；目标当前归属必须在操作者 scope 内 |
| `employee:update` | 编辑非敏感基础信息 |
| `employee:resign` | 离职/复职 |
| `id_card:reveal` | 查看完整身份证号，必须写审计 |
| `id_card:update` | 修改身份证号，必须写审计 |
| `salary:read` | 查看薪酬字段 |
| `salary:write` | 修改薪酬字段 |
| `employee:export` | 批量导出，导出字段仍受敏感权限限制 |
| `org:manage` | 管理分厂/车间/组织归属 |
| `admin:manage` | 管理账号、角色、权限、scope |

角色包为代码常量（`Permissions.h`），组合需求靠多角色叠加（权限取并集），scope 始终按用户单独配置：

| 角色 | 功能权限 | 典型 Scope |
|---|---|---|
| `super_admin` | 全部权限 | 全公司 |
| `hr_manager` | `employee:read/create/update/resign/transfer` + `id_card:reveal/update` + `salary:read/write` | 全公司或指定分厂 |
| `factory_manager` | `employee:read/update/resign` | 指定分厂 |
| `workshop_manager` | `employee:read/update` | 指定车间 |
| `finance` | `employee:read` + `salary:read/write` | 全公司或指定分厂 |
| `viewer` | `employee:read` | 可配置 scope |

- 新增员工必须同时有 `employee:create` 和 `salary:write`（建档必填薪酬，CHECK 约束决定的）。
- 无 `salary:write` 的编辑请求不得改动 `pay_type` 或薪酬金额（改了返回 403），无 `salary:read` 时列表/详情薪酬字段返回 null。
- `employee:transfer`：转厂/转车间（改 `current_*`，工号与入职快照不动），源员工与目标组织都必须在操作者 scope 内，写审计。

#### 4.6.4 后端授权流程

所有 `/api/admin/*` 的 v2+ 流程统一为：

1. `requireAdmin` 校验 JWT，只解决“是谁”。
2. 加载当前用户的 `permissions` 和 `scopes`。
3. 先校验功能权限；没有权限返回 403。
4. 再构造组织 scope 过滤条件；列表、详情、更新、离职、reveal 都必须套用同一套过滤。
5. 对身份证、薪酬、导出等敏感动作再校验专门权限。
6. 敏感动作写 `audit_logs`，审计里记录操作者、员工、动作、脱敏值、时间和来源。

Scope 过滤必须在 SQL/服务层执行，不能只靠前端筛选：

```sql
-- 示例：分厂 scope
WHERE employees.current_factory_code IN (:allowed_factory_codes)

-- 示例：车间 scope
WHERE (employees.current_factory_code, employees.current_workshop_code)
      IN ((:factory_a, :workshop_a), (:factory_b, :workshop_b))
```

接口语义：

- 列表接口只返回 scope 内数据。
- 详情/编辑/reveal 访问 scope 外员工时，优先返回 404，避免泄露“这个员工存在但你不能看”。
- 有 scope 但缺功能权限时返回 403。
- `GET /api/admin/org` v2+ 应只返回操作者 scope 内可选组织；`super_admin` 才返回全量。
- 新增员工时，目标 `current_factory_code/current_workshop_code` 必须在操作者 scope 内。
- 转厂/转车间是组织归属变更，必须需要 `org:manage` 或专门的 `employee:transfer` 权限，并写审计/历史。

#### 4.6.5 前端职责

- 前端根据权限隐藏按钮和菜单，减少误操作，但不承担安全边界。
- 员工列表中薪酬字段按 `salary:read` 控制展示；无权限时显示空或占位，不从接口返回真实值。
- 身份证完整号、导出、薪酬修改、离职/复职等操作都要有独立按钮状态和后端错误提示。
- 当前用户的角色、权限、scope 可由 `/api/admin/me` 返回；前端只用于展示和交互控制。

#### 4.6.6 验证边界

v2+ 做权限时必须补以下验证：

- 分厂 A 管理者看不到分厂 B 员工列表。
- 分厂 A 管理者手写 B 员工 id 请求详情/编辑/reveal，返回 404 或 403，不返回数据。
- 有 `employee:read` 但无 `salary:read` 时，列表/详情不返回薪酬真实值。
- 有 `employee:read` 但无 `id_card:reveal` 时，完整身份证 reveal 返回 403 且不写成功审计。
- 有 `employee:create` 但目标车间不在 scope 内时，新建失败。
- 员工转厂后，旧工号不变，但列表与权限过滤按 `current_factory_code/current_workshop_code` 生效。
- `super_admin` 能跨 scope 管理，但敏感动作仍写审计。

## 5. 后端 API 设计

统一前缀 `/api`，响应体沿用 PetManager 风格：`{ "success": bool, "message": str, "data": ... }`。

| 方法 | 路径 | 说明 |
|---|---|---|
| POST | `/api/auth/login` | 管理员登录，返回 JWT |
| GET | `/api/admin/org` | 分厂+车间清单（新增员工表单下拉用） |
| GET | `/api/admin/employees` | 列表：`?page=&page_size=&pay_type=&status=&keyword=`（keyword 匹配工号/姓名/手机号）；身份证脱敏 |
| GET | `/api/admin/employees/:id` | 详情；身份证**同样脱敏** |
| GET | `/api/admin/employees/:id/id-card` | 返回完整身份证号 + 写 `view_id_card` 审计日志 |
| POST | `/api/admin/employees` | 新增（须带 `factory_code`/`workshop_code`，后端生成工号；按 pay_type 校验薪酬字段互斥必填） |
| PUT | `/api/admin/employees/:id` | 编辑；`id_card_no` 可选，传了才更新并写审计日志 |
| POST | `/api/admin/employees/:id/resign` | 离职（软删除），须带 `resigned_at` |
| POST | `/api/admin/employees/:id/reinstate` | 复职，清空 `resigned_at` |
| POST | `/api/admin/employees/:id/transfer` | 转厂/转车间：`{factory_code, workshop_code}` 改当前归属（快照与工号不动），需 `employee:transfer`，写审计 |
| GET | `/api/admin/me` | 当前账号的 username/roles/permissions/scopes（前端据此控制交互展示） |

- 所有 `/api/admin/*` 在 `backend/routes/Routes.cpp` 中先走 `requireAdmin` 校验 JWT。
- 分页默认 `page_size=20`，上限 100。
- 校验失败返回 400 + 逐字段错误信息（前端能对应到具体输入框，报错给下一步）。

## 6. 前端设计

- **布局**：登录页 → 主布局（侧边栏：员工管理；顶部：当前用户/退出）→ 员工列表页。
- **员工列表页**（核心页面）：
  - 筛选区：薪酬类型 Tab（全部/月薪工/时薪工）+ 在职状态下拉 + 关键字搜索（工号前缀/姓名/手机号）。
  - 表格列：工号、姓名、性别、薪酬类型（徽标色区分）、薪酬（月薪 ¥X/月，时薪 ¥X/时）、分厂/车间、手机号、部门、岗位、入职日期、状态、操作（编辑/离职/复职）。
  - 身份证号不进表格；详情抽屉里默认脱敏，点「查看完整号」按钮才调 reveal 端点。
- **新增/编辑弹窗**：分厂→车间级联下拉（数据来自 `/api/admin/org`，仅新增时可选，编辑时锁定）；**部门为预设下拉、岗位随部门级联**（预设清单在 `frontend/src/shared/orgPresets.ts`，YANG 可直接改该文件；后端契约不变仍存文本，编辑时存量非预设值自动注入选项）；选薪酬类型后**动态切换**薪酬输入框；编辑时身份证显示脱敏占位，重新输入才提交新值；前端做与后端相同的字段校验。
- **UX 原则**：月薪/时薪的区分靠"筛选 Tab + 徽标 + 动态表单"，不做两个独立页面——同一批人管理，拆页面逼 HR 来回切换。

## 7. 配置与脚本

`.env.example`（真实值不进 git、不进文档）：

```bash
# Server
SERVER_PORT=8090                  # 避开 PetManager 端口

# Database
DB_HOST=localhost
DB_PORT=33060
DB_USER=root
DB_PASS=<填写>
DB_NAME=factory_erp
DB_SSL_MODE=DISABLED
DB_AUTO_RUN_MIGRATIONS=true       # 本地默认 true；生产须显式开启并先备份

# JWT
JWT_SECRET=<openssl rand -hex 64>
JWT_EXPIRE_HOURS=12

# 身份证加密（两把密钥分开；备份到密码管理器，丢失即不可解密）
ID_CARD_ENC_KEY=<openssl rand -hex 32>
ID_CARD_HMAC_KEY=<openssl rand -hex 32>

# 首个管理员（仅在 admin_users 为空时由启动流程 seed，seed 后可删）
ADMIN_INIT_USERNAME=admin
ADMIN_INIT_PASSWORD=<一次性设置>
```

- `bin/build.sh` / `bin/start.sh`：照搬 PetManager 骨架（cmake `-DBUILD_TESTING=OFF`、加载 .env）。
- 密码哈希：PBKDF2-HMAC-SHA256，复用 PetManager `controllers/auth/encrypt/encrypt.cpp` 的实现思路（`PKCS5_PBKDF2_HMAC`，`$scheme$iterations$salt$hash` 格式，未来可平滑迁移 argon2）。不硬编码默认密码。

## 8. 验证方式（交付前自验路径）

1. `bin/build.sh` 后端编译通过。
2. `bin/start.sh` 启动，migrations 自动建表；若 `admin_users` 为空且 `.env` 配了 `ADMIN_INIT_USERNAME/PASSWORD`，启动流程会创建首个管理员。
3. `curl` 走通：登录拿 token → 新增月薪工 + 时薪工 → 列表按 pay_type 筛选 → 详情（脱敏）→ reveal 完整号（查 audit_logs 落了一条）→ 编辑身份证（audit_logs 再落一条）→ 离职 → 复职。
4. `cd frontend && npm run build` 通过；`npm run dev` 起页面，浏览器实际点一遍全流程（含表单逐字段报错）。
5. 边界用例：身份证校验位错误 / 重复身份证（撞 id_card_hash）/ 月薪工传时薪字段 / 非法分厂车间码 / 未带 token 访问 admin 接口 → 均应 4xx；并发创建员工不撞工号（同前缀并发 10 个 POST 验证）。
6. 工号规则：同车间同年序号连续递增；不同前缀互不影响；`keyword=AB26` 能按前缀筛出该车间当年员工。

## 9. 里程碑

1. **M1 骨架**：目录 + CMake + .env + build/start 脚本 + DB 连接 + migrations 跑通。
2. **M2 后端**：auth + 加密/HMAC 工具 + employees 全部接口 + curl 自验（含并发取号）。
3. **M3 前端**：登录 + 列表 + 表单 + 详情抽屉 + 浏览器全流程自验。
4. **M4 收尾**：`.agents/context/` 项目文档、README、最终验证报告。

## 10. 评审记录（Claude ⇄ Codex，2026-07-04）

| 决策点 | 初稿 | 最终结论 | 变更原因 |
|---|---|---|---|
| 前端脚手架 | 倾向 Vite+Pinia，待定 | **Vite + Pinia**（一致同意） | Vue CLI 停维护，新项目不背包袱 |
| 身份证存储 | 明文 + 接口脱敏 | **AES-256-GCM 加密 + HMAC-SHA256 唯一哈希** | Codex：v1 不需按身份证搜索，加密无功能损失，日后迁移明文更贵；Claude 补充 HMAC 防枚举 |
| 详情接口 | 返回完整身份证号 | **默认脱敏，显式 reveal 端点 + 审计** | Codex：降低完整号暴露面 |
| 身份证编辑/审计 | 待定 | **允许编辑；最小 audit_logs 仅覆盖 view/update 身份证两类动作**，日志只存脱敏值 | 覆盖最高敏感操作，控制 v1 范围 |
| ENUM vs 字典表 | 倾向 ENUM | **ENUM，不做字典表**，附共享取值清单 | 取值封闭，字典表过重 |
| 密码哈希 | 倾向 PBKDF2 | **PBKDF2-HMAC-SHA256**，复用 PetManager encrypt.cpp 模式（已核实存在） | 无新依赖，格式可迁移 |
| 工号 | FE + 5 位 | ~~FE + 6 位~~ → **被第二轮推翻，见下** | Codex：MAX+1 并发撞号是初稿遗漏 |
| 离职一致性 | 未约束 | **chk_resigned CHECK 约束** | Codex 指出遗漏 |
| 迁移开关 | 未提 | **DB_AUTO_RUN_MIGRATIONS**（本地 true，生产显式） | Codex 指出遗漏 |
| 密钥轮换 | 未提 | **v1 不做**，文档注明限制与备份责任 | 双方同意，范围可控 |

### 第二轮（2026-07-04，YANG 追加需求后）

| 决策点 | 结论 | 说明 |
|---|---|---|
| 工号格式 | **`{分厂 1 字母}{车间 1 字母}{YY}{4 位序号}`**（如 `AB260007`），编码入职快照，调岗/转厂/转薪酬类型不改工号 | YANG 要求编码分厂/车间/入职年份以支持前缀搜索；"不编码可变属性"原则改为"编码但永不变更" |
| 前缀解析 | **按定长位置解析（右起 4 位为序号）**；"找第一个 0 向前"仅作 UI 近似 | 双方一致：该启发式 2030 年代起失效（年份"30"含 0），不可作后端规则 |
| 取号 | 序列表按前缀（如 `AB26`）隔离，事务 FOR UPDATE；每车间每年上限 9999，溢出报错 | Codex 确认 |
| 组织数据 | 新增 `factories`/`workshops` 种子表 + `GET /api/admin/org`，不做管理 UI | 支撑工号编码与表单下拉，范围最小化 |
| role 字段 | v1 固定 `'admin'`；中间件不做 role 分支，前端不做角色路由 | 第四轮已调整：v2+ 不继续扩展字符串 role，改走 roles/permissions/scopes |
| 实现分工 | Claude：骨架+后端+文档+前端脚手架+集成验证；Codex：`frontend/src/**` 业务代码+前端自验 | 详见 `docs/WORKPLAN.md`；契约变更须经 Claude 裁决改文档，Codex 不私自改 |

### 第三轮（2026-07-04，YANG 追问 ERP 权限边界）

| 决策点 | 结论 | 说明 |
|---|---|---|
| 权限模型 | v2+ 采用 **RBAC + Scope**，RBAC 管功能权限，Scope 管组织数据范围 | ERP 的真实边界不是“有没有菜单”，而是同一功能下能访问哪些分厂/车间数据 |
| 分厂隔离 | 必须在后端 SQL/服务层强制过滤，前端隐藏菜单不作为安全边界 | 防止分厂管理者通过手写请求访问其他分厂员工、薪酬、身份证信息 |
| 工号与权限 | 工号/`factory_code`/`workshop_code` 是入职快照，不作为长期权限归属依据 | 员工转厂/转车间后，权限应看当前归属，未来需补 `current_factory_code`/`current_workshop_code` 或组织归属历史表 |
| 敏感权限 | 身份证 reveal、薪酬、导出、离职/删除等单独授权 | 降低高敏感数据被普通管理权限顺带放大的风险 |

### 第四轮（2026-07-04，YANG 要求重新设计权限边界）

| 决策点 | 结论 | 说明 |
|---|---|---|
| `admin_users.role` | 只保留 v1 兼容，不继续扩展 `'hr'`/`'viewer'` 字符串分支 | 避免角色组合爆炸，后期迁移到 `roles` / `permissions` / `admin_user_roles` / `admin_user_scopes` |
| 授权链路 | **认证 → 功能权限 → 组织 Scope → 敏感动作权限 → 审计** | `requireAdmin` 只回答“是谁”，不能直接等同于“能做什么/能看哪些数据” |
| 组织归属 | 新增当前归属概念：`current_factory_code` / `current_workshop_code`，必要时再加归属历史表 | 工号和入职分厂/车间保持历史快照；权限、管理列表、转厂后的操作走当前归属 |
| 后端过滤 | 列表、详情、编辑、离职、reveal、导出必须统一套 scope 过滤 | scope 外员工优先 404；有 scope 但缺功能权限返回 403 |
| 验证要求 | v2+ 必须补越权测试矩阵 | 覆盖跨分厂读取、手写 id、薪酬裁剪、身份证 reveal、新建目标组织、员工转厂等路径 |

### 第五轮（2026-07-04，YANG 决定实施 v2 权限，Claude 审查 §4.6 后优化）

| 决策点 | 原设计 | 定稿 | 变更原因 |
|---|---|---|---|
| 角色/权限存储 | `roles`/`permissions`/`role_permissions` 3 张表 | **角色→权限映射为代码常量**（`Permissions.h`），只建 `admin_user_roles` + `admin_user_scopes` 两张表 | 权限 key 是代码检查点，入库形成双真相源；无权限编辑 UI 时 DB 映射是死数据；自定义组合靠多角色叠加。将来要运行时配置再升级为表，纯追加迁移 |
| 权限加载 | 未明确 | **每请求从 DB 加载权限/scope/is_active，不进 JWT** | 进 JWT 滞留到过期才生效；每请求加载让撤权/停号立即生效 |
| 账号停用 | 未设计 | `admin_users.is_active`，`requireAccess` 每请求校验 | 低成本弥补 JWT 无吊销：停用即封禁 |
| 当前归属可变更 | 只有 `current_*` 列，无变更途径 | 补 `POST /:id/transfer` 端点（`employee:transfer` + 审计） | 没有变更途径的"当前归属"永远等于快照，区分是死重 |
| 建档薪酬权限 | 未明确 | `employee:create` 须同时具备 `salary:write`；无 `salary:write` 不得改 `pay_type`/金额 | 建档必填薪酬（CHECK 约束）；堵"无薪酬权限但能借编辑改薪酬"的洞 |
| 空 scope 语义 | 未明确 | 无 scope 行 = 看不到任何数据（安全默认，fail-closed） | 漏配 scope 应当少看而不是全看 |
| audit_logs 扩容 | 未明确 | `action` ENUM→VARCHAR，`operator_id`/`employee_id` 可空，加 `ip`/`detail` 列 | 支持 login_failed（无 operator）与 transfer（记录组织变化）等新动作 |
| admin:manage / 导出 | 在权限 key 清单中 | **本轮不实现**，账号/角色/scope 先由 SQL 维护，管理 API+UI 与导出留下阶段 | 控制单轮范围；权限强制是安全核心，管理界面是易用性 |

### 实现期追加（2026-07-04，YANG 提出）

| 需求 | 结论 |
|---|---|
| 部门改预设下拉 | 预设清单常量文件 `frontend/src/shared/orgPresets.ts`，后端契约不变（仍存文本） |
| 岗位随部门级联 | 岗位选项 = 所选部门的子数组；切换部门清空岗位；编辑时存量非预设值注入选项防丢值 |

## 11. v1 实现层安全审查（2026-07-04）

对 v1 已实现代码的鉴权与敏感数据路径做了一轮审查（设计层权限模型见 §4.6，本节只记实现现状）。

**已核实无问题**：所有 `/api/admin/*` 均过 `requireAdmin`；SQL 全参数化；JWT 签名常量时间比较、不信任 header alg、过期校验；密码 PBKDF2-HMAC-SHA256（10 万次迭代 + 随机盐 + 常量时间比较）；身份证 AES-256-GCM 每次随机 nonce、解密验 tag；reveal 先写审计再返回明文（审计失败不返回）；身份证变更与审计同事务；登录统一报错防账号枚举；启动时 fail-fast 校验密钥配置；前端完整身份证只存组件局部 ref、切换员工即清空，不进 store/localStorage；无任何代码用 `factory_code`/`workshop_code` 或工号前缀做权限过滤。

**本轮修复**：
- CORS 由 `*` 通配改为来源白名单（`CORS_ALLOWED_ORIGINS`，默认放行本地 dev 前端；非白名单来源不回 CORS 头）。审查中同时确认：OPTIONS 预检由 Crow 内置处理器接管、不经过中间件，即后端本就不支持浏览器跨域直连（dev/生产均为同源部署，vite 代理 `/api`，不受影响）；若将来跨域部署，CORS 含预检须在反向代理层处理。
- 登录增加防爆破限流：同一「用户名+IP」15 分钟内失败 5 次返回 429，成功登录清零；内存实现，单实例有效。

**已知限制（v1 接受，v2+ 处理）**：
- JWT 无服务端吊销：退出登录只清前端 token，已签发 token 在 12 小时内仍有效；泄露时的兜底是更换 `JWT_SECRET` 并重启（全员下线）。v2 引入多角色时须改短时 token + 刷新或服务端会话表。
- token 存 localStorage，XSS 可窃取——依赖前端不引入不可信第三方脚本（v1 无此注入面）。
- 审计只覆盖身份证 view/update 两类动作；登录成败、离职/复职、薪酬变更均不落审计。v2 权限矩阵落地时审计范围同步扩大（见 §4.6 授权链路第 6 步）。
- 登录限流为内存态：重启清零；部署在反向代理后 remote_ip 是代理地址，限流退化为按用户名。
- 无管理员改密接口：改密码须直接操作数据库。
- 传输安全：Crow 只出明文 HTTP，生产必须由反向代理终结 TLS；`DB_SSL_MODE=DISABLED` 仅限本机 MySQL，数据库上远程必须开启 SSL。
