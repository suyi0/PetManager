# 前端技术与结构设计说明

本文档用于说明宠物医院管理系统前端的技术选型、目录结构、角色模块划分、状态管理、缓存策略和维护约定。

## 1. 技术栈

前端项目基于 Vue CLI 构建，主要技术如下：

| 技术 | 用途 |
| --- | --- |
| Vue 3 | 页面组件、响应式状态和视图渲染 |
| TypeScript | 类型约束、接口数据结构定义和维护安全性 |
| Vue Router 4 | 前端路由、角色入口和权限路由守卫 |
| Vuex 4 | 全局状态管理、角色端业务数据缓存 |
| Axios | HTTP 请求封装 |
| Sass / CSS | 页面样式和模块化界面设计 |
| Workbox / PWA 插件 | 构建阶段生成 Service Worker 相关资源 |
| CropperJS | 用户头像等图片裁剪能力 |

常用命令：

```bash
npm run serve
npm run build
npm run lint
```

## 2. 总体结构

前端源码主要位于 `src` 目录：

```text
src
├── api                 # 全局 HTTP 封装、响应结构、错误处理
├── app                 # 应用启动、根路由、根 store、全局 UI 状态
├── assets              # 图片、上传资源、全局静态样式
├── core                # 核心领域能力，目前主要是 auth 认证模块
├── modules             # 按角色拆分的业务模块
├── shared              # 跨角色复用的组件、布局、样式、工具
└── views               # 登录、注册等不属于某个业务角色的页面
```

设计原则：

- `app` 放应用级基础设施，例如根 store、根 router、启动逻辑。
- `core` 放核心领域能力，例如认证、用户会话、角色解析。
- `modules` 放角色端业务，角色之间尽量减少直接耦合。
- `shared` 放跨角色可复用能力，例如公共组件、布局、缓存工具。
- `views` 放登录、注册这类全局入口页面。

## 3. 角色模块划分

业务功能按角色拆在 `src/modules` 下：

```text
modules
├── user              # 用户端：宠物档案、预约、订单记录、个人信息
├── doctor            # 医生端：工作台、待接诊队列、预约记录、诊单、订单
├── super-admin       # 超级管理员端：系统首页、用户、考勤、日志
├── warehouse-admin   # 仓库管理员端：库存、入库、出库、预警、操作流
├── finance           # 财务端：财务首页、工资摘要、工资详情
├── personnel         # 人事端：用户/员工相关管理
└── boss              # 总裁端：总览、股权/分红等管理
```

每个角色模块通常包含：

```text
模块名
├── api       # 当前角色端调用后端接口的方法和类型
├── router    # 当前角色端路由配置
├── store     # 当前角色端 Vuex state/actions/mutations/types
├── utils     # 当前角色端业务工具和本地缓存封装
├── views     # 当前角色端页面
└── components # 当前角色端内部复用组件
```

## 4. 路由设计

全局路由入口在：

```text
src/app/router/index.ts
```

它负责整合各角色端路由：

- 用户端路由
- 医生端路由
- 总裁端路由
- 财务端路由
- 人事端路由
- 超级管理员端路由
- 仓库管理员端路由

路由守卫会检查：

- 页面是否需要登录：`meta.requiresAuth`
- 当前用户角色是否允许访问：`meta.allowedRoles`
- Vuex 中是否已有登录状态
- 必要时通过认证 action 检查登录状态

如果角色不匹配，会清理无效认证状态并跳回登录入口。

## 5. HTTP 与错误处理

全局 HTTP 能力主要位于：

```text
src/api/http.ts
src/api/httpError.ts
src/api/response.ts
```

设计目标：

- 统一后端响应结构解析。
- 统一 HTTP 错误转换。
- 对认证失效等情况进行集中处理。
- 避免每个页面重复写请求错误判断。

业务模块只需要在自己的 `api` 目录中定义接口方法和数据类型。

## 6. Vuex 状态管理

项目使用 Vuex 作为全局状态管理工具。

根 store 位于：

```text
src/app/store
```

角色端 store 位于各自模块中：

```text
src/modules/<role>/store
```

每个角色端 store 通常包含：

- `state.ts`：业务数据和缓存元信息初始值。
- `types.ts`：状态结构和业务数据结构。
- `actions.ts`：异步请求、缓存读取、业务写入。
- `mutations.ts`：同步修改 Vuex 状态。

当前设计中，页面尽量不直接请求接口，而是通过 store action 完成：

```text
页面 -> dispatch action -> api 请求 / localStorage 读取 -> commit mutation -> 页面渲染
```

这样可以统一处理缓存、加载状态和错误状态。

## 7. 缓存策略

当前前端缓存采用两层结构：

```text
Vuex 内存缓存 + localStorage 持久化缓存
```

### 7.1 Vuex 缓存元信息

公共缓存元信息定义在：

```text
src/app/store/cacheMeta.ts
```

每一类业务数据都有一份独立 `CacheMeta`：

```ts
interface CacheMeta {
  loaded: boolean;
  dirty: boolean;
  loading: boolean;
  lastFetchedAt: number | null;
}
```

含义：

- `loaded`：是否成功加载过。
- `dirty`：是否被业务操作标记为过期。
- `loading`：是否正在请求。
- `lastFetchedAt`：最近一次成功加载时间。

是否重新请求由 `shouldFetch(meta, force)` 统一判断，优先级为：

```text
force > 未加载 > dirty > TTL 过期
```

### 7.2 localStorage 版本化缓存

版本化本地缓存工具位于：

```text
src/shared/utils/versionedLocalCache.ts
```

localStorage 中不会直接裸存业务数据，而是统一包裹为：

```ts
{
  version: number,
  savedAt: number,
  data: T
}
```

这样可以解决三个问题：

- 字段结构变化时，通过 `version` 自动丢弃旧缓存。
- 缓存时间过长时，通过 `savedAt + ttlMs` 自动失效。
- JSON 损坏或旧版裸数据会被自动清理，避免页面渲染错误结构。

各角色端通过 `createVersionedLocalCacheAccessors` 创建自己的缓存访问器，只需要维护业务 key 和类型。

### 7.3 页面数据加载流程

页面进入时通常执行 `ensureXxxData`：

```text
1. 判断 Vuex 缓存是否可用
2. 如果 Vuex 可用，直接渲染
3. 如果 Vuex 不可用，尝试读取 localStorage
4. localStorage 命中则写回 Vuex 并渲染
5. localStorage 不可用则请求后端
6. 后端成功返回后，同时写入 Vuex 和 localStorage
```

业务新增、修改、删除后：

- 对受影响的数据调用 `markCacheDirty`。
- 必要时立即刷新当前页面数据。
- 对列表和详情通常分开管理缓存，避免详情数据污染列表。

### 7.4 典型缓存示例

医生端：

- 值班状态
- 待接诊队列
- 预约摘要
- 当前预约详情
- 订单摘要
- 当前订单详情

用户端：

- 宠物档案
- 预约医生
- 预约时间表
- 预约记录摘要
- 当前预约详情
- 订单摘要
- 当前订单详情

财务端：

- 首页统计
- 工资管理数据
- 工资摘要列表
- 当前工资详情

超级管理员端：

- 首页统计
- 用户列表
- 考勤记录
- 日志数据

## 8. 实时数据设计

项目中部分首页数据支持实时更新。

当前思路：

- 前端只在对应首页建立实时连接。
- 后端有数据变化时主动推送首页摘要数据。
- 前端收到推送后直接更新首页 Vuex 和 localStorage。
- 与首页数据相关的列表缓存只标记为 `dirty`，不立刻拉全量列表。

这样可以避免频繁刷新大列表，同时保证首页数据及时变化。

## 9. 认证与权限

认证相关能力集中在：

```text
src/core/auth
```

主要职责：

- 登录状态保存和恢复
- 当前用户资料缓存
- 角色解析
- 邮箱/手机号格式校验
- 登录过期处理
- 根据角色跳转对应入口

认证校验工具位于：

```text
src/core/auth/utils/authValidators.ts
```

角色解析工具位于：

```text
src/core/auth/utils/roleUtils.ts
```

## 10. 页面状态组件

跨页面的异步状态组件位于：

```text
src/shared/components/AsyncViewState.vue
```

用于统一展示：

- 加载中
- 错误信息
- 空状态
- 重试入口

建议后续所有列表页和详情页继续复用这个组件，减少每个页面重复写加载/错误状态。

## 11. 响应式与移动端

公共响应式能力位于：

```text
src/shared/composables/useViewport.ts
src/shared/layouts/ResponsivePage.vue
src/shared/layouts/MobileShell.vue
src/shared/styles/mobile.css
```

设计方向：

- 桌面端保持管理系统的信息密度。
- 移动端采用单列、卡片、底部/顶部导航等适合触屏的交互。
- 角色模块中可以分别维护 desktop 和 mobile 页面。

## 12. 代码维护约定

建议后续继续遵守以下约定：

1. 角色业务放在 `modules/<role>` 中。
2. 跨角色复用能力放在 `shared` 或 `core`。
3. 认证、角色、登录状态相关逻辑放在 `core/auth`。
4. 页面不要直接操作 localStorage，统一走模块 `utils/*DataCache.ts`。
5. 页面不要重复写请求流程，优先走 Vuex action。
6. 新增列表缓存时，需要同步设计：
   - Vuex 数据字段
   - `CacheMeta`
   - localStorage key
   - `ensureXxx`
   - `refreshXxx`
   - `markXxxDirty`
7. 详情缓存如果只展示一条记录，应采用“新详情覆盖旧详情”的策略。
8. 接口路径、前端 API 方法名和后端路由语义尽量保持一致。

## 13. 推荐后续优化

后续可以继续优化：

- 清理构建中遗留的 `console` 警告。
- 将体积较大的页面继续拆成懒加载路由块。
- 为关键 store action 增加单元测试。
- 将实时推送事件进一步细分为领域事件，例如订单更新、工资更新、医生上下班。
- 为缓存 key 建立统一命名文档，减少后期重复命名。
- 给每个角色模块补充一份更细的 README，说明该角色的页面、接口、缓存和路由。
