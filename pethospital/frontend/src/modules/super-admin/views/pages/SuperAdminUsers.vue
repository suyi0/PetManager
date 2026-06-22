<template>
  <section class="page">
    <section class="panel ledger-panel">
      <div class="lp-head">
        <div class="lp-title">
          用户台账 <span class="count-pill">共 {{ total }} 人</span>
        </div>
        <div class="lp-actions">
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
      </div>

      <div class="toolbar">
        <div class="toolbar__row">
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
        </div>

        <div class="toolbar_bottom">
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

          <AppPager
            :page="page"
            :total-pages="totalPages"
            @update:page="page = $event"
          />
        </div>
      </div>

      <div v-if="listError" class="state-banner state-banner--error">
        <span>{{ listError }}</span>
        <button type="button" class="button button--ghost" @click="loadUsers">
          重试
        </button>
      </div>

      <div
        ref="tableShellRef"
        class="table-shell"
        :class="{ 'table-shell--loading': loading }"
      >
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
            <tr v-for="item in users" :key="item.id" class="user-row">
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
  nextTick,
  onBeforeUnmount,
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
    const pageSize = ref(10);
    const tableShellRef = ref<HTMLElement | null>(null);
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
    // 计数缓存控制：记录上次统计所用 keyword；数据增删后置脏，强制下次重新统计。
    const lastCountsKeyword = ref<string | null>(null);
    const countsDirty = ref(true);
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
      Math.max(1, Math.ceil(total.value / pageSize.value))
    );

    const placeholderRows = computed(() => {
      if (loading.value || users.value.length === 0) {
        return 0;
      }

      return Math.max(0, pageSize.value - users.value.length);
    });

    // 按表格可用高度反推每页行数，使列表恰好填满、页面不滚动。
    const applyPageSize = () => {
      const shell = tableShellRef.value;
      if (!shell) return false;
      const shellH = shell.clientHeight;
      if (!shellH) return false;
      const thead = shell.querySelector("thead");
      const headerH = thead ? thead.getBoundingClientRect().height : 40;
      // 只量真实数据行，避免空态行（撑满整高）污染行高测量。
      const firstRow = shell.querySelector("tbody tr.user-row");
      const rowH = firstRow ? firstRow.getBoundingClientRect().height : 48;
      const next = Math.max(4, Math.floor((shellH - headerH) / rowH));
      if (Number.isFinite(next) && next !== pageSize.value) {
        pageSize.value = next;
        return true;
      }
      return false;
    };

    const onResize = () => {
      if (applyPageSize()) void loadUsers();
    };

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

    const loadUsers = async () => {
      const currentRequestId = requestId.value + 1;
      requestId.value = currentRequestId;
      loading.value = true;
      listError.value = "";

      // 角色计数只跟 keyword/数据变化有关，与翻页、切角色标签无关；
      // 只有计数可能变化时才让后端统计，其余请求复用上次缓存。
      const needCounts =
        countsDirty.value || lastCountsKeyword.value !== keyword.value;

      try {
        const listResult = await superAdminApi.searchUsers({
          keyword: keyword.value,
          role: selectedRoleOption.value.role,
          page: page.value,
          pageSize: pageSize.value,
          includeCounts: needCounts,
        });

        if (currentRequestId !== requestId.value) {
          return;
        }

        users.value = listResult.items;
        total.value = listResult.total;
        if (listResult.roleCounts) {
          roleCounts.all = listResult.roleCounts.all;
          roleCounts.normal = listResult.roleCounts.normal;
          roleCounts.medical = listResult.roleCounts.medical;
          roleCounts.admin = listResult.roleCounts.admin;
          lastCountsKeyword.value = keyword.value;
          countsDirty.value = false;
        }

        const nextTotalPages = Math.max(
          1,
          Math.ceil(listResult.total / pageSize.value)
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
        // 新增用户改变了各角色计数，置脏强制下次重新统计。
        countsDirty.value = true;
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

    let resizeObserver: ResizeObserver | null = null;

    onMounted(() => {
      // 首次加载由 ResizeObserver 的首帧驱动：此时表格高度已完成布局，applyPageSize
      // 能算出正确的每页行数，保证「进入页面即拉到并填满第一页」，且只发一次请求。
      // 后续真实尺寸变化才在行数变化时重新拉取。
      if (tableShellRef.value && typeof ResizeObserver !== "undefined") {
        let initialized = false;
        resizeObserver = new ResizeObserver(() => {
          if (!initialized) {
            initialized = true;
            applyPageSize();
            void loadUsers();
          } else {
            onResize();
          }
        });
        resizeObserver.observe(tableShellRef.value);
      } else {
        // 不支持 ResizeObserver 时退化为下一帧测量后加载。
        void nextTick(async () => {
          applyPageSize();
          await loadUsers();
        });
      }
    });

    onBeforeUnmount(() => {
      resizeObserver?.disconnect();
    });

    return {
      users,
      total,
      page,
      totalPages,
      tableShellRef,
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
  grid-template-rows: minmax(0, 1fr);
  height: 100%;
  min-height: 0;
}

.panel {
  border: 1px solid #e7e9ee;
  border-radius: 16px;
  background: #ffffff;
  box-shadow: 0 1px 2px rgba(16, 24, 40, 0.04);
}

.lp-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;
}

.lp-title {
  font-size: 16px;
  font-weight: 700;
  color: #0f172a;
  display: flex;
  align-items: center;
  gap: 8px;
}

.count-pill {
  font-size: 12px;
  font-weight: 600;
  color: #64748b;
  background: #f1f2f5;
  border-radius: 999px;
  padding: 2px 9px;
}

.lp-actions {
  display: flex;
  align-items: center;
  gap: 10px;
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
  color: #64748b;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0;
}

.ledger-head h2 {
  margin: 0;
  color: #0f172a;
  font-size: 26px;
  line-height: 1.15;
}

.ledger-head__copy {
  margin: 8px 0 0;
  color: #64748b;
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
  border: 1px solid #4338ca;
  border-radius: 10px;
  padding: 11px 16px;
  background: #4f46e5;
  color: #ffffff;
  cursor: pointer;
  font-size: 12px;
  font-weight: 700;
  line-height: 1.2;
}

.button--ghost {
  border-color: #e7e9ee;
  background: #eef2ff;
  color: #3730a3;
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
  outline: 3px solid rgba(79, 70, 229, 0.18);
  outline-offset: 2px;
}

.ledger-panel {
  display: flex;
  flex-direction: column;
  gap: 14px;
  padding: 18px 22px 20px;
  min-height: 0;
  height: 100%;
  box-sizing: border-box;
}

.toolbar {
  display: grid;
  gap: 12px;
}

.toolbar__row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
}

.toolbar__search {
  display: flex;
  align-items: center;
  gap: 10px;
  flex: 0 1 420px;
  max-width: 420px;
}

.search-input {
  min-width: 0;
  flex: 1;
  height: 36px;
  border: 1px solid #e0e7ff;
  border-radius: 9px;
  padding: 0 12px;
  background: #ffffff;
  color: #0f172a;
  font-size: 13px;
}

.search-input::placeholder {
  color: #64748b;
}

.search-hint {
  flex-shrink: 0;
  display: inline-flex;
  align-items: center;
  height: 36px;
  border: 1px solid #e7e9ee;
  border-radius: 9px;
  padding: 0 10px;
  background: #eef2ff;
  color: #64748b;
  font-size: 12px;
  font-weight: 700;
}

.toolbar__clear {
  flex-shrink: 0;
}

.toolbar_bottom {
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
}

.role-filters {
  display: flex;
  flex-wrap: wrap;
  gap: 15px;
}

.role-filter {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  border: 1px solid #e7e9ee;
  border-radius: 10px;
  padding: 10px 12px;
  background: #ffffff;
  color: #64748b;
  cursor: pointer;
  font-size: 12px;
  font-weight: 700;
}

.role-filter strong {
  color: inherit;
  font-size: 13px;
}

.role-filter--active {
  border-color: #4338ca;
  background: #4f46e5;
  color: #ffffff;
}

.ledger-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  color: #64748b;
  font-size: 12px;
  font-weight: 700;
}

.ledger-meta span {
  border: 1px solid #e7e9ee;
  border-radius: 999px;
  padding: 6px 10px;
  background: #f8fafc;
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
  border: 1px solid rgba(220, 38, 38, 0.28);
  background: rgba(220, 38, 38, 0.08);
  color: #dc2626;
}

.table-shell {
  position: relative;
  overflow: hidden;
  border: 1px solid #e7e9ee;
  border-radius: 12px;
  flex: 1;
  min-height: 0;
  background: #ffffff;
}

.user-table {
  width: 100%;
  border-collapse: collapse;
  table-layout: fixed;
}

.user-table th,
.user-table td {
  border-bottom: 1px solid #e5e7eb;
  padding: 0 14px;
  text-align: left;
  color: #0f172a;
  font-size: 13px;
  vertical-align: middle;
}

.user-table th {
  height: 40px;
  background: #f8fafc;
  color: #64748b;
  font-size: 12px;
  font-weight: 700;
}

.user-table tbody tr {
  height: 48px;
}

.user-table tbody tr:hover:not(.placeholder-row) {
  background: #f8fafc;
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
  color: #64748b;
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
  background: #eef2ff;
  color: #3730a3;
}

.role-pill--medical {
  background: #eef2ff;
  color: #2863da;
}

.role-pill--admin {
  background: rgba(155, 104, 23, 0.12);
  color: #b45309;
}

.status-pill--online {
  background: rgba(16, 185, 129, 0.1);
  color: #247b62;
}

.status-pill--offline {
  background: #eef2ff;
  color: #64748b;
}

.table-action {
  border: 1px solid #e7e9ee;
  border-radius: 10px;
  padding: 8px 11px;
  background: #ffffff;
  color: #3730a3;
  cursor: pointer;
  font-size: 12px;
  font-weight: 700;
}

.empty-state {
  display: grid;
  place-items: center;
  gap: 6px;
  min-height: 300px;
  color: #64748b;
  text-align: center;
}

.empty-state strong {
  color: #0f172a;
  font-size: 15px;
}

.placeholder-row td {
  height: 48px;
  background: #ffffff;
}

.loading-layer {
  position: absolute;
  inset: 42px 0 0;
  display: grid;
  place-items: center;
  background: rgba(255, 255, 255, 0.72);
  color: #4f46e5;
  font-size: 13px;
  font-weight: 700;
}

:deep(.pager) {
  justify-content: flex-end;
  padding-top: 4px;
  color: #64748b;
}

:deep(.pager-button),
:deep(.pager-button--ghost) {
  border-color: #e7e9ee;
  border-radius: 10px;
  background: #4f46e5;
  box-shadow: none;
  color: #ffffff;
}

:deep(.pager-button--ghost) {
  background: #eef2ff;
  color: #3730a3;
}

:deep(.pager-jump input) {
  border-color: #e0e7ff;
  border-radius: 10px;
  background: #ffffff;
  color: #0f172a;
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
  border: 1px solid #e7e9ee;
  border-radius: 16px;
  background: #ffffff;
  box-shadow: 0 24px 60px rgba(16, 24, 40, 0.18);
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
  color: #0f172a;
  font-size: 20px;
}

.dialog__head span {
  display: block;
  margin-top: 6px;
  color: #64748b;
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
  color: #0f172a;
  font-size: 12px;
  font-weight: 700;
}

.form input {
  border: 1px solid #e0e7ff;
  border-radius: 10px;
  padding: 11px 12px;
  background: #ffffff;
  color: #0f172a;
  font-size: 13px;
}

.form input::placeholder {
  color: #64748b;
}

.form-error {
  margin: 0;
  color: #dc2626;
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
