# Super Admin Frontend (Independent Draft)

This module is implemented without touching user-side pages and routes.

## Included

- Dedicated router: `src/super-admin/router/superAdminRouter.ts`
- Layout + pages:
  - `views/SuperAdminLayout.vue`
  - `views/pages/SuperAdminOverview.vue`
  - `views/pages/SuperAdminWorktime.vue`
  - `views/pages/SuperAdminUsers.vue`
- Backend-driven API wrapper:
  - `api/superAdminApi.ts`
  - `api/types.ts`

## Backend routes mapped

- `GET /admin/getWorkTimeRecord`
- `POST /admin/changeDoctorWorkTime`
- `GET /api/admin/getUsers`

## Moved Out

- 角色权限授予已迁到 `personnel` 端，不再属于超级管理员模块

## Notes

- Existing frontend router (`src/router/index.ts`) is untouched.
- To enable this module later, wire `superAdminRouter` into a dedicated entry point or merge routes in your main router.
