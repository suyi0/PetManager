<template>
  <section class="page">
    <header class="ledger-head panel">
      <div>
        <p class="section-label">用户管理</p>
        <h2>用户台账</h2>
        <p class="ledger-head__copy">
          按用户名称、邮箱、手机号检索，使用角色筛选快速定位需要处理的账号。
        </p>
      </div>
      <div class="ledger-head__actions">
        <button
          type="button"
          class="button button--ghost"
          @click="refreshUsers"
        >
          刷新
        </button>
        <button type="button" class="button" @click="openCreateDialog">
          新增普通用户
        </button>
      </div>
    </header>

    <section class="panel ledger-panel">
      <div class="toolbar">
        <div class="toolbar__search">
          <input
            v-model.trim="keywordInput"
            class="search-input"
            type="text"
            placeholder="按用户名 / 邮箱 / 手机号查询"
            @keyup.enter="applySearch"
          />
          <span class="search-hint">Enter 搜索</span>
          <button
            v-if="keyword"
            type="button"
            class="button button--ghost toolbar__clear"
            @click="clearSearch"
          >
            清除
          </button>
        </div>

        <div class="role-filters" aria-label="角色筛选">
          <button
            v-for="option in roleOptions"
            :key="option.key"
            type="button"
            class="role-filter"
            :class="{ 'role-filter--active': activeRole === option.key }"
            @click="setRole(option.key)"
          >
            <span>{{ option.label }}</span>
            <strong>{{ roleCounts[option.key] }}</strong>
          </button>
        </div>
      </div>

      <div class="ledger-meta">
        <span>共 {{ total }} 条记录</span>
        <span v-if="keyword">当前搜索：{{ keyword }}</span>
        <span>角色：{{ activeRoleLabel }}</span>
      </div>

      <div v-if="listError" class="state-banner state-banner--error">
        <span>{{ listError }}</span>
        <button type="button" class="button button--ghost" @click="loadUsers">
          重试
        </button>
      </div>

      <div class="table-shell" :class="{ 'table-shell--loading': loading }">
        <table class="user-table">
          <thead>
            <tr>
              <th>姓名</th>
              <th>角色</th>
              <th>手机号</th>
              <th>邮箱</th>
              <th>状态</th>
              <th>生日</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="item in users" :key="item.id">
              <td>
                <div class="user-cell">
                  <strong>{{ item.name || "未命名用户" }}</strong>
                  <span>用户编号 {{ item.id }}</span>
                </div>
              </td>
              <td>
                <span class="role-pill" :class="roleTone(item)">
                  {{ formatRole(item) }}
                </span>
              </td>
              <td>{{ item.phone || "未填写" }}</td>
              <td>{{ item.email || "未填写" }}</td>
              <td>
                <span
                  class="status-pill"
                  :class="
                    item.status === 'online'
                      ? 'status-pill--online'
                      : 'status-pill--offline'
                  "
                >
                  {{ formatStatus(item) }}
                </span>
              </td>
              <td>{{ item.birthday || "未填写" }}</td>
              <td>
                <button
                  type="button"
                  class="table-action"
                  @click="goToDetail(item.id)"
                >
                  查看
                </button>
              </td>
            </tr>

            <tr v-if="!loading && users.length === 0">
              <td colspan="7">
                <div class="empty-state">
                  <strong>没有匹配的用户记录</strong>
                  <span>调整搜索词或切换角色筛选后再试。</span>
                </div>
              </td>
            </tr>

            <tr
              v-for="index in placeholderRows"
              :key="`placeholder-${index}`"
              class="placeholder-row"
            >
              <td colspan="7"></td>
            </tr>
          </tbody>
        </table>

        <div v-if="loading" class="loading-layer">正在同步用户数据...</div>
      </div>

      <AppPager
        :page="page"
        :total-pages="totalPages"
        @update:page="page = $event"
      />
    </section>

    <div
      v-if="showCreateDialog"
      class="dialog-backdrop"
      @click.self="closeCreateDialog"
    >
      <div class="dialog">
        <div class="dialog__head">
          <div>
            <p class="section-label">新增账号</p>
            <h3>新增普通用户</h3>
            <span>创建后可进入详情继续维护资料。</span>
          </div>
          <button
            type="button"
            class="button button--ghost"
            @click="closeCreateDialog"
          >
            关闭
          </button>
        </div>

        <form class="form" @submit.prevent="handleCreate">
          <label>
            <span>姓名</span>
            <input
              v-model.trim="form.name"
              type="text"
              placeholder="请输入姓名"
            />
          </label>
          <label>
            <span>手机号</span>
            <input
              v-model.trim="form.phone"
              type="text"
              placeholder="选填，建议填写"
            />
          </label>
          <label>
            <span>邮箱</span>
            <input
              v-model.trim="form.email"
              type="email"
              placeholder="选填，建议填写"
            />
          </label>
          <label>
            <span>初始密码</span>
            <input
              v-model.trim="form.password"
              type="text"
              placeholder="留空则使用系统默认密码"
            />
          </label>
          <label>
            <span>生日</span>
            <input v-model="form.birthday" type="date" />
          </label>
          <p v-if="formError" class="form-error">{{ formError }}</p>

          <div class="dialog__actions">
            <button
              type="button"
              class="button button--ghost"
              @click="closeCreateDialog"
            >
              取消
            </button>
            <button type="submit" class="button" :disabled="creating">
              {{ creating ? "创建中..." : "确认创建" }}
            </button>
          </div>
        </form>
      </div>
    </div>
  </section>
</template>

<script lang="ts">
import {
  computed,
  defineComponent,
  onMounted,
  reactive,
  ref,
  watch,
} from "vue";
import { useStore } from "vuex";
import { useRouter } from "vue-router";
import {
  isSuperAdminPortalRole,
  resolveRoleName,
} from "@/core/auth/utils/roleUtils";
import { storeKey } from "@/app/store";
import AppPager from "@/shared/components/AppPager.vue";
import { UserRow } from "../../api/types";
import { superAdminApi } from "../../api/superAdminApi";

type RoleFilterKey = "all" | "normal" | "medical" | "admin";

const roleOptions: Array<{
  key: RoleFilterKey;
  label: string;
  role: string;
}> = [
  { key: "all", label: "全部", role: "all" },
  { key: "normal", label: "普通用户", role: "普通用户" },
  { key: "medical", label: "医护人员", role: "medical" },
  { key: "admin", label: "管理员", role: "admin" },
];

export default defineComponent({
  name: "SuperAdminUsers",
  components: { AppPager },
  setup() {
    const getErrorDetails = (error: unknown) => {
      const responseError = error as {
        response?: {
          data?: {
            error?: {
              details?: string;
            };
          };
        };
      };

      return responseError.response?.data?.error?.details;
    };

    const store = useStore(storeKey);
    const router = useRouter();
    const users = ref<UserRow[]>([]);
    const total = ref(0);
    const page = ref(1);
    const pageSize = 10;
    const loading = ref(false);
    const listError = ref("");
    const keywordInput = ref("");
    const keyword = ref("");
    const activeRole = ref<RoleFilterKey>("all");
    const roleCounts = reactive<Record<RoleFilterKey, number>>({
      all: 0,
      normal: 0,
      medical: 0,
      admin: 0,
    });
    const showCreateDialog = ref(false);
    const creating = ref(false);
    const formError = ref("");
    const requestId = ref(0);
    const form = reactive({
      name: "",
      phone: "",
      email: "",
      password: "",
      birthday: "",
    });

    const selectedRoleOption = computed(
      () =>
        roleOptions.find((option) => option.key === activeRole.value) ||
        roleOptions[0]
    );

    const activeRoleLabel = computed(() => selectedRoleOption.value.label);

    const totalPages = computed(() =>
      Math.max(1, Math.ceil(total.value / pageSize))
    );

    const placeholderRows = computed(() => {
      if (loading.value || users.value.length === 0) {
        return 0;
      }

      return Math.max(0, pageSize - users.value.length);
    });

    const formatRole = (user: UserRow) =>
      resolveRoleName(user.type_name, user.type_id) || "未知角色";

    const formatStatus = (user: UserRow) => {
      const roleName = formatRole(user);
      if (roleName !== "医生" && roleName !== "护士") {
        return "不适用";
      }

      return user.status === "online" ? "在线" : "离线";
    };

    const roleTone = (user: UserRow) => {
      const roleName = formatRole(user);
      if (roleName === "医生" || roleName === "护士") {
        return "role-pill--medical";
      }

      if (isSuperAdminPortalRole(roleName)) {
        return "role-pill--admin";
      }

      return "role-pill--normal";
    };

    const fetchRoleCounts = async () => {
      const results = await Promise.all(
        roleOptions.map((option) =>
          superAdminApi.searchUsers({
            keyword: keyword.value,
            role: option.role,
            page: 1,
            pageSize: 1,
          })
        )
      );

      roleOptions.forEach((option, index) => {
        roleCounts[option.key] = results[index].total;
      });
    };

    const loadUsers = async () => {
      const currentRequestId = requestId.value + 1;
      requestId.value = currentRequestId;
      loading.value = true;
      listError.value = "";

      try {
        const [listResult] = await Promise.all([
          superAdminApi.searchUsers({
            keyword: keyword.value,
            role: selectedRoleOption.value.role,
            page: page.value,
            pageSize,
          }),
          fetchRoleCounts(),
        ]);

        if (currentRequestId !== requestId.value) {
          return;
        }

        users.value = listResult.items;
        total.value = listResult.total;

        const nextTotalPages = Math.max(
          1,
          Math.ceil(listResult.total / pageSize)
        );
        if (page.value > nextTotalPages) {
          page.value = nextTotalPages;
        }
      } catch (error: unknown) {
        if (currentRequestId !== requestId.value) {
          return;
        }

        users.value = [];
        total.value = 0;
        listError.value = getErrorDetails(error) || "用户列表加载失败，请重试";
      } finally {
        if (currentRequestId === requestId.value) {
          loading.value = false;
        }
      }
    };

    watch(page, () => {
      void loadUsers();
    });

    const applySearch = () => {
      keyword.value = keywordInput.value.trim();
      if (page.value === 1) {
        void loadUsers();
        return;
      }

      page.value = 1;
    };

    const clearSearch = () => {
      keywordInput.value = "";
      keyword.value = "";
      if (page.value === 1) {
        void loadUsers();
        return;
      }

      page.value = 1;
    };

    const setRole = (role: RoleFilterKey) => {
      if (activeRole.value === role) {
        return;
      }

      activeRole.value = role;
      if (page.value === 1) {
        void loadUsers();
        return;
      }

      page.value = 1;
    };

    const refreshUsers = async () => {
      await loadUsers();
    };

    const resetForm = () => {
      form.name = "";
      form.phone = "";
      form.email = "";
      form.password = "";
      form.birthday = "";
      formError.value = "";
    };

    const openCreateDialog = () => {
      resetForm();
      showCreateDialog.value = true;
    };

    const closeCreateDialog = () => {
      showCreateDialog.value = false;
    };

    const goToDetail = (userId: number) => {
      void router.push({ name: "superAdminUserDetail", params: { userId } });
    };

    const handleCreate = async () => {
      if (!form.name) {
        formError.value = "请先填写姓名";
        return;
      }

      if (!form.phone && !form.email) {
        formError.value = "手机号和邮箱至少填写一项";
        return;
      }

      creating.value = true;
      formError.value = "";

      try {
        await store.dispatch("superAdmin/createUser", {
          name: form.name,
          phone: form.phone || undefined,
          email: form.email || undefined,
          password: form.password || undefined,
          birthday: form.birthday || undefined,
        });
        closeCreateDialog();
        if (page.value === 1) {
          await loadUsers();
          return;
        }

        page.value = 1;
      } catch (error: unknown) {
        formError.value = getErrorDetails(error) || "创建失败，请稍后重试";
      } finally {
        creating.value = false;
      }
    };

    onMounted(() => {
      void loadUsers();
    });

    return {
      users,
      total,
      page,
      totalPages,
      loading,
      listError,
      keywordInput,
      keyword,
      activeRole,
      activeRoleLabel,
      roleOptions,
      roleCounts,
      placeholderRows,
      showCreateDialog,
      creating,
      form,
      formError,
      formatRole,
      formatStatus,
      roleTone,
      loadUsers,
      refreshUsers,
      applySearch,
      clearSearch,
      setRole,
      openCreateDialog,
      closeCreateDialog,
      handleCreate,
      goToDetail,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  gap: 18px;
}

.panel {
  border: 1px solid #dce7ff;
  border-radius: 16px;
  background: #ffffff;
  box-shadow: 0 18px 36px rgba(34, 64, 128, 0.06);
}

.ledger-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
  padding: 22px 24px;
}

.section-label {
  margin: 0 0 8px;
  color: #617196;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0;
}

.ledger-head h2 {
  margin: 0;
  color: #13203a;
  font-size: 26px;
  line-height: 1.15;
}

.ledger-head__copy {
  margin: 8px 0 0;
  color: #617196;
  font-size: 13px;
  line-height: 1.6;
}

.ledger-head__actions {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-shrink: 0;
}

.button {
  border: 1px solid #1f5fe8;
  border-radius: 10px;
  padding: 11px 16px;
  background: #2f6ff3;
  color: #ffffff;
  cursor: pointer;
  font-size: 12px;
  font-weight: 700;
  line-height: 1.2;
}

.button--ghost {
  border-color: #dce7ff;
  background: #edf2ff;
  color: #284181;
}

.button:disabled {
  cursor: not-allowed;
  opacity: 0.62;
}

.button:focus-visible,
.search-input:focus-visible,
.role-filter:focus-visible,
.table-action:focus-visible,
.form input:focus-visible {
  outline: 3px solid rgba(47, 111, 243, 0.18);
  outline-offset: 2px;
}

.ledger-panel {
  display: grid;
  gap: 14px;
  padding: 18px 22px 20px;
}

.toolbar {
  display: grid;
  gap: 14px;
}

.toolbar__search {
  display: flex;
  align-items: center;
  gap: 10px;
}

.search-input {
  min-width: 0;
  flex: 1;
  border: 1px solid #cfdcff;
  border-radius: 10px;
  padding: 11px 12px;
  background: #ffffff;
  color: #13203a;
  font-size: 13px;
}

.search-input::placeholder {
  color: #6c7a9f;
}

.search-hint {
  flex-shrink: 0;
  border: 1px solid #dce7ff;
  border-radius: 10px;
  padding: 9px 10px;
  background: #edf2ff;
  color: #617196;
  font-size: 12px;
  font-weight: 700;
}

.toolbar__clear {
  flex-shrink: 0;
}

.role-filters {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.role-filter {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  border: 1px solid #dce7ff;
  border-radius: 10px;
  padding: 10px 12px;
  background: #ffffff;
  color: #617196;
  cursor: pointer;
  font-size: 12px;
  font-weight: 700;
}

.role-filter strong {
  color: inherit;
  font-size: 13px;
}

.role-filter--active {
  border-color: #1f5fe8;
  background: #2f6ff3;
  color: #ffffff;
}

.ledger-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  color: #617196;
  font-size: 12px;
  font-weight: 700;
}

.ledger-meta span {
  border: 1px solid #dce7ff;
  border-radius: 999px;
  padding: 6px 10px;
  background: #f6f9ff;
}

.state-banner {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  border-radius: 10px;
  padding: 12px 14px;
  font-size: 13px;
  font-weight: 700;
}

.state-banner--error {
  border: 1px solid rgba(176, 68, 85, 0.28);
  background: rgba(176, 68, 85, 0.08);
  color: #b04455;
}

.table-shell {
  position: relative;
  overflow: hidden;
  border: 1px solid #dce7ff;
  border-radius: 12px;
  min-height: 476px;
  background: #ffffff;
}

.table-shell--loading {
  min-height: 476px;
}

.user-table {
  width: 100%;
  border-collapse: collapse;
  table-layout: fixed;
}

.user-table th,
.user-table td {
  border-bottom: 1px solid #edf2ff;
  padding: 12px 14px;
  text-align: left;
  color: #13203a;
  font-size: 13px;
  vertical-align: middle;
}

.user-table th {
  height: 42px;
  background: #f6f9ff;
  color: #617196;
  font-size: 12px;
  font-weight: 700;
}

.user-table tbody tr {
  height: 43px;
}

.user-table tbody tr:hover:not(.placeholder-row) {
  background: #f7faff;
}

.user-table th:nth-child(1),
.user-table td:nth-child(1) {
  width: 22%;
}

.user-table th:nth-child(2),
.user-table td:nth-child(2) {
  width: 13%;
}

.user-table th:nth-child(3),
.user-table td:nth-child(3) {
  width: 14%;
}

.user-table th:nth-child(4),
.user-table td:nth-child(4) {
  width: 20%;
}

.user-table th:nth-child(5),
.user-table td:nth-child(5),
.user-table th:nth-child(6),
.user-table td:nth-child(6) {
  width: 11%;
}

.user-table th:nth-child(7),
.user-table td:nth-child(7) {
  width: 9%;
}

.user-cell {
  display: grid;
  gap: 3px;
  min-width: 0;
}

.user-cell strong,
.user-cell span,
.user-table td {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.user-cell strong {
  font-size: 13px;
}

.user-cell span {
  color: #6c7a9f;
  font-size: 12px;
}

.role-pill,
.status-pill {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 58px;
  border-radius: 999px;
  padding: 5px 9px;
  font-size: 12px;
  font-weight: 700;
  white-space: nowrap;
}

.role-pill--normal {
  background: #edf2ff;
  color: #284181;
}

.role-pill--medical {
  background: #e8f7ff;
  color: #2863da;
}

.role-pill--admin {
  background: rgba(155, 104, 23, 0.12);
  color: #9b6817;
}

.status-pill--online {
  background: rgba(36, 123, 98, 0.1);
  color: #247b62;
}

.status-pill--offline {
  background: #edf2ff;
  color: #617196;
}

.table-action {
  border: 1px solid #dce7ff;
  border-radius: 10px;
  padding: 8px 11px;
  background: #ffffff;
  color: #284181;
  cursor: pointer;
  font-size: 12px;
  font-weight: 700;
}

.empty-state {
  display: grid;
  place-items: center;
  gap: 6px;
  min-height: 300px;
  color: #617196;
  text-align: center;
}

.empty-state strong {
  color: #13203a;
  font-size: 15px;
}

.placeholder-row td {
  height: 43px;
  background: #ffffff;
}

.loading-layer {
  position: absolute;
  inset: 42px 0 0;
  display: grid;
  place-items: center;
  background: rgba(255, 255, 255, 0.72);
  color: #2f6ff3;
  font-size: 13px;
  font-weight: 700;
}

:deep(.pager) {
  justify-content: flex-end;
  padding-top: 4px;
  color: #617196;
}

:deep(.pager-button),
:deep(.pager-button--ghost) {
  border-color: #dce7ff;
  border-radius: 10px;
  background: #2f6ff3;
  box-shadow: none;
  color: #ffffff;
}

:deep(.pager-button--ghost) {
  background: #edf2ff;
  color: #284181;
}

:deep(.pager-jump input) {
  border-color: #cfdcff;
  border-radius: 10px;
  background: #ffffff;
  color: #13203a;
}

.dialog-backdrop {
  position: fixed;
  inset: 0;
  z-index: 30;
  display: grid;
  place-items: center;
  padding: 20px;
  background: rgba(24, 35, 30, 0.28);
}

.dialog {
  width: min(560px, 100%);
  border: 1px solid #dce7ff;
  border-radius: 16px;
  background: #ffffff;
  box-shadow: 0 24px 60px rgba(25, 42, 92, 0.18);
  padding: 22px;
}

.dialog__head,
.dialog__actions {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
}

.dialog__head h3 {
  margin: 0;
  color: #13203a;
  font-size: 20px;
}

.dialog__head span {
  display: block;
  margin-top: 6px;
  color: #617196;
  font-size: 13px;
}

.form {
  display: grid;
  gap: 14px;
  margin-top: 18px;
}

.form label {
  display: grid;
  gap: 8px;
  color: #13203a;
  font-size: 12px;
  font-weight: 700;
}

.form input {
  border: 1px solid #cfdcff;
  border-radius: 10px;
  padding: 11px 12px;
  background: #ffffff;
  color: #13203a;
  font-size: 13px;
}

.form input::placeholder {
  color: #6c7a9f;
}

.form-error {
  margin: 0;
  color: #b04455;
  font-size: 13px;
  font-weight: 700;
}

.dialog__actions {
  align-items: center;
  justify-content: flex-end;
  margin-top: 4px;
}

@media (max-width: 1080px) {
  .ledger-head {
    flex-direction: column;
  }

  .ledger-head__actions {
    width: 100%;
    justify-content: flex-start;
  }

  .table-shell {
    overflow-x: auto;
  }

  .user-table {
    min-width: 920px;
  }
}

@media (max-width: 720px) {
  .ledger-panel,
  .ledger-head {
    padding: 16px;
  }

  .toolbar__search {
    align-items: stretch;
    flex-direction: column;
  }

  .search-hint,
  .toolbar__clear {
    width: 100%;
    box-sizing: border-box;
    text-align: center;
  }

  .role-filter {
    flex: 1 1 calc(50% - 8px);
    justify-content: space-between;
  }

  .dialog__head {
    flex-direction: column;
  }
}
</style>
