# PetManager 动态角色/权限（RBAC）设计方案（定稿 v3）

> 状态：**设计定稿 v3**，产品决策 A/B/C/D 均已定；Codex 两轮评审已全部并入（派职位授权闸门、WS 降权、DB 层元权限约束、rebuild 仅限 dev 库、P1+P3 同发、静态硬门禁）。**决策 D=是**：部门是真实数据边界 → 纳入 org_scope（§16，三级 全部分院/分院/部门，分院层按 YANG 要求预留最高管理层）。可据此进入实现（分阶段见 §12）。
> 作者 Claude（Lead），评审 Codex（Assist）。目标读者：YANG。
> 借鉴：`~/Code/FactoryERP` 已验证的 RBAC（固定权限包 + 每请求 Access 解析 + 元权限不可委派）。

## 0. 结论先行

- PetManager 已经有一半底座：`Permissions.h` 是固定权限 key 目录，`roleHasPermission` 已是"权限包"概念——**只是"角色→权限包"映射硬编码在 C++，超管改不了**。本方案核心：把这个映射从代码搬进数据库，key 目录留在代码。
- **权限 key 永远是代码常量**（`Permissions.h`），超管只能"勾选组合已有 key"，不能凭空发明 key。
- 组织结构两级：**部门（department）→ 职位（position）**。职位是承载权限的主体。
- **建部门/职位放给普通管理员，授权归超管**（决策 B）：空职位无权=fail-closed 无害，故建职位可开放；真正的权力闸门在"给职位授权"**以及"把人派进含权限职位"**（Codex 修正：派人进高权职位=变相授权），这两步都超管独占（§8）。
- **门户权限可授予，元权限不可委派**：`portal:*`（含 `portal:super-admin`）是可授予门户权限；**唯一不可委派的是元权限 `rbac:manage`**（管角色/权限本身）。超管身份靠受保护系统职位 `system_key='super-admin'` 承载。
- **超管 = 等价包，不做 root**（决策 A）：超管职位持有与现状等价的权限包 + `rbac:manage`，保 P1 等价性。
- **业务身份两层拆分**（决策 C = C4）：账户级 `users.account_type`（customer/staff）答"客户还是员工"；职位级 `positions.staff_kind` 答"工种（医生/仓管…）"。二者都不再靠可变的角色名。
- **一次性全迁移**：所有按角色名判权/判业务身份的入口必须一起改（清单见 §6）。**做一半留下的名字/权限双轨就是安全漏洞**。
- 数据库不走增量迁移：删 `types` 表整体 rebuild，新表进 `DatabaseMigrations.cpp` 建表清单，seed 重放现有行为。

## 1. 目标与范围

**要解决**：超级管理员在界面上创建/维护职位并授予权限，不必改 C++ 重编发版。新职位默认零权限，须超管发布。

**做**：
- 组织目录：部门 + 职位两级 lookup + CRUD（建：普通管理员；授权：超管）。
- RBAC：职位→权限映射入库；固定权限模版（seed）；超管授权/收权界面。
- 全部鉴权判定从"按角色名"迁到"按权限 key"；业务身份改按 `account_type`/`staff_kind`。
- 审计：角色/权限/授权变更全程留痕。
- 操作日志 `system_role`/`user_role` 由 ENUM 改 VARCHAR 快照。

**不做（本期边界）**：
- 一人多职位（本期一人一职位；多职位留 v2）。
- 超管升级为 root（隐式全权）——如未来要做，作为单独产品变更单独验证。

- 密钥/登录等无关改造。

> **决策 D=是（已定）**：部门是真实数据边界，本方案**纳入 org_scope**（§16，三级：全部分院/分院/部门，分院层预留）。"部门经理只看本部门数据"要做；"最高管理层跨全部分院"预留。这不是"不做"项，是本期新增工作。

## 2. 现状（证据）

| 现有资产 | 位置 | 评价 |
|---|---|---|
| 固定权限 key 目录（portal:* / salary:* / logs:read / stock:* / scope:* 等 ~25 个） | `utils/permissions/Permissions.h` | ✅ 直接复用 |
| 角色→权限包映射 | `utils/permissions/Permissions.cpp` `roleHasPermission` | ⚠️ 硬编码 `isBossPackage/...`，超管改不了 → 搬进库 |
| 每请求查库拿角色名再判权 | `jwtUtils.cpp` `isUserAuthorizedForPermission` → `getUserAuthTargetById` | ✅ 改角色立即生效，无 JWT 陈旧授权，保留 |
| 角色目录 | `types` 表（id + type 名） | ⚠️ 名字散、无层级；`type_id`/`type_name`/`JOIN types`/`getRoleId` 引用遍布 backend model/JWT/中间件/外键迁移/Redis cache/前端 storage/守卫/mock（远不止早期估的"15 处"）→ rebuild 替换 + 硬门禁核清（§10） |
| 按名字分组判断 | `roleTypeUtils` ~10 个 `isBossRole/...` | ❌ 名字硬编码 → 退休，改按权限 |
| 门户校验 | authMiddleware `isValidBossPortalToken` 等 10 个 | ⚠️ 按名字判门户 → 改按 `portal:*` |
| 业务身份判断 | `getRoleId("普通用户"/"医生")`（财务/定时任务/预约） | ❌ 把名字当业务身份 → 改按 `account_type`/`staff_kind`（§16） |
| 前端角色路由/布局 | 26+ 个 `.vue`/`.ts` 硬编码角色名 | ❌ 改按权限（`/me` 下发） |

**关键结论**：现状对未知角色 **fail-closed**（`roleHasPermission("车间主任", x)` → false）。危险不在"创建角色"，在"给角色授权 + 把全套鉴权/业务身份从按名字改成按权限/按标记"。

## 3. 核心语义（职位=权限主体）

职位是**唯一**权限承载主体；权限单独一张表挂载；治理敏感动作分级：
- **部门**：纯分组、无权限含义 → 普通管理员自由建。
- **职位**：可被派人的主体；建空职位、派人进**零权限职位**普通管理员可做（决策 B）；但**授权/收权、以及把人派进含权限职位**超管独占（§8）——空职位无权=无害，一旦职位含权，派人=变相授权，必须走 `rbac:manage`。
- 放弃的替代：部门/职位与安全角色拆两套（FactoryERP 做法）。理由：PetManager 一人一职位，拆两套每人维护两样更绕。

## 4. 数据模型

```sql
-- 部门（组织一级，纯分组，不直接挂权限；普通管理员可 CRUD）
CREATE TABLE departments (
    id         INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
    name       VARCHAR(64) NOT NULL UNIQUE,
    system_key VARCHAR(32) NULL UNIQUE COMMENT '内置部门稳定标识(如 medical)，业务身份锚点可用',
    sort_order INT NOT NULL DEFAULT 0,
    is_system  TINYINT NOT NULL DEFAULT 0 COMMENT '系统内置部门，不可删',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 职位（组织二级 = 权限承载主体 + 工种标记）
CREATE TABLE positions (
    id            INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
    department_id INT NOT NULL,
    name          VARCHAR(64) NOT NULL,
    -- 工种（决策 C4）：业务代码只认这个标记，不认可变的职位名字
    staff_kind    ENUM('doctor','nurse','warehouse','finance','management','personnel','general_staff')
                  NOT NULL DEFAULT 'general_staff',
    system_key    VARCHAR(32) NULL UNIQUE COMMENT '内置职位稳定标识(如 super-admin)，非空即受保护，不可删/降权/改 key',
    status        ENUM('draft','published') NOT NULL DEFAULT 'published' COMMENT '本期不启用 draft，字段备用',
    created_at    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_position_dept FOREIGN KEY (department_id) REFERENCES departments(id),
    UNIQUE KEY uq_position (department_id, name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 职位→权限（超级管理员独占写入；permission_key 必须 ∈ Permissions.h 代码目录，且 ≠ rbac:manage）
CREATE TABLE position_permissions (
    position_id    INT NOT NULL,
    permission_key VARCHAR(64) NOT NULL,
    granted_by     INT NULL COMMENT '授予人 user_id',
    granted_at     TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (position_id, permission_key),
    CONSTRAINT fk_pp_position FOREIGN KEY (position_id) REFERENCES positions(id) ON DELETE CASCADE,
    -- 纵深防御(Codex)：DB 层兜底元权限不可委派，防 seed/脚本/后台 bug 破坏不变量
    CONSTRAINT chk_pp_not_meta CHECK (permission_key <> 'rbac:manage')
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 权限模版（开发期预设"固定权限包"，超管一键套用到职位；纯 seed）
CREATE TABLE permission_templates (
    id   INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(64) NOT NULL UNIQUE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE permission_template_items (
    template_id    INT NOT NULL,
    permission_key VARCHAR(64) NOT NULL,
    PRIMARY KEY (template_id, permission_key),
    CONSTRAINT fk_pti_template FOREIGN KEY (template_id) REFERENCES permission_templates(id) ON DELETE CASCADE,
    CONSTRAINT chk_pti_not_meta CHECK (permission_key <> 'rbac:manage')
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- users 改造（决策 C4）：type_id → position_id + account_type
--   account_type='customer'（宠物主人，不在编）→ position_id 必须 NULL
--   account_type='staff'   （员工）           → position_id 必须非空
-- rebuild 时替换 type_id；全部 type_id/JOIN types 引用同步（范围见 §10 硬门禁）。
ALTER TABLE users
    ADD COLUMN account_type ENUM('customer','staff') NOT NULL DEFAULT 'customer',
    ADD COLUMN position_id  INT NULL,
    ADD CONSTRAINT fk_users_position FOREIGN KEY (position_id) REFERENCES positions(id),
    ADD CONSTRAINT chk_account_position CHECK (
        (account_type = 'customer' AND position_id IS NULL) OR
        (account_type = 'staff'    AND position_id IS NOT NULL)
    );
-- 说明：rebuild 直接建新 users 即可，上面 ALTER 仅示意字段与约束语义。
```

**为什么 C4 的两层是对的**：现状把"客户（宠物主人）"和"员工角色"塞进同一张 `types` 表，本身是设计的味道——客户根本不是"有职位的人"。C4 把"客户 vs 员工"提为**账户级一等事实**（`account_type`），把"工种"留在职位级（`staff_kind`）。财务/工资判据变成 `account_type='staff'`，干净且不受角色改名影响；医生判据变成 `staff_kind='doctor'`。CHECK 约束保证"客户无职位、员工有职位"不漂。

## 5. 权限目录与模版

- **权限 key 目录**：保留 `Permissions.h`，两类：
  - **门户/能力权限**（`portal:*`、`salary:*`、`logs:read`、`stock:*` …）：**可授予**。`portal:super-admin` 也可授予——现状总裁/副总裁/部门经理都进超管门户，设成不可授予会让等价性失败。
  - **元权限 `rbac:manage`**：**唯一不可委派**。永不进 catalog/模版/授权写入；服务端对任何试图授予它的请求强拒（含隐藏字段、模版套用）。
- 补 **`portal:user`**（或定义为"任何登录用户默认能力"）——现状用户端靠名字进用户门户（[jwtUtils.cpp:512](pethospital/backend/controllers/auth/jwtUtils/jwtUtils.cpp)），动态化后须有明确权限载体。
- **只读接口** `GET /api/admin/permissions/catalog`：返回可授予 key（= `Permissions.h` 全集 − `rbac:manage`），供超管界面渲染。**单一真相源是代码**，接口只暴露不复制。
- **模版 seed**：把现有 `roleHasPermission` 的 6 个硬编码包（Boss/Finance/SuperAdmin/Personnel/Medical/Warehouse）落成 `permission_templates` seed。超管建职位后"套用模版"一键填 `position_permissions`，再增减。这就是"开发期把固定权限模版设计好"。

## 6. 鉴权改造（一次性全迁移，最关键）

**原则**：改造后授权判定只认权限 key，业务身份只认 `account_type`/`staff_kind`，都不认角色名。**禁止名字/权限双轨并存**。

**每请求解析**（保留并扩展现有模式）：
1. `getUserAuthTargetById` 按 user_id 查库 → 拿 `position_id` + `account_type`。
2. `loadPermissions(position_id)`：查 `position_permissions` 得权限集。
3. `roleHasPermission(name,key)` → `positionHasPermission(positionId,key)`。
4. 超管：`positions.system_key='super-admin'` → 隐式持 `rbac:manage`（硬编码不查表，防降权 brick）；业务权限仍来自 seed 等价包（非 root）。

**必须改造的入口清单（做一半=漏洞）**：
- [ ] `Permissions.cpp roleHasPermission` → 库驱动 `positionHasPermission`。
- [ ] `roleTypeUtils` 全部名字函数（`isBossRole/isPersonnelRole/isManagementRole/isFinancePortalRole/isMedicalStaffRole/isWarehouseStaffRole/isNormalUserRole/…` ~10 个）→ 改按 `portal:*`，然后删除。
- [ ] authMiddleware 10 个门户校验 `isValidBossPortalToken` 等 → 改判 `portal:*`。
- [ ] **DataScope/可见性过滤链**：[orderCommonHandler.cpp:16](pethospital/backend/controllers/common/orderCommon/orderCommonHandler.cpp)、[reservationCommonHandler.cpp:17](pethospital/backend/controllers/common/reservationCommon/reservationCommonHandler.cpp)、[searchCommonHandler.cpp:158](pethospital/backend/controllers/common/searchCommon/searchCommonHandler.cpp)。
- [ ] **JWT/session/WebSocket**：token TTL/refresh/logout、WS `onaccept` 依赖 `typeName/isManagementRole`：[jwtUtils.cpp:288](pethospital/backend/controllers/auth/jwtUtils/jwtUtils.cpp)、[AuthSessionStore.cpp:46](pethospital/backend/services/auth/AuthSessionStore.cpp)。
- [ ] **人事授权接口**：把用户改成固定"医生/仓库管理员/普通用户" → 改成"派职位 + 设 account_type"：[personnelHandler.cpp:210](pethospital/backend/controllers/modules/personnel/personnelHandler.cpp)。
- [ ] **医生身份业务**：预约医生列表/下单校验/上下班查"医生"角色 → 改 `staff_kind='doctor'`：[userHandlerReservation.cpp:84](pethospital/backend/controllers/modules/user/userHandler/userHandlerReservation.cpp)。
- [ ] **财务 + 定时任务**："普通用户"当"非员工" → 改 `account_type='staff'`：[financeHandler.cpp:66](pethospital/backend/controllers/modules/finance/financeHandler.cpp)、[scheduledTaskManager.cpp:456](pethospital/backend/utils/scheduledTaskManager/scheduledTaskManager.cpp)。
- [ ] **前端**：26+ 路由文件 + [roleUtils.ts](pethospital/frontend/src/core/auth/utils/roleUtils.ts)、[router/index.ts:60](pethospital/frontend/src/app/router/index.ts)、session guard、authStorage、布局切换 → 全改按 `/me` 权限集；角色名只用于展示。
- [ ] 复核每个判定点对"未知/空/无/已删职位"均 **fail-closed**。

**业务身份 vs 判权（决策 C4 落地要点）**：上面"医生/普通用户/人事"几处是把角色名当**业务身份**，不是判权。迁移分两类：
- **判权** → `portal:*`/能力权限。
- **业务身份** → 账户级 `account_type='staff'`（员工/客户）；职位级 `staff_kind='doctor'`（工种）。**不得**继续用可改名的职位名字符串。

**缓存**（正确性优先）：现有 `UserRoleCache`（`userId→roleName`，300s TTL）扛不住授权降权，"DEL 失败等 TTL"=安全洞。
- **P1 授权热路径不加缓存，每请求查 DB**，降权随 DB commit 即时生效（PetManager 本就每请求查库）。
- 若后续缓存，走**版本化**（`positions.permission_version` 或全局 `auth_version`，鉴权带版本读，变更 bump + bump session-version），不靠 TTL。

**已建立的 WebSocket 降权**（Codex 修正——"下一请求失败"覆盖不到长连接）：现状 WS 只在 `onaccept` 校验一次就加入 broadcaster（如 [doctorRoutes.cpp:231](pethospital/backend/routes/doctorRoutes/doctorRoutes.cpp)），撤权后旧连接仍会持续收医生队列/库存/管理端数据直到断线。必做其一：
- 授权/职位变更时 **bump 该用户 session-version 并主动关闭其相关 WS**（首选，即时）；或
- broadcaster **发送前 revalidate**（或定时 revalidate）该连接权限，失权即踢。
- 验证项须包含"撤权后已开 WS 在 N 秒内停止收数据"，不能只测下一次 HTTP 请求。

## 7. 元权限不可委派 + 超管保护（防 brick）

- **只有 `rbac:manage` 不可委派**：永不进 catalog/模版/授权写入；服务端强拒任何把它写进 `position_permissions` 的请求。杜绝 grant-of-grant 提权。`portal:super-admin` 是可授予门户权限，不在此列。
- **超管身份靠受保护系统职位**：`positions.system_key='super-admin'`（不可变、唯一），隐式持 `rbac:manage`（硬编码不走 `position_permissions`）。RBAC 管理接口**拒绝**删除它、移除其权限、改其 `system_key`、将其降级。
- **通用派人接口不得把用户派进超管职位**：派超管走单独强审计 break-glass 流程。
- **保留最后一个超管**：删除/停用最后一个超管用户时后端拒绝，防 brick。

## 8. 动态管理流程

**权力闸门（Codex 修正——修掉"派职位绕过授权"漏洞）**：把用户派进一个**已含权限**的职位，本质等于给这个人授了那些权限。所以派人不能一律放给普通管理员，按目标职位是否含权分级：

- 建空部门/职位（零权限）：**普通管理员**可做。
- 派人进**零权限职位**（纯标签）：**普通管理员**可做。
- 派人进**任何含权限的职位**（含 `portal:*`/`salary:*`/`logs:read`/`scope:*`/`user:delete` 等）：**需 `rbac:manage`（超管独占）**，与"授权"同级闸门。
- **禁止自我派权**：任何人不得把自己派进含权限职位（即便有 rbac:manage 也须走双人/审计，见 break-glass）。
- 派进超管职位（`system_key='super-admin'`）：永远只能走强审计 break-glass。
- 授权/收权、套模版：`requireAccess(rbac:manage)`（超管独占）。

> 这样决策 B 的"管理员随便加"仍成立（建组织结构、派人进描述性空职位自由），但"派人=变相授权"的逃逸口被堵死：任何会让某人获得实权的动作，都收敛到 `rbac:manage` 这一道闸门。

新增 `/api/admin/rbac/*` 与 `/api/admin/org/*`：
- `POST /org/departments`、`POST /org/positions`（普通管理员，建组织项，写审计）。
- `PUT /users/:id/position`：目标职位零权限 → 普通管理员；含权限 → 要 `rbac:manage`；超管职位 → break-glass；禁止自我派进含权职位。**服务端按目标职位实时权限集判定,不信前端。**
- `GET /rbac/positions/:id/permissions`、`PUT /rbac/positions/:id/permissions`（授权/收权，超管，写审计）。
- `POST /rbac/positions/:id/apply-template`（套模版，超管）。
- `GET /rbac/permissions/catalog`（可授予 key 列表）。

**新职位生命周期**：管理员建空职位（零权限）→ 超管套模版或勾权限（此刻起该职位"含权限"，再派人要 rbac:manage）→ 该职位的人下次请求即时生效；未授权前 fail-closed 什么都进不去。

## 9. 审计

- 复用现有操作日志。RBAC/组织变更（建/删部门职位、授权、收权、派职位、改 account_type）各记一条，含操作人、目标、动作、前后 diff。
- `system_operations`/`user_operations` 的 `system_role`/`user_role`：**ENUM → VARCHAR(64) 快照**。写入路径 [operationLogger.cpp:250](pethospital/backend/services/logger/operationLogger.cpp) 现由 `SELECT t.type` 改 `SELECT positions.name`，存文本快照——历史显示"当时职位名"，不随改名/删除被改写。**删除现有 `DatabaseMigrations.cpp` 里的 `kOperationRoleEnum` 对齐逻辑**（那是为旧 ENUM 服务的，本期退场）。

## 10. rebuild 方案（无迁移，直接重建）

> **适用范围（Codex 修正，务必看）：仅 dev/clean 库可直接 rebuild。** 现状仍有外键 helper 指向 `users.type_id → types.id`（如 [ForeignKeyMigrations.cpp:205](pethospital/backend/database/migrations/foreign_keys/ForeignKeyMigrations.cpp)），且 orders/reservations/salary/logs 等子表都挂在 `users` 上。直接删 `types`、重建 `users` 结构会破坏这些外键链路和真实数据。
> **一旦库里有需保留的真实数据**，不能 rebuild，必须写**保数据迁移**：保留 `users.id`；旧 `type_id` 映射到对应 seed `position_id`（客户映射到 `account_type='customer'`+`position_id=NULL`）；子表不重建、FK 不断；退休旧的 `type_id` 外键 helper。YANG 已确认本地走 dev reset；生产/带数据环境须先补这条迁移，本方案不覆盖。

1. `DatabaseMigrations.cpp` 建表清单：删 `types`，加 `departments`/`positions`/`position_permissions`/`permission_templates`/`permission_template_items`；`users` 用 `position_id` + `account_type` + CHECK 替换 `type_id`。同步移除指向 `types` 的外键 helper。
2. **seed 重放现有行为**：
   - seed 部门 + 职位（现有 10 角色按部门归类，设 `staff_kind`：医生/护士→医护部 staff_kind=doctor/nurse，仓库管理员→仓储部 warehouse，财务经理→财务部 finance，总裁/副总裁/部门经理→管理 management，人事经理→人事部 personnel）。
   - **普通用户 → 不建职位**，改为 `account_type='customer'`、`position_id=NULL`。
   - seed 权限模版（6 个现有包）→ 套到对应 seed 职位 → `position_permissions`。
   - seed 超管职位 `system_key='super-admin'` + 初始超管用户。
   - 员工用户 `account_type='staff'` + 对应 `position_id`。
3. **全部** `type_id`/`JOIN types` 引用改 `position_id`/`account_type`；`getRoleName/getRoleId` 等改指 `positions`（完整范围由 §10.4 硬门禁把关，不靠人工估数）。
4. **等价性 + 安全验证**（等价性只证"没漂"，不证"没漏洞"，两者都要）：
   - **旧角色 × 全部 permission key 矩阵对拍**：每 seed 职位对每 key 的 `positionHasPermission` == 改造前 `roleHasPermission`。
   - **每条 route/WebSocket guard 对拍**：放行/拒绝一致。
   - **业务身份对拍**：财务工资的员工集（`account_type='staff'`）、医生集（`staff_kind='doctor'`）与改造前一致。
   - **fail-closed 用例集**：空/未知/无/已删职位一律拒绝；客户账户访问员工端被拒。
   - **元权限不可授予**：直接授 `rbac:manage`、模版套用、隐藏字段提交全部被拒。
   - **降权即时生效**：撤权后同一旧 token 下一请求立即失败。
   - **静态硬门禁**（Codex 修正——"15 处 type_id"是低估，实际 `type_id`/`type_name`/`userRole`/`JOIN types`/`getRoleId`/`getRoleName` 命中遍布 backend model、JWT、auth middleware、外键迁移、Redis role cache、前端 storage、路由守卫、mock、API types）：迁移完成的**准入门禁**，不是可选检查——
     - 后端 `rg` **零命中**：`JOIN types`、用 `type_id` 判权、`RoleTypeUtils::*Role`。
     - 前端 `rg` **零命中**：用 `allowedRoles`/`userRole` 做**判权**（展示用途需显式标注豁免）。
     - 任一非零 = 迁移未完成，不得进入可发布状态（防"看似迁完、实则双轨残留"）。

## 11. 前端影响

- `/me` 下发当前用户**权限集** + `account_type`（不下发角色名判权）。
- 26+ 硬编码角色名文件改按权限显隐/路由；角色名残留只用于**展示**。
- 新增超管 RBAC 管理页：部门/职位树 + 工种设置 + 权限勾选（渲染自 catalog）+ 模版套用 + 派职位。

## 12. 分阶段实施

| 阶段 | 内容 | 产出/验证 |
|---|---|---|
| P0 | 操作日志 ENUM→VARCHAR（独立小改，可独立发布） | 日志能记任意职位名 |
| P1 | 建表 + seed 重放 + 后端判权改库驱动 + 业务身份改 account_type/staff_kind + 退休全部名字入口 | **等价性 + 业务身份对拍**；后端静态硬门禁零命中 |
| P3 | 前端权限化路由（allowedRoles/userRole 判权全退）+ 超管管理界面 | 浏览器走通；前端静态硬门禁零命中 |
| P2 | 组织/RBAC 管理 API（建职位:管理员；授权/派高权职位:rbac:manage）+ 元权限不可委派 + 超管保护 + WS 降权 + 审计 | 越权测试：管理员碰不到 rbac:manage；grant-of-grant/派高权职位/自我派权被拒；派超管职位被拒；撤权后 WS 停收 |
| P2.5 | org_scope（§16）：branches+总院 seed + user_scopes + 组织数据 scope 过滤 + 员工日志部门快照 + 超管配范围界面 | 部门 A 经理看不到部门 B 组织数据；最高管理层(scope:all)看全部分院；客户数据不受影响 |

> **可发布单元（Codex 修正——修掉"分阶段"与"禁止双轨"的矛盾）**：P1（后端按权限判权）与 P3（前端按权限路由）**必须合成同一个可发布单元**——现状前端仍按 `allowedRoles`/`userRole` 判路由守卫/布局/sessionStorage（[router/index.ts:60](pethospital/frontend/src/app/router/index.ts)、[roleUtils.ts](pethospital/frontend/src/core/auth/utils/roleUtils.ts)），P1 单独上线会让前端跳转/守卫/布局全错乱。**P0 可独立发布；P1+P3 打包一起发；P2 在其后。** 中间态（只有 P1 没有 P3）**只允许存在于本地开发分支，不允许部署**。这与 §0"禁止名字/权限双轨并存"一致：双轨只在开发分支的过程中短暂存在，绝不进可部署制品。

P1 是重头也是风险核心，**必须一次改干净、等价性 + 硬门禁兜住**。

## 13. 优缺点（如实）

**优点**：组织/角色不改代码发版；权限单一来源（key 在代码、映射在库）；建职位放开满足"管理员随便加"、授权超管独占守住安全；业务身份两层拆分让"客户 vs 员工"成一等事实、工资/医生判据干净；每请求解析降权即时。

**代价/风险**：
- P1 迁移面大且被低估（后端 ~10 函数 + 遍布的 type_id/JOIN types/getRoleId + 财务/医生/人事业务身份 + 前端 26+ 文件 + Redis role cache + session/WS），回归成本高，靠等价性 + 业务身份对拍 + 静态硬门禁兜。
- 超管是主钥匙 → 账号强保护 + 变更全审计。
- 缓存没配好会"降权不生效"——§6 列为必做。
- `account_type`/`position_id` 一致性靠 CHECK + 应用层双保险，写用户路径都要遵守。

## 14. 决策与开放项

**产品决策（YANG 已定，2026-07-05）**：
- **A. 超管 = 等价包**，不做 root。要 root 作为单独产品变更单独验证。
- **B. 建部门/职位放给普通管理员**；授权/收权超管独占；派人不得指向超管职位。
- **C. 业务身份 = C4 两层拆分**：`users.account_type`（customer/staff）+ `positions.staff_kind`（工种）。

**Codex 首轮已收敛**：门户权限可授予 vs 元权限不可委派；超管非 root；迁移入口补全；缓存正确性优先；超管保护用 `system_key`；补 `portal:user`；验证含 fail-closed/元权限/降权/静态兜底；命名用 departments+positions。

**D. 部门是真实管理边界（YANG 已定，2026-07-05）**：需要"部门经理只看本部门数据" → 纳入 org_scope（§16）。范围三级：**全部分院（最高管理层，scope:all）/ 分院 / 部门**；分院层按 YANG 要求**预留**（seed 单个"总院"，表结构就位，开分院不返工），最高管理层从第一天就能跨全部分院。

**留待实现期定的小项**：
- `staff_kind` 枚举是否够覆盖所有业务分支（P1 时对着代码所有"是不是某工种"判断点核一遍，缺哪个补哪个）。
- 一人多职位（v2）。
- 授权路径版本化缓存（P2 视性能再定，P1 先不缓存）。

## 15. 评审记录

**第一轮（Claude 出稿 → Codex 审计，2026-07-05）**：
| 点 | Codex 意见 | 处理 |
|---|---|---|
| portal:super-admin | 不能设不可委派——现状总裁/副总裁/部门经理都进超管门户，否则等价性失败 | 采纳：`portal:*` 全可授予，**只 `rbac:manage` 不可委派** |
| 超管=隐式全权 | 过强，是产品变更非等价迁移 | 采纳：默认"等价包 + rbac:manage"，root 单列（决策 A 取等价包） |
| 迁移入口不全 | 漏 DataScope/可见性、JWT/session/WS、人事、医生业务、财务/定时任务、前端 storage | 采纳：§6 清单补全 |
| 缓存 | UserRoleCache 300s TTL 扛不住授权降权，DEL 失败等 TTL=安全洞 | 采纳：P1 不缓存查 DB；后续版本化非 TTL |
| 空职位无害 | 只对提权成立，对治理不成立 | 部分采纳：治理风险记录在案；决策 B 仍放开建职位，授权守超管 |
| 超管保护 | 只靠 is_system 不够，需 system_key + 禁通用接口派超管 | 采纳：§7 system_key + break-glass |
| portal:user 缺失 | 用户端靠名字进门户 | 采纳：§5 补 portal:user |
| 等价性验证 | 只证没漂不证没漏洞 | 采纳：§10 补 fail-closed/元权限/降权/静态兜底 + 业务身份对拍 |
| 命名 | permission_type 混淆组织与权限 | 采纳：departments+positions |

**决策收敛（YANG，2026-07-05）**：A=等价包、B=放开建职位（授权守超管）、C=C4 两层业务身份、**D=部门是真实数据边界，需补 org_scope（见 §16）**。

**第二轮（Codex 审 v3 定稿，2026-07-05）**，全部采纳：
| 点 | Codex 意见 | 处理 |
|---|---|---|
| 派职位绕过授权 | 派人进已有高权职位=变相授权，绕过"授权超管独占" | 采纳：派进含权限职位需 `rbac:manage`，禁自我派权（§8）；决策 B 收紧 |
| rebuild 对非空库危险 | 有 FK helper 指 type_id，子表挂 users，rebuild 只适合 dev | 采纳：§10 写死"仅 dev/clean 库"+ 保数据迁移路径 |
| 分阶段与"禁双轨"矛盾 | P1 后端/ P3 前端分开上线会让前端错乱 | 采纳：§12 P1+P3 合成同一可发布单元，中间态只准本地分支 |
| 降权漏 WebSocket | WS onaccept 校验一次后持续收数据，撤权不断线 | 采纳：§6 加 bump session-version + 主动断 WS / broadcaster revalidate |
| 元权限只靠应用层 | 表结构没防 rbac:manage 写入 | 采纳：§4 两表加 `CHECK (permission_key <> 'rbac:manage')` |
| 组织 scope 被排除 | 部门只剩标签意义，做不到部门级数据隔离 | 采纳：§1 写明限制 + §14 决策 D 交 YANG |
| 迁移面低估 | "15 处"远不止，遍布 model/JWT/中间件/FK/Redis/前端 | 采纳：§10 静态检查升级为硬门禁（后端零 JOIN types/type_id 判权/RoleTypeUtils，前端零 allowedRoles 判权）|

**决策 D（YANG，2026-07-05）**：**部门是真实数据边界**——需要"部门经理只看本部门数据"。故本方案纳入 org_scope（§16），不再是纯标签。

## 16. 组织数据范围（org_scope，决策 D=是）

**要解决**：功能权限只答"能不能做"（能看工资），org_scope 答"能看**哪些行**"（只看本部门的工资 vs 全院的）。现状 DataScope 只有 All/MedicalAssigned/Owner（[DataScope.cpp:8](pethospital/backend/utils/dataScope/DataScope.cpp)），缺"本部门"这一档，本节补上。

### 16.1 关键边界：只切"组织/HR 数据"，不切"客户交易数据"

- **受 org_scope 约束**（按部门切）：员工列表、工资、员工操作日志、人事操作——这些是**组织内部数据**，天然属于某部门。
- **不受 org_scope 约束**（沿用旧 All/Owner/MedicalAssigned）：订单、预约、宠物——这些是**客户（宠物主人）数据**，不属于任何"部门"，用部门切它没有意义。硬切会切错。
- 判据：一份数据"属于某部门"才纳入 org_scope；属于某客户的，继续走客户维度 scope。

### 16.2 范围粒度：三级（预留分院层，YANG 追加）

自顶向下三级，对齐 FactoryERP 的 company/factory/workshop：

- **全部分院（最高管理层）**：持 `scope:all`（现有 key）→ 跨**所有分院**看/管全部组织数据。这就是 YANG 要预留的"能管全部分院的最高管理层"——它天然是范围树的根，不需要新机制，就是 `scope:all` 的语义从"全院"升级为"全部分院"。
- **分院（branch）**：范围限定在一个或几个分院 → 看这些分院下所有部门的组织数据。（本期**预留不全建**，见 16.2.1）
- **部门（department）**：范围限定在具体部门 → 只看本部门。

**层级关系**：分院 → 部门（部门隶属分院）；用户范围可授在"分院级"（整个分院）或"部门级"（单个部门）。

#### 16.2.1 分院层怎么"预留"（最小改动，避免日后重构）

PetManager 当前单院运营，但为了"以后开分院不用重构"，现在就把钩子留好：
- 新增 `branches` 表；`departments` 加 `branch_id` FK → `branches`。
- **seed 一个默认分院"总院"**，现有部门全挂它下面。当前一切照常单院运行。
- `scope:all` 明确定义为"全部分院"（最高管理层），不是"某一院内全部门"。
- `user_scopes` 预留 `branch_id`（可空）：非空=分院级范围（整个分院），空+`department_id`=部门级范围。本期只用部门级 + `scope:all` 两档；分院级授予接口/UI 待真正开分院时再启用。
- 这样"最高管理层跨全部分院"从第一天就成立；"某人只管 A 分院"是将来加数据 + 启用分院级授予，不动表结构。

```sql
CREATE TABLE branches (
    id         INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
    name       VARCHAR(64) NOT NULL UNIQUE,
    system_key VARCHAR(32) NULL UNIQUE,
    is_system  TINYINT NOT NULL DEFAULT 0,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
-- departments 加：branch_id INT NOT NULL，FK → branches(id)；seed 时全指向"总院"
-- user_scopes 加：branch_id INT NULL（非空=分院级范围；见 16.3）

### 16.3 一个人的部门范围从哪来（混合：默认隐式 + 超管显式）

```sql
-- 显式范围授予（超管独占，data-access = 敏感，与授权同级闸门）
-- 一行 = 一个范围授予：branch_id 非空=分院级（整个分院，预留）；否则 department_id=部门级
CREATE TABLE user_scopes (
    id            INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
    user_id       INT NOT NULL,
    branch_id     INT NULL COMMENT '非空=分院级范围（预留，本期不用）',
    department_id INT NULL COMMENT '部门级范围',
    granted_by    INT NULL,
    granted_at    TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_us_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT fk_us_branch FOREIGN KEY (branch_id) REFERENCES branches(id),
    CONSTRAINT fk_us_dept FOREIGN KEY (department_id) REFERENCES departments(id),
    CONSTRAINT chk_us_level CHECK (branch_id IS NOT NULL OR department_id IS NOT NULL)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

**有效范围**（自顶向下）：
- 持 `scope:all`（最高管理层）→ **全部分院**，忽略下面。
- 否则 = 自己职位所在部门（隐式默认）∪ `user_scopes` 里的部门级授予 ∪（预留）分院级授予展开成的该分院全部门。
- 隐式默认：部门经理不用额外配置，天然能看本部门（`position.department_id → branch`）。
- 显式扩展：跨部门（财务总监看多个部门）或跨分院，超管在 `user_scopes` 加行。
- **范围授予是 data-access 授予，归超管（`rbac:manage` 同级闸门）**，不让普通管理员派——否则绕过隔离（与 §8 派职位同理）。

### 16.4 强制点（后端 SQL 层，前端隐藏不算数）

- 组织数据的**列表查询**：无 `scope:all` 时追加 `WHERE <resource>.department_id IN (有效范围)`（仿 FactoryERP `appendScopeCondition`）。
- 组织数据的**单条读写**：范围外一律 **404**（不泄露存在性，仿 FactoryERP `employeeVisible`）。
- 每请求解析范围（同鉴权，不进 JWT，改了即时生效）。

### 16.5 数据的部门归属（要补的列）

org_scope 要 work，受约束的数据得能查到"属于哪个部门"：
- **员工**：`user → position.department_id`，已有链路，无需加列。
- **工资**：`salary → user → position.department_id`，JOIN 可得。
- **员工操作日志**：需在写日志时**快照 operator 的 department_id**（加列 `operator_department_id`）——否则事后按当前职位反查会因转岗而错乱（同"角色名存快照"的道理）。
- 客户数据（订单/预约/宠物）：不加、不切。

### 16.6 对实施的影响（这就是"另一坨工作"）

- 新增 `branches` 表 + `departments.branch_id` + seed 默认"总院"（分院层预留，16.2.1）。
- 新增 `user_scopes` 表（含预留 `branch_id`）+ 超管范围管理接口 + 审计。
- 组织数据的列表/单条查询全部加 scope 过滤（员工/工资/员工日志/人事）。
- 员工操作日志加 `operator_department_id` 快照。
- 前端超管界面加"给用户配可见部门"。
- **本期建**：部门级范围 + `scope:all`（最高管理层=全部分院）两档。**预留不建**：分院级范围授予接口/UI（等真开分院再启用，表结构已就位不返工）。
- **验证**：部门 A 经理看不到部门 B 的员工/工资/日志（列表被过滤、单条 404）；`scope:all`（最高管理层）看全部分院；客户数据不受影响；撤销范围后即时生效（含 §6 的 WS 处理）。
- **落位**：作为 **P2.5**（在 P2 RBAC 管理之后、P3 前端之前，或与 P2 合并），依赖 positions/departments/branches 已就位，同属超管管理面。

## 17. 实施与验证记录（2026-07-08，Claude 审核 + Codex 独立验证）

Codex 完成全部实施（P0–P3 + P2.5 org_scope + 超管 RBAC 管理 UI `SuperAdminRbac.vue`）并提交。Claude 按本方案做审核验证，Codex 独立复核。

**已验证通过**：后端 `bin/build.sh` + 前端 `npm run build` + `bin/verify.sh backend` 18/18；硬门禁六项全零（后端无 `JOIN types`/`RoleTypeUtils::*Role`/`roleHasPermission`/`resolveForRole` 判权；前端无 `allowedRoles`/`roleUtils`/`is*PortalRole` 判权）。一次性库 DB 级实测：10 seed 职位 + 74 权限落库、职能职位 scope:all 落库、`position_permissions`/`permission_template_items` 的 `CHECK(permission_key<>'rbac:manage')` 实测挡住写入。

**审核发现并修复**：
1. **（Claude）seed 职能角色缺 `scope:all`**：super-admin/finance-*/department-manager/personnel-manager 无 scope:all，org_scope 把它们限到本部门，违反 §10.4 等价性（超管看不到全院日志、财务无法全院发薪）。YANG 拍板"内置职能角色看全院"。修复：给这 5 个职位 + Finance/SuperAdmin/Personnel 三模版各补 scope:all（doctor/nurse/warehouse 不碰组织数据，不补）。
2. **（Codex 阻断级）`PUT /api/admin/rbac/positions/<id>/permissions` 授权门回退**：提交版误用 `isValidManagementToken`，任一管理用户可给自己岗位授 portal:super-admin/scope:all/user:delete，绕过"授权归超管"。修复：PUT 分支改 `isValidPermissionToken(kRbacManage)`，GET 仍管理端。修复后提权链两道门（授权 + 派高权职位）都要 rbac:manage，闭合。

**已知限制（本期不做，记录）**：
- 多实例 WS 撤权：`AccessRevocation::closeRealtimeConnections()` 只关本进程连接；多实例部署需补跨实例 access-revoked 广播（与 §6 单实例假设一致）。
- `scope:all` 同时用于 org_scope（HR 数据）与 DataScope（客户交易数据）：本期耦合可接受（职能角色无 portal:user/portal:medical，够不到客户端点）；长期可拆 `org:scope:all` / `data:scope:all`，属新增设计，另开评审。
- HTTP 级越权矩阵（非超管管理用户 PUT 改权限应 403）待在可连库环境补跑；DB 级 + 静态门禁已覆盖核心不变量。
- rebuild 仅限 dev/clean 库（§10）。
