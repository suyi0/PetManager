# PetManager 权限边界设计记录

> 状态：本轮用于收敛 PetManager 现有角色权限边界，并承接 `DESIGN.md` 中 RBAC + Scope + Sensitive Permission 的后续方向。

## 0. 结论先行

- PetManager 当前不直接上完整 RBAC 表结构迁移，先把现有“中文角色名 + 门户路由”边界收紧到后端。
- `isValidManagementToken` 只保留为通用管理会话能力，不再作为具体业务门户的默认授权边界。
- 业务门户使用更窄 token scope：
  - `BossPortal`：总裁 / 副总裁。
  - `FinancePortal`：总裁 / 副总裁 / 财务总监 / 财务经理。
  - `SuperAdminPortal`：总裁 / 副总裁 / 部门经理 / 超级管理员。
- 后端已增加 `Permissions` 代码常量层，用 `portal:*`、`salary:*`、`logs:read`、`medical-record:*`、`doctor-work:write`、`user:delete`、`equity:*`、`stock:*`、`staff-role:write`、`scope:*` 等权限键表达当前角色包；`RoleTypeUtils` 的门户判断逐步委托到它。
- 后端已增加 `DataScope` 数据范围层，把当前行级可见性表达为 `All` / `MedicalAssigned` / `Owner`，供 `VisibilityFilter` 生成 SQL 过滤。
- 敏感动作已使用 `FinishSensitiveRoute` 强制写审计日志，并在日志 details/source 中标记 `permissionKey` 与 `sensitive_permission`。
- 后续若继续演进，方向是 **认证 -> 功能权限 -> 数据 Scope -> 敏感动作权限 -> 审计**，前端只做体验控制，后端才是安全边界。

## 1. 为什么这样做

PetManager 已经是多门户系统：总裁端、财务端、超级管理员端、人事端、医生端、仓库端、用户端。前端通过 `allowedRoles` 控制入口，但此前部分后端路由仍使用宽泛的 `management` 判断。

这会让“能进入某个管理角色组”被误当成“能执行所有管理门户动作”。例如财务角色应该能访问财务工资数据，但不应能手写请求访问总裁端股份接口或超级管理员用户/日志接口。

本轮先收紧后端门户边界，而不是立刻改数据库权限模型，是因为：

- 现有系统已经有清晰的角色门户分组，最小改动可以立刻降低越权面。
- 完整 RBAC + Scope 需要表结构、管理 UI、迁移、缓存失效、审计和大量接口测试，属于更大阶段。
- 当前最危险的不一致点是“前端已分门户、后端仍用宽 management”，可以先修。

## 2. 用户影响

- 财务角色继续能使用财务端，但不能靠手写请求访问总裁端或超级管理员端业务接口。
- 部门经理 / 超级管理员继续能使用超级管理员端，但不能靠手写请求访问财务端工资接口或总裁端股份接口。
- 总裁 / 副总裁仍是跨管理门户的高权限角色。
- 登录、刷新管理 token、会话失效逻辑不变，避免合法管理用户因为本轮边界收紧被迫下线。

## 3. 当前后端边界

### 3.1 认证与会话

- JWT 校验只回答“这个请求是谁发的、token 是否仍有效”。
- JWT 中的 `typeName` 只作为会话失效判断的输入，不作为业务授权放行依据；业务授权必须重新读取当前 DB-backed 角色/权限。
- `AuthSessionStore` 继续负责管理角色 session-version 失效。
- `isValidManagementToken` 保留给横向管理会话入口，例如 `/api/admins/session-renewals`。

### 3.2 门户授权

后端门户 token scope 应与前端 `roleUtils.ts` 的门户分组保持一致：

| 门户 | 后端校验 | 允许角色 |
|---|---|---|
| 总裁端 | `isValidBossPortalToken` | 总裁 / 副总裁 |
| 财务端 | `isValidFinancePortalToken` | 总裁 / 副总裁 / 财务总监 / 财务经理 |
| 超级管理员端 | `isValidSuperAdminPortalToken` | 总裁 / 副总裁 / 部门经理 / 超级管理员 |
| 人事端 | `isValidPersonnelToken` | 总裁 / 副总裁 / 人事经理 |
| 医疗端 | `isValidMedicalStaffToken` | 总裁 / 副总裁 / 医生 / 护士 |
| 仓库端 | `isValidWarehouseStaffToken` | 总裁 / 副总裁 / 仓库管理员 |

规则：新增或调整业务路由时，不要因为它在“管理后台”下就默认使用 `isValidManagementToken`。先判断它属于哪个业务门户或敏感动作，再选择最窄校验。

规则：不要在中间件里基于 JWT claims 做跨门户短路放行。即使 token 里写着 Boss，仍要走对应 `JwtUtils::isUserAuthorizedFor*`，以当前数据库角色为准；这样角色被撤销或调整后，旧 token 不会继续扩大权限。

### 3.3 功能权限键

`utils/permissions/Permissions` 是当前的代码常量权限包：

- 门户能力：`portal:boss`、`portal:finance`、`portal:super-admin`、`portal:personnel`、`portal:medical`、`portal:warehouse`。
- 功能 / 敏感能力：`salary:read`、`salary:write`、`logs:read`、`medical-record:read`、`medical-record:write`、`doctor-work:write`、`user:delete`、`equity:read`、`equity:write`、`stock:read`、`stock:write`、`staff-role:write`。
- 数据范围能力：`scope:all`、`scope:medical-assigned`。普通用户 / 未知角色不显式授予 scope，默认落到 owner 范围。

当前仍是代码内角色包映射，不是运行时可配置 RBAC。这样做的目的，是先把“角色名数组”升级成“权限键契约”，为后续表结构、管理 UI、scope 和审计留出清晰接口。

高风险动作或已拆出的功能权限路由应优先使用 `isValidPermissionToken(..., Permissions::k*)`，而不是只使用门户 token：

- 工资写入：`salary:write`。
- 工资列表 / 搜索 / 详情读取：`salary:read`。
- 股份分配 / 变更：`equity:write`。
- 股份分布读取：`equity:read`。
- 库存读取 / 搜索：`stock:read`。
- 库存上传 / 更新 / 删除：`stock:write`。
- 医生 / 仓库管理员身份分配与移除：`staff-role:write`。
- 操作日志读取 / 搜索：`logs:read`。
- 病历列表读取：`medical-record:read`（Boss / 超级管理员门户全量，医护角色包同样持有，数据面按 `scope:medical-assigned` 收敛到自己负责的订单）。
- 医护端病历 / 诊单记录创建：`medical-record:write`。
- 管理员调整医生排班 / 工作状态：`doctor-work:write`。
- 删除用户：`user:delete`。

其中工资、股份、日志、病历读写、医生排班/状态、删除用户、身份分配/移除、库存写入等敏感路由还应使用 `OperationLogger::FinishSensitiveRoute(...)` 收口日志。当前不新增独立 `audit_logs` 表，先复用 `user_operations` / `system_operations`：`source` 以 `sensitive:<permissionKey>` 开头，`details` 包含 `auditType=sensitive_permission` 与 `permissionKey`，便于在现有日志页筛选。库存读取仍沿用普通操作日志，库存上传 / 更新 / 删除按 `stock:write` 纳入敏感审计。

### 3.4 数据可见性

订单、预约、搜索的行级可见性目前由 `VisibilityFilter` 收敛：

- Boss 看全部。
- 医护按 `doctor_id = currentUserId`。
- 普通用户按 owner 列过滤。
- 软删除默认排除。

`DataScope::resolveForRole(roleName, userId)` 是当前 scope 解析入口：

- `scope:all` -> `All`，不绑定当前用户，仍排除软删。
- `scope:medical-assigned` -> `MedicalAssigned`，绑定 `doctor_id = currentUserId`。
- 其他角色 / 空角色 / 未知角色 -> `Owner`，绑定 owner 列，fail-closed。

后续如果 PetManager 引入院区、科室、门店或更细组织数据范围，应扩展 `DataScope` 解析与 `VisibilityFilter` SQL 生成，而不是只在前端隐藏数据。

订单详情 / 更新等资源级授权也应复用同一条 scope 规则：通过 `DataScope::resolveForRole(...)` 解析当前 DB-backed 角色，再用 `VisibilityFilter` 查询 `orders`。这样医生 / 护士只能访问分配给自己的订单，普通用户只能访问自己的订单，Boss 可访问全部未软删订单；scope 外资源统一视为不存在。

订单更新接口在 scope 之上再做字段收窄：医护（`scope:medical-assigned`）只能修改诊疗数据 `order_data`；改动 `pet_id` / `doctor_id`（身份改派）、`order_type`、`order_status`（全部为支付 / 退款语义）返回 403。Boss 与 owner 路径保持原有全字段行为。

管理员病历列表虽然由 `medical-record:read` 控制入口，但数据面仍必须套同一条订单 scope：功能权限回答“能不能读病历”，`DataScope` 回答“能读哪些订单病历”。当前 Boss 依 `scope:all` 看全部未软删订单，医护依 `scope:medical-assigned` 看自己负责的订单，其他角色默认 owner 范围、fail-closed。

## 4. 后续 RBAC + Scope 方向

当角色组合继续增加时，不再扩展更多散落的中文角色硬编码分支。短期先维护 `Permissions` 代码常量层；中长期再迁移到：

- `roles`：角色包，例如 `boss`、`finance_manager`、`super_admin`。
- `permissions`：动作权限，例如 `salary:read`、`salary:write`、`equity:read`、`equity:write`、`user:delete`、`logs:read`、`medical-record:read`、`medical-record:write`、`doctor-work:write`、`stock:write`、`staff-role:write`。
- `role_permissions` / `user_roles`：角色与账号关系。
- `user_scopes`：数据范围，例如全院、指定院区、指定科室、指定医生团队。
- `audit_logs`：敏感动作审计。

授权链路：

1. 校验 JWT 与 session-version。
2. 加载当前账号角色、权限、scope。
3. 校验功能权限，无权限返回 403。
4. 对列表、详情、更新、删除统一套 scope 过滤。
5. 对薪酬、股份、删除、日志、导出等敏感动作再校验专门权限。
6. 敏感动作写审计。

接口语义建议：

- 有功能权限但访问 scope 外数据时，详情/更新优先返回 404，避免泄露资源存在性。
- 缺功能权限时返回 403；缺 token、token 无效或 session-version 失效仍返回 401。
- 前端隐藏按钮只降低误操作，不作为安全边界。

## 5. 代价与风险

- 本轮仍然依赖现有中文角色名，未解决角色配置化问题。
- Boss 角色仍是跨门户高权限角色；这是当前产品语义，不是通用 RBAC 的最终形态。
- 现有部分前后端角色分组可能仍有 UX 不一致，需要后续逐项核对。例如用户端前端允许的角色与后端用户 token 校验并非完全同一张表。
- 没有新增运行时 HTTP 越权用例；当前验证主要锁定角色矩阵和构建测试。若要证明具体接口返回码，需要准备多角色测试账号和可用数据库状态。

## 6. 本轮已落地的边界

- `RoleTypeUtils` 增加 `isSuperAdminPortalRole` / `isFinancePortalRole`。
- `Permissions` 增加角色包到权限键的代码常量映射；`isBossRole` / `isSuperAdminPortalRole` / `isFinancePortalRole` 委托到权限键判断。
- `DataScope` 增加 `All` / `MedicalAssigned` / `Owner` 数据范围模型；订单、预约、搜索公共查询改为从 `DataScope` 生成 `VisibilityFilter`。
- 订单详情 / 更新等资源级授权改为复用 `DataScope` + `VisibilityFilter`，不再单独手写 Boss / owner 分支。
- 管理员病历列表改为复用 `DataScope` + `VisibilityFilter`，不再手写 `p.user_id = currentUser`，并统一排除软删订单。
- 医生端订单详情路由在医疗门户 token 之后追加订单 scope 校验，防止医生直接访问未分配订单详情。
- JWT 授权增加 `isUserAuthorizedForSuperAdminPortal` / `isUserAuthorizedForFinancePortal` / `isUserAuthorizedForBossPortal`。
- Auth middleware 增加 `isValidSuperAdminPortalToken` / `isValidFinancePortalToken` / `isValidBossPortalToken`。
- Auth middleware 增加 `isValidPermissionToken`，用于按权限键校验敏感动作。
- `isValidPermissionToken` 的权限不足返回 403，认证失败 / 会话失效仍返回 401，避免把越权误判为登录过期。
- 订单详情 / 更新等资源级 scope 失败返回 404，避免泄露订单是否存在；缺 token、token 无效或会话失效仍返回 401。
- `adminRoutes`、`financeRoutes`、`bossRoutes` 改用各自门户 token scope。
- 工资读取/写入、股份读取/写入、日志读取/搜索、管理员病历读取、医护端病历写入、医生排班/工作状态写入、删除用户路由改用敏感权限键校验。
- 仓库库存读取/搜索/条件查询路由改用 `stock:read`，上传/更新/删除路由改用 `stock:write`。
- 工资读取/写入、股份读取/写入、日志读取/搜索、管理员病历读取、医护端病历写入、医生排班/工作状态写入、删除用户、库存写入路由改用 `FinishSensitiveRoute`，成功、失败、校验异常输入都带敏感审计标记落入现有操作日志。
- 人事端医生 / 仓库管理员身份分配与移除路由改用 `staff-role:write`，并纳入 `FinishSensitiveRoute` 敏感审计。
- `role_permission_tests` 锁定前后端门户角色矩阵，防止后续把财务、超级管理员、Boss 边界混回宽泛 management。
- `permission_model_tests` 锁定角色包到权限键的映射，确保财务、超级管理员、Boss、仓库、人事、医护的功能能力不被误扩。
- `data_scope_tests` 锁定角色到数据范围的映射，以及 `VisibilityFilter` 基于 scope 生成的 SQL 片段。
- `sensitive_permission_route_tests` 锁定敏感路由不能退回只看门户 token，也不能绕过敏感审计收口。
- `authMiddleware` 移除基于 `claims.typeName` 的 Boss 跨门户短路；`auth_middleware_boundary_tests` 锁定旧 token 角色声明不能绕过当前 DB-backed 授权。
- 医护角色包补授 `medical-record:read`：医护可经病历列表接口按 `scope:medical-assigned` 查看自己负责的订单病历，数据面由 `VisibilityFilter` 收敛。
- 医生端订单更新接口对医护 scope 做字段收窄：仅 `order_data` 可改，改派（`doctor_id`）、`pet_id`、`order_type`、支付状态（`order_status`）拒绝并返回 403。
