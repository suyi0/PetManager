<template>
  <section class="page">
    <!-- 授予权限 -->
    <section class="card grant">
      <div class="grant__head">
        <span class="grant__title">授予权限</span>
        <span class="grant__hint"
          >从下方列表选择用户，选择权限后授予或移除</span
        >
      </div>

      <div class="grant__row">
        <span class="field-label">权限</span>
        <div class="segment">
          <button
            v-for="role in grantableRoles"
            :key="role.label"
            type="button"
            class="segment__btn"
            :class="{ 'segment__btn--active': selectedRoleKey === role.label }"
            @click="selectedRoleKey = role.label"
          >
            {{ role.label }}
          </button>
        </div>

        <div class="ops">
          <button class="btn btn-grant" @click="grant">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2.2"
            >
              <path d="M20 6 9 17l-5-5" />
            </svg>
            授予
          </button>
          <button class="btn btn-remove" @click="revoke">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2.2"
            >
              <path d="M5 12h14" />
            </svg>
            移除
          </button>
        </div>

        <div class="target" :class="{ 'target--empty': !userID }">
          <template v-if="userID">
            <span class="target__avatar" :class="`avatar--${selectedKind}`">{{
              selectedInitial
            }}</span>
            <span class="target__name">{{ selectedName }}</span>
            <span class="target__id">#{{ userID }}</span>
            <span
              v-if="selectedRole"
              class="pill"
              :class="`pill--${selectedKind}`"
              >{{ selectedRole }}</span
            >
          </template>
          <span v-else class="target__hint">未选择用户</span>
          <label class="target__manual">
            ID
            <input
              v-model.number="userID"
              type="number"
              min="1"
              placeholder="手填"
            />
          </label>
        </div>
      </div>

      <div class="status" :class="`status--${messageType}`">
        <span class="status__dot"></span>
        <span class="status__text">{{ message }}</span>
      </div>
    </section>

    <!-- 用户列表 -->
    <section class="card list">
      <div class="list__head">
        <div class="list__title">
          用户列表 <span class="count-pill">共 {{ total }} 人</span>
        </div>
        <div class="tools">
          <div class="search">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
            >
              <circle cx="11" cy="11" r="7" />
              <path d="m21 21-4.3-4.3" />
            </svg>
            <input
              v-model.trim="keywordInput"
              type="text"
              placeholder="搜索用户名 / 邮箱 / 手机号"
              @keyup.enter="applySearch"
            />
          </div>
          <button class="btn-ghost" @click="loadUsers">刷新</button>
          <AppPager
            :page="page"
            :total-pages="totalPages"
            @update:page="page = $event"
          />
        </div>
      </div>

      <div
        ref="tableShellRef"
        class="table-wrap"
        :style="{ '--rows': pageSize }"
      >
        <table>
          <thead>
            <tr>
              <th class="col-id">ID</th>
              <th class="col-role">职位</th>
              <th>用户名</th>
              <th>邮箱 / 手机号</th>
              <th class="col-action">操作</th>
            </tr>
          </thead>
          <tbody>
            <tr
              v-for="item in users"
              :key="item.id"
              class="row"
              :class="{ 'row--active': userID === item.id }"
              @click="selectUser(item)"
            >
              <td class="col-id">
                <span class="id-text">#{{ item.id }}</span>
              </td>
              <td>
                <span
                  class="pill"
                  :class="`pill--${roleKind(item.type_name, item.type_id)}`"
                >
                  {{ formatRole(item.type_name, item.type_id) }}
                </span>
              </td>
              <td>
                <div class="user">
                  <span
                    class="avatar"
                    :class="`avatar--${roleKind(item.type_name, item.type_id)}`"
                    >{{ initialOf(item.name) }}</span
                  >
                  <span class="user__name">{{
                    item.name || "未命名用户"
                  }}</span>
                </div>
              </td>
              <td class="contact">
                {{ item.email || item.phone || "暂无联系方式" }}
              </td>
              <td class="col-action">
                <span v-if="userID === item.id" class="act act--picked"
                  >已选中</span
                >
                <span v-else class="act">选择</span>
              </td>
            </tr>

            <tr
              v-for="n in placeholderRows"
              :key="`ph-${n}`"
              class="placeholder"
            >
              <td colspan="5"></td>
            </tr>

            <tr v-if="users.length === 0">
              <td class="empty-cell" colspan="5">没有匹配的用户</td>
            </tr>
          </tbody>
        </table>
      </div>
    </section>
  </section>
</template>

<script lang="ts">
import {
  computed,
  defineComponent,
  nextTick,
  onBeforeUnmount,
  onMounted,
  ref,
  watch,
} from "vue";
import { useStore } from "vuex";
import {
  isSuperAdminPortalRole,
  resolveRoleName,
} from "@/core/auth/utils/roleUtils";
import { storeKey } from "@/app/store";
import AppPager from "@/shared/components/AppPager.vue";
import { calculateTotalPages } from "@/shared/utils/pagination";
import { UserRow } from "@/modules/super-admin/api/types";
import { personnelApi } from "../../api/personnelApi";

type MessageType = "info" | "success" | "error";
type RoleKind = "boss" | "doctor" | "warehouse" | "user";

interface GrantableRole {
  label: string;
  grantAction: string;
  revokeAction: string;
}

// 本页可授权的职位。系统职位很多，但只有接好授予/移除接口的才列在这里。
// 新增一个可授权职位 = 加一行（并在后端/store 接好对应 action），无需改模板。
const GRANTABLE_ROLES: GrantableRole[] = [
  {
    label: "医生",
    grantAction: "personnel/createDoctor",
    revokeAction: "personnel/deleteDoctor",
  },
  {
    label: "仓库管理员",
    grantAction: "personnel/createWarehouseManager",
    revokeAction: "personnel/deleteWarehouseManager",
  },
];

// 行高/表头高（与设计稿一致），用于按可用高度反推每页行数。
const ROW_HEIGHT = 56;
const HEADER_HEIGHT = 42;
const DEFAULT_PAGE_SIZE = 8;

export default defineComponent({
  name: "PersonnelRoleAccess",
  components: { AppPager },
  setup() {
    const store = useStore(storeKey);
    const userID = ref<number | null>(null);
    const selectedUser = ref<UserRow | null>(null);
    const selectedRoleKey = ref<string>(GRANTABLE_ROLES[0].label);
    const message = ref("等待操作");
    const messageType = ref<MessageType>("info");
    const users = ref<UserRow[]>([]);
    const total = ref(0);
    const page = ref(1);
    const pageSize = ref(DEFAULT_PAGE_SIZE);
    const keywordInput = ref("");
    const keyword = ref("");
    const tableShellRef = ref<HTMLElement | null>(null);

    const setMessage = (text: string, type: MessageType = "info") => {
      message.value = text;
      messageType.value = type;
    };

    const ensureUserID = () => {
      if (!userID.value || userID.value <= 0) {
        throw new Error("请先选择或输入合法用户ID");
      }
      return userID.value;
    };

    const formatRole = (typeName?: string, typeId?: number | null) => {
      return resolveRoleName(typeName, typeId) || "未知角色";
    };

    const roleKind = (typeName?: string, typeId?: number | null): RoleKind => {
      const role = formatRole(typeName, typeId);
      if (isSuperAdminPortalRole(role)) return "boss";
      if (role === "医生" || role === "护士") return "doctor";
      if (role === "仓库管理员") return "warehouse";
      return "user";
    };

    const initialOf = (name?: string) => {
      const trimmed = (name || "").trim();
      return trimmed ? trimmed.charAt(0) : "?";
    };

    // 仅当选中用户与当前 ID 一致时展示其姓名/角色（手填 ID 时只显示 #ID）
    const matchedUser = computed(() =>
      selectedUser.value && selectedUser.value.id === userID.value
        ? selectedUser.value
        : null
    );
    const selectedName = computed(
      () => matchedUser.value?.name || (userID.value ? "未命名用户" : "")
    );
    const selectedRole = computed(() =>
      matchedUser.value
        ? formatRole(matchedUser.value.type_name, matchedUser.value.type_id)
        : ""
    );
    const selectedKind = computed<RoleKind>(() =>
      matchedUser.value
        ? roleKind(matchedUser.value.type_name, matchedUser.value.type_id)
        : "user"
    );
    const selectedInitial = computed(() => initialOf(selectedName.value));

    const activeRole = computed(
      () =>
        GRANTABLE_ROLES.find((r) => r.label === selectedRoleKey.value) ||
        GRANTABLE_ROLES[0]
    );

    const totalPages = computed(() =>
      calculateTotalPages(total.value, pageSize.value)
    );

    // 当前页记录不足一页时补足的空行数量；零结果时不补（交给空状态行）。
    const placeholderRows = computed(() =>
      users.value.length === 0
        ? 0
        : Math.max(0, pageSize.value - users.value.length)
    );

    // 按表格容器实测高度反推每页行数，使列表恰好填满、页面无滚动。
    const applyPageSize = () => {
      const shellHeight = tableShellRef.value?.clientHeight ?? 0;
      if (!shellHeight) return false;
      const next = Math.max(
        4,
        Math.floor((shellHeight - HEADER_HEIGHT) / ROW_HEIGHT)
      );
      if (Number.isFinite(next) && next !== pageSize.value) {
        pageSize.value = next;
        return true;
      }
      return false;
    };

    const onResize = () => {
      if (applyPageSize()) void loadUsers();
    };

    const loadUsers = async () => {
      const result = await personnelApi.searchUsers({
        keyword: keyword.value,
        page: page.value,
        pageSize: pageSize.value,
      });
      users.value = result.items;
      total.value = result.total;
      if (page.value > totalPages.value) {
        page.value = totalPages.value;
      }
    };

    watch(page, () => {
      void loadUsers();
    });

    const applySearch = () => {
      keyword.value = keywordInput.value.trim();
      page.value = 1;
      void loadUsers();
    };

    const selectUser = (item: UserRow) => {
      userID.value = item.id;
      selectedUser.value = item;
      setMessage(`已选择用户 #${item.id}`);
    };

    const grant = async () => {
      const role = activeRole.value;
      try {
        await store.dispatch(role.grantAction, ensureUserID());
        setMessage(`${role.label}权限授予成功`, "success");
        await loadUsers();
      } catch (error) {
        setMessage(
          error instanceof Error ? error.message : `${role.label}权限授予失败`,
          "error"
        );
      }
    };

    const revoke = async () => {
      const role = activeRole.value;
      try {
        await store.dispatch(role.revokeAction, ensureUserID());
        setMessage(`${role.label}权限移除成功`, "success");
        await loadUsers();
      } catch (error) {
        setMessage(
          error instanceof Error ? error.message : `${role.label}权限移除失败`,
          "error"
        );
      }
    };

    onMounted(() => {
      window.addEventListener("resize", onResize);
      void nextTick(async () => {
        applyPageSize();
        await loadUsers();
      });
    });

    onBeforeUnmount(() => {
      window.removeEventListener("resize", onResize);
    });

    return {
      applySearch,
      formatRole,
      grant,
      grantableRoles: GRANTABLE_ROLES,
      initialOf,
      keywordInput,
      loadUsers,
      message,
      messageType,
      page,
      pageSize,
      placeholderRows,
      revoke,
      roleKind,
      selectUser,
      selectedInitial,
      selectedKind,
      selectedName,
      selectedRole,
      selectedRoleKey,
      tableShellRef,
      total,
      totalPages,
      userID,
      users,
    };
  },
});
</script>

<style scoped>
.page {
  --indigo: #4f46e5;
  --indigo-50: #eef2ff;
  --green: #059669;
  --green-50: #ecfdf5;
  --red: #dc2626;
  --red-50: #fef2f2;
  --amber: #b45309;
  --amber-50: #fffbeb;
  --text: #0f172a;
  --muted: #64748b;
  --faint: #94a3b8;
  --border: #e7e9ee;

  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 16px;
  height: 100%;
  min-height: 0;
  color: var(--text);
}

.card {
  background: #ffffff;
  border: 1px solid var(--border);
  border-radius: 14px;
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.04);
}

/* ---- 授予权限 ---- */
.grant {
  padding: 18px 20px;
  display: grid;
  gap: 16px;
}

.grant__head {
  display: flex;
  align-items: baseline;
  gap: 10px;
}

.grant__title {
  font-size: 15px;
  font-weight: 700;
}

.grant__hint {
  font-size: 13px;
  color: var(--muted);
}

.grant__row {
  display: flex;
  align-items: center;
  gap: 16px;
  flex-wrap: wrap;
}

.field-label {
  font-size: 12px;
  font-weight: 600;
  color: var(--muted);
}

.segment {
  display: inline-flex;
  padding: 4px;
  gap: 2px;
  background: #f1f2f5;
  border-radius: 10px;
}

.segment__btn {
  height: 34px;
  padding: 0 16px;
  border: 0;
  border-radius: 8px;
  background: transparent;
  color: var(--muted);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
}

.segment__btn--active {
  background: #ffffff;
  color: var(--indigo);
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.08);
}

.ops {
  display: flex;
  gap: 10px;
}

.btn {
  height: 38px;
  padding: 0 18px;
  border-radius: 9px;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
  border: 1px solid transparent;
  display: inline-flex;
  align-items: center;
  gap: 7px;
}

.btn svg {
  width: 16px;
  height: 16px;
}

.btn-grant {
  background: var(--green);
  color: #ffffff;
}

.btn-grant:hover {
  background: #047857;
}

.btn-remove {
  background: #ffffff;
  color: var(--red);
  border-color: #f3c9cd;
}

.btn-remove:hover {
  background: var(--red-50);
}

.target {
  margin-left: auto;
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 7px 12px 7px 8px;
  border: 1px solid var(--border);
  border-radius: 999px;
  background: #fafbfc;
}

.target--empty {
  background: #ffffff;
}

.target__avatar {
  width: 30px;
  height: 30px;
  border-radius: 50%;
  display: grid;
  place-items: center;
  font-weight: 700;
  font-size: 12px;
}

.target__name {
  font-size: 13px;
  font-weight: 700;
}

.target__id {
  font-size: 12px;
  color: var(--faint);
}

.target__hint {
  font-size: 13px;
  color: var(--muted);
  padding: 0 4px;
}

.target__manual {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  color: var(--muted);
  margin-left: 4px;
  padding-left: 10px;
  border-left: 1px solid var(--border);
}

.target__manual input {
  width: 64px;
  height: 30px;
  padding: 0 8px;
  border: 1px solid var(--border);
  border-radius: 7px;
  font-size: 13px;
  color: var(--text);
}

.status {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 13px;
}

.status__dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: var(--faint);
}

.status__text {
  color: var(--muted);
  font-weight: 600;
}

.status--success .status__dot {
  background: var(--green);
}

.status--success .status__text {
  color: var(--green);
}

.status--error .status__dot {
  background: var(--red);
}

.status--error .status__text {
  color: var(--red);
}

/* ---- 用户列表 ---- */
.list {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  min-height: 0;
}

.list__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 16px 20px;
  border-bottom: 1px solid var(--border);
  flex-wrap: wrap;
}

.list__title {
  font-size: 15px;
  font-weight: 700;
  display: flex;
  align-items: center;
  gap: 8px;
}

.count-pill {
  font-size: 12px;
  font-weight: 600;
  color: var(--muted);
  background: #f1f2f5;
  border-radius: 999px;
  padding: 2px 9px;
}

.tools {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}

/* 把分页器与搜索/刷新拉开距离 */
.tools :deep(.pager) {
  margin-left: 16px;
}

.search {
  display: flex;
  align-items: center;
  gap: 8px;
  height: 38px;
  padding: 0 12px;
  min-width: 280px;
  border: 1px solid var(--border);
  border-radius: 9px;
  background: #ffffff;
}

.search:focus-within {
  border-color: var(--indigo);
  box-shadow: 0 0 0 3px var(--indigo-50);
}

.search svg {
  width: 16px;
  height: 16px;
  color: var(--faint);
}

.search input {
  border: 0;
  outline: 0;
  font-size: 13px;
  width: 100%;
  color: var(--text);
  background: transparent;
}

.btn-ghost {
  height: 38px;
  padding: 0 14px;
  border: 1px solid var(--border);
  border-radius: 9px;
  background: #ffffff;
  color: var(--text);
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
}

.btn-ghost:hover {
  background: #f6f7f9;
}

.table-wrap {
  min-height: 0;
  overflow: hidden;
}

table {
  width: 100%;
  height: 100%;
  border-collapse: collapse;
  table-layout: fixed;
}

thead th {
  height: 42px;
  text-align: left;
  padding: 0 20px;
  font-size: 12px;
  font-weight: 600;
  color: var(--faint);
  background: #fbfbfc;
  border-bottom: 1px solid var(--border);
}

tbody td {
  height: 56px;
  padding: 0 20px;
  font-size: 14px;
  border-bottom: 1px solid #f1f2f4;
}

.col-id {
  width: 96px;
}

.col-role {
  width: 150px;
}

.col-action {
  width: 110px;
  text-align: right;
}

.row {
  cursor: pointer;
  transition: background 0.12s ease;
}

.row:hover {
  background: #fafbff;
}

.row--active {
  background: var(--indigo-50);
}

.row--active td:first-child {
  box-shadow: inset 3px 0 0 var(--indigo);
}

.id-text {
  font-weight: 700;
  color: var(--muted);
  font-variant-numeric: tabular-nums;
}

.user {
  display: flex;
  align-items: center;
  gap: 10px;
}

.avatar {
  width: 32px;
  height: 32px;
  border-radius: 50%;
  flex: 0 0 auto;
  display: grid;
  place-items: center;
  font-weight: 700;
  font-size: 13px;
}

.avatar--boss {
  background: #eef2ff;
  color: #4f46e5;
}

.avatar--doctor {
  background: #ecfdf5;
  color: #059669;
}

.avatar--warehouse {
  background: #fff7ed;
  color: #c2410c;
}

.avatar--user {
  background: #f1f5f9;
  color: #475569;
}

.user__name {
  font-weight: 600;
}

.contact {
  color: var(--muted);
  font-variant-numeric: tabular-nums;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.pill {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  height: 24px;
  padding: 0 10px;
  border-radius: 7px;
  font-size: 12px;
  font-weight: 600;
}

.pill::before {
  content: "";
  width: 6px;
  height: 6px;
  border-radius: 50%;
}

.pill--boss {
  background: var(--indigo-50);
  color: #4338ca;
}

.pill--boss::before {
  background: #6366f1;
}

.pill--doctor {
  background: var(--green-50);
  color: #047857;
}

.pill--doctor::before {
  background: #10b981;
}

.pill--warehouse {
  background: var(--amber-50);
  color: var(--amber);
}

.pill--warehouse::before {
  background: #f59e0b;
}

.pill--user {
  background: #f1f5f9;
  color: #475569;
}

.pill--user::before {
  background: #94a3b8;
}

.act {
  font-size: 13px;
  font-weight: 600;
  color: var(--indigo);
}

.act--picked {
  color: var(--green);
}

.empty-cell {
  height: calc(56px * var(--rows, 8));
  text-align: center;
  color: var(--faint);
}

@media (max-width: 860px) {
  .list__head {
    flex-direction: column;
    align-items: stretch;
  }

  .search {
    flex: 1;
    min-width: 0;
  }

  .contact {
    display: none;
  }
}
</style>
