# Super Admin Module

超级管理员前端模块，挂载在应用级路由 `src/app/router/index.ts` 中。

## Structure

- `src/modules/super-admin/router/superAdminRouter.ts`: super admin route definitions
- `src/modules/super-admin/views/SuperAdminLayout.vue`: shell layout
- `src/modules/super-admin/views/pages/*`: page-level views
- `src/modules/super-admin/components/*`: module-specific UI components
- `src/modules/super-admin/api/*`: backend API wrapper and local types
- `src/modules/super-admin/store/*`: module Vuex state, mutations, and actions

## Backend routes mapped

- `GET /api/admins/work-time-records`
- `POST /api/admins/doctor-work-time-changes`
- `GET /api/admins/users`

## Moved Out

- 角色权限授予已迁到 `personnel` 端，不再属于超级管理员模块

## Notes

- 应用级 router 位于 `src/app/router/index.ts`。
- 应用级 store 位于 `src/app/store`。
- 新代码不要再引用已删除的旧路径 `src/router` 或 `src/store`。
