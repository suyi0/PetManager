# FactoryERP 方案初稿（v1 — 员工信息管理）

> 状态：初稿，待与 Codex 评审讨论后出最终版（`docs/DESIGN.md`）。
> 借鉴项目：`~/Code/PetManager`（C++ Crow 后端 + Vue 前端的分层与配置方式）。

## 0. 结论先行

- 新建独立项目 `~/Code/FactoryERP`，**后端 C++17 + Crow + MySQL**、**前端 Vue 3 + TypeScript**，目录分层照搬 PetManager 的成熟结构（routes / controllers / middleware / database / migrations；前端 module 化：api / router / store / views）。
- v1 只做一件事做透：**管理员登录 + 员工信息列表管理（增删改查、筛选、分页）**，月薪工/时薪工用**单表 + `pay_type` 枚举**区分，不拆两张表。
- 配置沿用 PetManager 的 `.env` + `bin/build.sh` / `bin/start.sh` 模式，**只借结构不复制任何密钥值**。

## 1. 要解决的问题与边界

**本质问题**：工厂需要一个员工花名册系统，HR/管理员能维护员工基本信息，并且月薪工与时薪工的薪酬口径不同，需要在录入、展示、筛选上区分。

**v1 范围（做）**：
- 管理员账号登录（JWT）。
- 员工列表：分页、按姓名/工号/手机号搜索、按薪酬类型（月薪/时薪）与在职状态筛选。
- 员工新增 / 编辑 / 离职（软删除）/ 详情。
- 字段校验（身份证 18 位含校验位、手机号 11 位、邮箱格式）。

**v1 不做（明确排除，避免范围失控）**：
- 考勤、排班、计时、工资核算与发放（时薪工的工时记录属于此类，v2 再做）。
- 多角色权限体系（v1 只有一个 admin 角色，但代码结构预留角色字段）。
- 部门树、组织架构管理（v1 部门为普通文本/字典字段）。
- 短信/邮件通知（PetManager 有此能力，v1 不需要）。

## 2. 技术栈与取舍

| 层 | 选型 | 为什么 | 代价/风险 |
|---|---|---|---|
| 后端 | C++17 + Crow + MySQL Connector/C++ + OpenSSL | 与 PetManager 一致，YANG 能看懂、能直接复用构建脚本与中间件写法 | C++ 开发效率低于脚本语言；接受 |
| 数据库 | MySQL 8（本机已有实例，新建库 `factory_erp`） | 复用现有环境，零新增运维 | 与 PetManager 共用实例，误操作库要靠库名隔离 |
| 前端 | Vue 3 + TS + Vue Router + **Pinia** + **Vite** | Vue CLI 已停止维护，新项目没必要背历史包袱；Pinia 是 Vuex 的官方继任，写法更简单 | 与 PetManager（Vue CLI + Vuex）不完全一致，模式可借鉴但代码不能逐行照抄。**待与 Codex 讨论：是否为了一致性退回 Vue CLI + Vuex** |
| UI 组件 | Element Plus | 后台管理型系统，表格/表单/弹窗开箱即用，避免手写组件拖慢 v1 | 引入约 800KB 依赖；对内部系统可接受 |
| 鉴权 | JWT（最终落地为 `backend/middleware/Auth.*` + `requireAdmin`，借鉴 PetManager 中间件思路） | 已验证的模式 | — |
| 缓存/消息 | **不引入 Redis** | v1 单实例、无跨实例广播需求，第一性原理：没有这个问题就不引入这个解 | 将来多实例部署需补 |

## 3. 目录结构

比 PetManager 少一层嵌套（PetManager 的 `pethospital/` 包裹层是历史产物，新项目不需要）：

```
FactoryERP/
├── CLAUDE.md / AGENTS.md          # 双入口 → .agents/context/*（沿用 YANG 的双 agent 约定）
├── .agents/context/               # PROJECT / ARCHITECTURE / COMMANDS / BACKEND / FRONTEND
├── .env.example                   # 模板（真实 .env 不进 git）
├── bin/
│   ├── build.sh                   # cmake 配置 + 编译（借鉴 PetManager，-DBUILD_TESTING=OFF）
│   └── start.sh                   # 加载 .env → build → 启动后端
├── docs/
│   ├── DESIGN-v1-draft.md         # 本文件
│   └── DESIGN.md                  # 评审后的最终方案
├── backend/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── routes/
│   │   └── Routes.{h,cpp}         # 注册 /api/auth/* 与 /api/admin/*
│   ├── controllers/
│   │   ├── auth/                  # 登录、JWT 签发
│   │   └── admin/                 # EmployeeHandlers（列表/详情/增改/离职）
│   ├── middleware/
│   │   ├── Auth.{h,cpp}           # JWT 校验（借鉴 PetManager）
│   │   └── Cors.h
│   ├── database/
│   │   ├── Db.{h,cpp}                 # X DevAPI session + 启动迁移
│   │   └── Migrations.{h,cpp}         # 建表 SQL，启动时按序执行
│   └── utils/                     # 校验（身份证/手机号）、JWT、密码哈希
└── frontend/
    ├── vite.config.ts
    └── src/
        ├── main.ts / App.vue      # bootstrap 与根组件
        ├── app/                   # router 与全局布局
        ├── core/auth/             # 登录页、token 管理、路由守卫
        ├── modules/hr/            # 员工管理模块：api / store / views / router
        │   └── views/
        │       ├── EmployeeListPage.vue
        │       └── components/EmployeeFormDialog.vue
        └── shared/                # axios 封装、通用组件、样式
```

## 4. 数据库设计

### 4.1 月薪工 vs 时薪工：单表 + 枚举（核心决策）

**决策**：一张 `employees` 表，`pay_type ENUM('monthly','hourly')` 区分，薪酬字段按类型二选一填写。

**为什么不拆 `monthly_employees` / `hourly_employees` 两张表**：两类员工 90% 的字段完全相同（工号/姓名/证件/联系方式/婚姻状况…），差异只有 1-2 个薪酬字段。拆表会让"全员列表"这个最高频操作变成 UNION 查询，增删改要写两套，得不偿失。

**为什么不做 EAV 或子表继承**：v1 差异字段极少，过度设计。若 v2 时薪工要挂工时记录，再加 `work_time_logs` 子表即可，不影响本表。

**代价**：`monthly_salary` 与 `hourly_rate` 互斥字段并存于一表，靠应用层 + CHECK 约束保证一致性（MySQL 8 支持 CHECK）。

### 4.2 表结构

```sql
-- 管理员账号
CREATE TABLE admin_users (
    id            BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    username      VARCHAR(64)  NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,          -- 加盐哈希，绝不存明文
    role          VARCHAR(32)  NOT NULL DEFAULT 'admin',  -- 预留多角色
    created_at    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 员工
CREATE TABLE employees (
    id             BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    employee_no    VARCHAR(16)  NOT NULL UNIQUE,   -- 工号，规则见 4.3
    name           VARCHAR(64)  NOT NULL,
    gender         ENUM('male','female') NOT NULL,
    id_card_no     CHAR(18)     NOT NULL UNIQUE,   -- 身份证号，18 位含校验位
    phone          VARCHAR(16)  NOT NULL,
    email          VARCHAR(128) NULL,
    marital_status ENUM('single','married','divorced','widowed') NOT NULL DEFAULT 'single',
    pay_type       ENUM('monthly','hourly') NOT NULL,
    monthly_salary_cents BIGINT UNSIGNED NULL,     -- 月薪（分），pay_type=monthly 时必填
    hourly_rate_cents    BIGINT UNSIGNED NULL,     -- 时薪（分），pay_type=hourly 时必填
    department     VARCHAR(64)  NULL,
    position       VARCHAR(64)  NULL,
    hire_date      DATE         NOT NULL,
    status         ENUM('active','resigned') NOT NULL DEFAULT 'active',  -- 软删除=置 resigned
    resigned_at    DATE         NULL,
    created_at     TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at     TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    CONSTRAINT chk_pay_fields CHECK (
        (pay_type = 'monthly' AND monthly_salary_cents IS NOT NULL AND hourly_rate_cents IS NULL) OR
        (pay_type = 'hourly'  AND hourly_rate_cents    IS NOT NULL AND monthly_salary_cents IS NULL)
    ),
    INDEX idx_pay_type_status (pay_type, status),
    INDEX idx_name (name),
    INDEX idx_phone (phone)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

说明：
- **金额用整数分**存储，避免浮点误差（工资场景必须）。
- **性别/婚姻/薪酬类型用 ENUM**：取值集合封闭且极少变化；前端映射中文展示。
- **"等等"字段的扩展策略**：入职日期、部门、岗位 v1 已含；后续加字段走 `backend/database/Migrations.cpp` 追加迁移 id，不改历史迁移 SQL。

### 4.3 工号规则

`FE` + 5 位自增序号（如 `FE00001`），**不在工号里编码薪酬类型**。理由：员工可能月薪转时薪（转正/转岗），工号是终身标识，不应携带可变属性。工号在创建时由后端生成，前端不可编辑。

### 4.4 身份证号的敏感性处理（待 Codex 讨论）

- 列表接口返回**脱敏**值（`3301**********1234`），仅详情接口返回完整号。
- v1 数据库明文存储（内部系统 + 本机部署），**是否上加密（AES-GCM，密钥进 .env）留给评审讨论**——加密的代价是模糊搜索/唯一约束实现变复杂。

## 5. 后端 API 设计

统一前缀 `/api`，响应体沿用 PetManager 风格：`{ "success": bool, "message": str, "data": ... }`。

| 方法 | 路径 | 说明 |
|---|---|---|
| POST | `/api/auth/login` | 管理员登录，返回 JWT |
| GET | `/api/admin/employees` | 列表：`?page=&page_size=&pay_type=&status=&keyword=`（keyword 匹配 工号/姓名/手机号） |
| GET | `/api/admin/employees/:id` | 详情（完整身份证号） |
| POST | `/api/admin/employees` | 新增（后端生成工号） |
| PUT | `/api/admin/employees/:id` | 编辑（工号/身份证号不可改？→ 身份证号允许改但记日志，待讨论） |
| POST | `/api/admin/employees/:id/resign` | 离职（软删除），带 `resigned_at` |
| POST | `/api/admin/employees/:id/reinstate` | 复职（误操作恢复） |

- 所有 `/api/admin/*` 走 `backend/middleware/Auth.*` 的 `requireAdmin` 校验 JWT。
- 列表分页默认 `page_size=20`，上限 100。
- 校验失败返回 400 + 具体字段错误信息（给前端逐字段提示，报错给下一步而不是只说失败）。

## 6. 前端设计

- **布局**：登录页 → 主布局（侧边栏：员工管理；顶部：当前用户/退出）→ 员工列表页。
- **员工列表页**（核心页面）：
  - 顶部筛选区：薪酬类型 Tab（全部/月薪工/时薪工）+ 在职状态下拉 + 关键字搜索框。
  - 表格列：工号、姓名、性别、薪酬类型（月薪/时薪 徽标色区分）、薪酬（月薪显示 ¥X/月，时薪显示 ¥X/时）、手机号、部门、岗位、入职日期、状态、操作（编辑/离职）。
  - 身份证号列表不展示，点开详情抽屉查看。
- **新增/编辑弹窗**：选择薪酬类型后**动态切换**薪酬输入框（月薪 or 时薪），前端做与后端相同的字段校验（身份证校验位、手机号、邮箱）。
- **薪酬类型区分的 UX 原则**：区分靠"筛选 + 徽标 + 动态表单"，而不是做两个独立页面——两类员工是同一批人管理，拆页面会让 HR 来回切换。

## 7. 配置与脚本（借鉴 PetManager，不复制密钥）

`.env.example`：

```bash
# Server
SERVER_PORT=8090            # 避开 PetManager 端口

# Database
DB_HOST=localhost
DB_PORT=33060
DB_USER=root
DB_PASS=<填写>
DB_NAME=factory_erp
DB_SSL_MODE=DISABLED

# JWT
JWT_SECRET=<openssl rand -hex 64 生成>
JWT_EXPIRE_HOURS=12
```

- `bin/build.sh` / `bin/start.sh`：照搬 PetManager 脚本骨架（cmake -DBUILD_TESTING=OFF、加载 .env）。
- CMakeLists 借鉴 PetManager 的 Homebrew 路径探测与依赖查找，但删掉本项目用不到的部分（curl/短信/Python 桥接）。
- 首个管理员账号：启动时读取 `ADMIN_INIT_USERNAME/PASSWORD`，仅在 `admin_users` 为空时一次性创建，不硬编码默认密码。

## 8. 验证方式（交付前自验路径）

1. `bin/build.sh` 后端编译通过。
2. `bin/start.sh` 启动，migrations 自动建表。
3. `curl` 走通：登录拿 token → 新增月薪工 + 时薪工 → 列表按 pay_type 筛选 → 编辑 → 离职 → 复职。
4. `cd frontend && npm run build` 通过；`npm run dev` 起页面，浏览器实际点一遍上述流程（含表单校验报错提示）。
5. 边界用例：身份证校验位错误 / 重复工号(不可能,自动生成) / 重复身份证 / 月薪工传时薪字段 → 均应 400。

## 9. 留给评审的决策点（与 Codex 讨论）

1. **前端脚手架**：Vite + Pinia（推荐，新项目不背 Vue CLI 包袱） vs 与 PetManager 完全一致的 Vue CLI + Vuex（YANG 已熟悉）。
2. **身份证号存储**：明文 + 接口脱敏（推荐，v1 简单） vs 落库加密（更安全但搜索/约束复杂）。
3. **身份证号是否允许编辑**：录错需要改是现实需求，倾向允许编辑；是否需要操作日志表 `audit_logs` 进 v1？
4. **ENUM vs 字典表**：gender/marital_status/pay_type 用 ENUM（推荐）还是 lookup 表（更灵活但 v1 过重）？
5. **密码哈希方案**：OpenSSL 可用的 PBKDF2-HMAC-SHA256（推荐，无新依赖） vs 引入 bcrypt/argon2 库。
6. **工号规则**：`FE + 5 位序号` 是否满足工厂习惯（有的工厂要按车间/年份编码）。

## 10. 里程碑

1. **M1 骨架**：目录 + CMake + .env + build/start 脚本 + DB 连接 + migrations 跑通。
2. **M2 后端**：auth + employees 全部接口 + curl 自验。
3. **M3 前端**：登录 + 列表 + 表单 + 浏览器全流程自验。
4. **M4 收尾**：`.agents/context/` 项目文档、README、最终验证报告。
