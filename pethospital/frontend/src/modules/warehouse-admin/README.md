# Warehouse Admin

蓝黑主题的仓库管理员前端模块，设计基于方案 B「夜航面板」。

## Structure

- `router/warehouseAdminRouter.ts`: 仓库管理员路由定义
- `views/WarehouseAdminLayout.vue`: 仓库管理员布局
- `views/pages/*`: 页面级视图
- `components/*`: 仓库模块专用组件
- `api/*`: 后端接口封装与类型定义
- `store/*`: 仓库管理员 Vuex 状态、mutations 与 actions
- `utils/*`: 会话守卫等模块工具

应用级 router 和 store 位于 `src/app/router`、`src/app/store`。
新代码不要再引用已删除的旧路径 `src/router` 或 `src/store`。

包含页面：

- `overview` 仓库总览
- `inventory` 库存列表与编辑/删除交互
- `create` 新增物品
- `warnings` 预警中心
- `logs` 操作日志

接口默认对接后端仓库管理模块：

- `GET /api/warehouse-managers/items`
- `GET /api/warehouse-managers/items/data-id/:id`
- `GET /api/warehouse-managers/items/item-name/:name`
- `POST /api/warehouse-managers/items`
- `PATCH /api/warehouse-managers/items/:id`
- `DELETE /api/warehouse-managers/item-deletions`
