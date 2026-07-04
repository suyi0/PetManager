# WORKPLAN — Claude ⇄ Codex 实现分工（v1）

> 依据 `~/.agents/TEAMWORK.md` 分工并行模式：同一时刻同一文件只由一方编辑。
> 角色：**Claude = Lead**（拆解、决策、整合、对交付负责），**Codex = Assist**。
> 双方已于 2026-07-04 确认本分工。接口契约以 `docs/DESIGN.md` §5（API）+ §4.5（枚举清单）为准。

## 文件所有权

| 区域 | 所有者 | 内容 |
|---|---|---|
| `CMakeLists.txt`、`bin/`、`.env.example`、根配置 | Claude | 工程骨架与构建脚本 |
| `backend/**` | Claude | `Db`/`Migrations`、auth、加密/HMAC 工具、employees/org 接口、中间件 |
| `docs/**`、`.agents/context/**`、`CLAUDE.md`/`AGENTS.md` | Claude | 方案与项目文档 |
| `frontend/`（脚手架层：vite.config、package.json、依赖安装、axios 封装骨架） | Claude | M3 交接前一次性建好，避免 Codex 沙箱装依赖受阻 |
| `frontend/src/**`（业务代码） | **Codex** | 登录页、路由守卫、Pinia store、员工列表页、表单弹窗、详情抽屉、枚举中文映射 |

## 边界规则

1. Codex 只写 `frontend/src/**`（含其内样式/类型文件）；需要改 `vite.config.ts` / 加依赖 / 动后端时，在交接消息中提出，由 Claude 执行。
2. 实现中发现 API 契约不合理：**不私自改**，反馈 Claude 裁决 → Claude 改 `DESIGN.md` → 双方按新契约各自实现。
3. 验证遵循「谁改谁自验 + Lead 端到端」：Codex 自验 `npm run build` + 页面流程；Claude 负责后端 curl 自验与最终全栈浏览器验证。
4. 收尾只关自己起的服务；不动对方或 YANG 已开的服务。

## 交接顺序

1. **M1+M2（Claude）**：骨架 + 后端全部接口，curl 自验通过（含并发取号、身份证加密/脱敏/审计）。
2. **交接点**：Claude 完成前端脚手架 + `shared/` axios 封装骨架，向 Codex 发交接消息（后端已就绪的接口清单 + 启动方式 + 契约文档位置），Codex 以 workspace-write 沙箱开工。
3. **M3（Codex）**：`frontend/src/**` 业务实现，交回「任务/改动文件/验证/风险」四段式交接。
4. **M4（Claude）**：集成验证（浏览器全流程）、项目文档、向 YANG 汇报。

## role 约定（已统一）

- 协作角色：Claude Lead / Codex Assist（本文件即为「先统一再动手」的凭据）。
- `admin_users.role` 字段：v1 固定 `'admin'`，预留 `'hr'`（只管员工）、`'viewer'`（只读）；v1 无 role 分支逻辑。
