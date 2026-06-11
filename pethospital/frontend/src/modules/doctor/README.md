## Doctor Module

Standalone doctor-facing frontend module.

Structure:

- `router/DoctorRouter.ts`: doctor route definitions
- `views/DoctorLayout.vue`: shell layout with sidebar + topbar
- `views/pages/*`: page-level views mapped to design drafts
- `views/desktop/*`: desktop-specific doctor views
- `views/mobile/*`: mobile-first doctor views
- `components/*`: shared doctor UI building blocks
- `api/*`: backend API wrapper and local types
- `store/*`: doctor Vuex state, mutations, and actions
- `utils/*`: doctor workflow helpers

App-level router and store live in `src/app/router` and `src/app/store`.
Use `@/shared/*` for shared components and layouts instead of fragile relative
paths.
