# 医生端首版 UI 说明

这版改成你明确的 4 个核心区块，不再发散：

- 自己是否在线
- 待接诊队列
- 查看预约订单
- 创建就医订单

后续前端实现建议先做成单文件主页面，例如 `DoctorWorkbench.vue`，内部按区块组织。

## 严格对应现有后端的区块

- 在线状态：`POST /api/doctor/online`、`POST /api/doctor/offline`
- 在线医生参考：`GET /api/doctor/getDoctor`
- 创建就医订单：`POST /api/order/createOrder`
- 订单结构参考：`GET /api/order/getOrderList`、`GET /api/order/getOrderInformation/<id>`

## 需要先定 UI、后续再补医生视角数据的区块

- 待接诊队列
- 预约订单查看

这两个区块和现有预约、订单模型有关，但当前后端更偏用户视角，所以这版先把列表结构和交互入口定下来。

## 药品选择列表展示

创建就医订单页面不再只放 `medicine_id` 输入框，UI 里会直接出现药品列表区，展示逻辑按你现在后端仓库字段来设计：

- 药品名：`item_name`
- 药品类型：`item_type`
- 单价：`item_price`
- 库存：`stock`

这样你后面确认稿子后，前端实现可以直接做成“选中药品卡片后回填表单”的交互。

## 视觉方向

- 风格：医疗感 + 管理台感，收敛、清晰、专业
- 结构：延续超级管理员端的左导航 + 主工作区
- 重点：把“创建就医订单”提升成主操作区，其次才是在线状态与队列信息
