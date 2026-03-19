# Warehouse Admin

蓝黑主题的仓库管理员前端模块，设计基于方案 B「夜航面板」。

包含页面：

- `overview` 仓库总览
- `inventory` 库存列表与编辑/删除交互
- `create` 新增物品
- `warnings` 预警中心
- `logs` 操作日志

接口默认对接后端仓库管理模块：

- `GET /warehouseManager/select`
- `GET /warehouseManager/select/dataID/:id`
- `GET /warehouseManager/select/item_name/:name`
- `POST /warehouseManager/upload`
- `PATCH /warehouseManager/updata/:id`
- `DELETE /warehouseManager/delete/:id`
