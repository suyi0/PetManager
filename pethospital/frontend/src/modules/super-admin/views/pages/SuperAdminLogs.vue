<template>
  <section class="logs-page">
    <header class="logs-head panel">
      <div>
        <p class="section-label">日志审计</p>
        <h3>日志台账</h3>
        <p>
          按时间、结果、模块、角色和关键词定位审计记录，右侧查看完整上下文。
        </p>
      </div>
      <button class="button button--ghost" type="button" @click="resetFilters">
        重置筛选
      </button>
    </header>

    <section class="panel audit-panel">
      <div class="summary-strip">
        <article>
          <span>当前筛选</span>
          <strong>{{ total }}</strong>
        </article>
        <article>
          <span>用户类日志</span>
          <strong>{{ userLogCount }}</strong>
        </article>
        <article>
          <span>系统日志</span>
          <strong>{{ systemLogCount }}</strong>
        </article>
        <article>
          <span>当前页</span>
          <strong>{{ page }} / {{ totalPages }}</strong>
        </article>
      </div>

      <div class="filters">
        <div class="major-tabs" aria-label="日志大类">
          <button
            v-for="tab in majorTabs"
            :key="tab.key"
            type="button"
            class="major-tab"
            :class="{ 'major-tab--active': activeMajorTab === tab.key }"
            @click="setMajorTab(tab.key)"
          >
            <strong>{{ tab.label }}</strong>
            <span>{{ tab.hint }}</span>
          </button>
        </div>

        <div class="filter-grid">
          <label>
            <span>开始日期</span>
            <input v-model="startDate" type="date" @change="applyFilters" />
          </label>
          <label>
            <span>结束日期</span>
            <input v-model="endDate" type="date" @change="applyFilters" />
          </label>
          <label>
            <span>结果状态</span>
            <select v-model="resultFilter" @change="applyFilters">
              <option
                v-for="option in resultOptions"
                :key="option.value"
                :value="option.value"
              >
                {{ option.label }}
              </option>
            </select>
          </label>
          <label>
            <span>角色</span>
            <select
              v-model="activeUserRole"
              :disabled="activeMajorTab === 'system'"
              @change="applyFilters"
            >
              <option
                v-for="option in userRoleOptions"
                :key="option.key"
                :value="option.key"
              >
                {{ option.label }}
              </option>
            </select>
          </label>
          <label>
            <span>模块</span>
            <input
              v-model.trim="moduleInput"
              type="text"
              placeholder="输入模块后按 Enter"
              @keyup.enter="applyModuleFilter"
            />
          </label>
          <label>
            <span>关键词</span>
            <input
              v-model.trim="keywordInput"
              type="text"
              placeholder="操作人 / 动作 / 描述"
              @keyup.enter="applySearch"
            />
          </label>
        </div>

        <div class="active-filters">
          <span>{{ activeMajorLabel }}</span>
          <span v-if="activeMajorTab === 'user'">
            角色：{{ activeUserRoleLabel }}
          </span>
          <span v-if="resultFilter !== 'all'">结果：{{ resultFilter }}</span>
          <span v-if="startDate || endDate">
            时间：{{ startDate || "不限" }} 至 {{ endDate || "不限" }}
          </span>
          <span v-if="moduleFilter">模块：{{ moduleFilter }}</span>
          <span v-if="keyword">关键词：{{ keyword }}</span>
          <button
            v-if="hasActiveTextFilter"
            class="text-button"
            type="button"
            @click="clearTextFilters"
          >
            清除文本筛选
          </button>
        </div>
      </div>
    </section>

    <section class="audit-layout">
      <section class="panel logs-ledger">
        <div class="ledger-headline">
          <div>
            <h4>{{ panelTitle }}</h4>
            <p>{{ panelDescription }}</p>
          </div>
          <button class="button button--ghost" type="button" @click="loadLogs">
            刷新
          </button>
        </div>

        <div v-if="listError" class="state-banner state-banner--error">
          <span>{{ listError }}</span>
          <button class="button button--ghost" type="button" @click="loadLogs">
            重试
          </button>
        </div>

        <div class="table-shell">
          <table class="logs-table">
            <thead>
              <tr>
                <th>时间</th>
                <th>类别</th>
                <th>操作人</th>
                <th>模块</th>
                <th>动作</th>
                <th>结果</th>
              </tr>
            </thead>
            <tbody>
              <tr
                v-for="item in logs"
                :key="item.id"
                class="logs-row"
                :class="{ 'logs-row--active': selectedLog?.id === item.id }"
                tabindex="0"
                @click="selectedLogId = item.id"
                @keyup.enter="selectedLogId = item.id"
              >
                <td>{{ item.time }}</td>
                <td>
                  <span
                    class="logs-badge"
                    :class="badgeClass(item.category, getUserRole(item))"
                  >
                    {{ displayCategory(item) }}
                  </span>
                </td>
                <td>{{ item.operator || "系统" }}</td>
                <td>{{ item.module || "未记录" }}</td>
                <td>{{ item.action || "未记录" }}</td>
                <td>
                  <span
                    class="logs-status"
                    :class="{
                      'logs-status--success': item.result === '成功',
                      'logs-status--warning': item.result === '警告',
                      'logs-status--failed': item.result === '失败',
                    }"
                  >
                    {{ item.result || "未知" }}
                  </span>
                </td>
              </tr>

              <tr v-if="!loading && logs.length === 0">
                <td colspan="6">
                  <div class="empty-state">
                    <strong>没有匹配的日志记录</strong>
                    <span>调整时间范围、结果状态、模块或关键词后再试。</span>
                  </div>
                </td>
              </tr>

              <tr
                v-for="index in placeholderRows"
                :key="`placeholder-${index}`"
                class="placeholder-row"
              >
                <td colspan="6"></td>
              </tr>
            </tbody>
          </table>

          <div v-if="loading" class="loading-layer">正在同步日志数据...</div>
        </div>

        <div class="logs-footer">
          <span>共 {{ total }} 条记录</span>
          <AppPager
            :page="page"
            :total-pages="totalPages"
            @update:page="page = $event"
          />
        </div>
      </section>

      <aside class="panel logs-detail">
        <div class="detail-head">
          <p class="section-label">日志详情</p>
          <h4>审计上下文</h4>
        </div>

        <div v-if="selectedLog" class="detail-content">
          <section class="detail-hero">
            <span
              class="logs-badge"
              :class="
                badgeClass(selectedLog.category, getUserRole(selectedLog))
              "
            >
              {{ displayCategory(selectedLog) }}
            </span>
            <strong>{{ selectedLog.action || "未记录动作" }}</strong>
            <p>{{ selectedLog.summary || "暂无操作摘要" }}</p>
          </section>

          <section class="detail-grid">
            <article>
              <span>操作人</span>
              <strong>{{ selectedLog.operator || "系统" }}</strong>
            </article>
            <article>
              <span>所属模块</span>
              <strong>{{ selectedLog.module || "未记录" }}</strong>
            </article>
            <article>
              <span>操作时间</span>
              <strong>{{ selectedLog.time || "未记录" }}</strong>
            </article>
            <article>
              <span>执行结果</span>
              <strong>{{ selectedLog.result || "未知" }}</strong>
            </article>
          </section>

          <section class="detail-trace">
            <article>
              <span>记录来源</span>
              <p>{{ selectedLog.source || "未记录" }}</p>
            </article>
            <article>
              <span>附加信息</span>
              <p>{{ selectedLog.details || "暂无附加信息" }}</p>
            </article>
          </section>
        </div>

        <div v-else class="empty-state empty-state--detail">
          <strong>请选择一条日志</strong>
          <span>点击左侧任意记录后，这里会展示更完整的审计细节。</span>
        </div>
      </aside>
    </section>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref, watch } from "vue";
import {
  ALL_ROLE_NAMES,
  isSuperAdminPortalRole,
} from "@/core/auth/utils/roleUtils";
import AppPager from "@/shared/components/AppPager.vue";
import {
  UserLogs,
  MajorTab,
  UserRole,
  LogCategory,
  AuditLogItem,
  LogResult,
} from "../../api/types";
import { superAdminApi } from "../../api/superAdminApi";

const isUserLog = (item: AuditLogItem): item is UserLogs =>
  item.category === "用户类";

type ResultFilter = "all" | LogResult;

export default defineComponent({
  name: "SuperAdminLogs",
  components: { AppPager },
  setup() {
    const activeMajorTab = ref<MajorTab>("user");
    const activeUserRole = ref<UserRole>("all");
    const keywordInput = ref("");
    const keyword = ref("");
    const moduleInput = ref("");
    const moduleFilter = ref("");
    const resultFilter = ref<ResultFilter>("all");
    const startDate = ref("");
    const endDate = ref("");
    const selectedLogId = ref<string>("");
    const page = ref(1);
    const pageSize = 10;
    const total = ref(0);
    const userLogCount = ref(0);
    const systemLogCount = ref(0);
    const logs = ref<AuditLogItem[]>([]);
    const loading = ref(false);
    const listError = ref("");
    const requestId = ref(0);

    const majorTabs = [
      {
        key: "user" as MajorTab,
        label: "用户类日志",
        hint: "按角色审计账号、预约、订单、资料等操作",
      },
      {
        key: "system" as MajorTab,
        label: "系统日志",
        hint: "查看任务、迁移、连接、服务调用等运行事件",
      },
    ];

    const userRoleOptions = [
      { key: "all" as UserRole, label: "全部角色" },
      ...ALL_ROLE_NAMES.map((role) => ({
        key: role as UserRole,
        label: role,
      })),
    ];

    const resultOptions: Array<{ value: ResultFilter; label: string }> = [
      { value: "all", label: "全部结果" },
      { value: "成功", label: "成功" },
      { value: "警告", label: "警告" },
      { value: "失败", label: "失败" },
    ];

    const selectedLog = computed(() =>
      logs.value.find((item) => item.id === selectedLogId.value)
    );

    const totalPages = computed(() =>
      Math.max(1, Math.ceil(total.value / pageSize))
    );

    const placeholderRows = computed(() => {
      if (loading.value || logs.value.length === 0) {
        return 0;
      }

      return Math.max(0, pageSize - logs.value.length);
    });

    const activeMajorLabel = computed(
      () =>
        majorTabs.find((tab) => tab.key === activeMajorTab.value)?.label ||
        "用户类日志"
    );

    const activeUserRoleLabel = computed(
      () =>
        userRoleOptions.find((option) => option.key === activeUserRole.value)
          ?.label || "全部角色"
    );

    const hasActiveTextFilter = computed(
      () => Boolean(keyword.value) || Boolean(moduleFilter.value)
    );

    const panelTitle = computed(() =>
      activeMajorTab.value === "system" ? "系统运行日志" : "用户操作日志"
    );

    const panelDescription = computed(() => {
      const parts = [activeMajorLabel.value];
      if (activeMajorTab.value === "user") {
        parts.push(`角色：${activeUserRoleLabel.value}`);
      }
      if (resultFilter.value !== "all") {
        parts.push(`结果：${resultFilter.value}`);
      }
      return parts.join(" / ");
    });

    const getUserRole = (item: AuditLogItem) =>
      isUserLog(item) ? item.userRole : undefined;

    const displayCategory = (item: AuditLogItem) => {
      if (item.category === "系统类") {
        return "系统日志";
      }
      return getUserRole(item) || "用户日志";
    };

    const badgeClass = (
      category: LogCategory,
      userRole?: Exclude<UserRole, "all">
    ) => {
      if (category === "系统类") {
        return "logs-badge--system";
      }
      if (userRole === "医生" || userRole === "护士") {
        return "logs-badge--doctor";
      }
      if (userRole === "仓库管理员") {
        return "logs-badge--warehouse";
      }
      if (userRole && isSuperAdminPortalRole(userRole)) {
        return "logs-badge--super";
      }
      return "logs-badge--user";
    };

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

    const loadLogs = async () => {
      const currentRequestId = requestId.value + 1;
      requestId.value = currentRequestId;
      loading.value = true;
      listError.value = "";

      try {
        const result = await superAdminApi.searchLogs({
          majorTab: activeMajorTab.value,
          role:
            activeMajorTab.value === "system" ? "all" : activeUserRole.value,
          keyword: keyword.value.trim(),
          module: moduleFilter.value.trim(),
          result: resultFilter.value,
          startDate: startDate.value,
          endDate: endDate.value,
          page: page.value,
          pageSize,
        });

        if (currentRequestId !== requestId.value) {
          return;
        }

        logs.value = result.items;
        total.value = result.total;
        selectedLogId.value = result.items[0]?.id ?? "";

        const nextTotalPages = Math.max(1, Math.ceil(result.total / pageSize));
        if (page.value > nextTotalPages) {
          page.value = nextTotalPages;
        }
      } catch (error: unknown) {
        if (currentRequestId !== requestId.value) {
          return;
        }

        listError.value = getErrorDetails(error) || "日志列表加载失败，请重试";
      } finally {
        if (currentRequestId === requestId.value) {
          loading.value = false;
        }
      }
    };

    const loadLogMetrics = async () => {
      const summary = await superAdminApi.homePageGetData();
      userLogCount.value = summary.userLogCount;
      systemLogCount.value = summary.systemLogCount;
    };

    const resetPageAndLoad = () => {
      if (page.value === 1) {
        void loadLogs();
        return;
      }

      page.value = 1;
    };

    const setMajorTab = (tab: MajorTab) => {
      if (activeMajorTab.value === tab) {
        return;
      }

      activeMajorTab.value = tab;
      if (tab === "system") {
        activeUserRole.value = "all";
      }
      resetPageAndLoad();
    };

    const applyFilters = () => {
      resetPageAndLoad();
    };

    const applySearch = () => {
      keyword.value = keywordInput.value.trim();
      resetPageAndLoad();
    };

    const applyModuleFilter = () => {
      moduleFilter.value = moduleInput.value.trim();
      resetPageAndLoad();
    };

    const clearTextFilters = () => {
      keywordInput.value = "";
      keyword.value = "";
      moduleInput.value = "";
      moduleFilter.value = "";
      resetPageAndLoad();
    };

    const resetFilters = () => {
      activeMajorTab.value = "user";
      activeUserRole.value = "all";
      keywordInput.value = "";
      keyword.value = "";
      moduleInput.value = "";
      moduleFilter.value = "";
      resultFilter.value = "all";
      startDate.value = "";
      endDate.value = "";
      resetPageAndLoad();
    };

    watch(page, () => {
      void loadLogs();
    });

    onMounted(() => {
      void loadLogMetrics();
      void loadLogs();
    });

    return {
      activeMajorTab,
      activeUserRole,
      keywordInput,
      keyword,
      moduleInput,
      moduleFilter,
      resultFilter,
      startDate,
      endDate,
      selectedLogId,
      page,
      totalPages,
      majorTabs,
      userRoleOptions,
      resultOptions,
      logs,
      total,
      userLogCount,
      systemLogCount,
      loading,
      listError,
      selectedLog,
      placeholderRows,
      activeMajorLabel,
      activeUserRoleLabel,
      hasActiveTextFilter,
      panelTitle,
      panelDescription,
      getUserRole,
      displayCategory,
      badgeClass,
      loadLogs,
      setMajorTab,
      applyFilters,
      applySearch,
      applyModuleFilter,
      clearTextFilters,
      resetFilters,
    };
  },
});
</script>

<style scoped>
.logs-page {
  display: grid;
  gap: 18px;
}

.panel {
  border: 1px solid #dce7ff;
  border-radius: 16px;
  background: #ffffff;
  box-shadow: 0 16px 32px rgba(34, 64, 128, 0.06);
}

.logs-head {
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
}

.logs-head h3,
.ledger-headline h4,
.detail-head h4 {
  margin: 0;
  color: #13203a;
}

.logs-head h3 {
  font-size: 26px;
  line-height: 1.15;
}

.logs-head p,
.ledger-headline p {
  margin: 8px 0 0;
  color: #617196;
  font-size: 13px;
  line-height: 1.6;
}

.button {
  border: 1px solid #1f5fe8;
  border-radius: 10px;
  padding: 10px 14px;
  background: #2f6ff3;
  color: #ffffff;
  cursor: pointer;
  font-size: 12px;
  font-weight: 700;
}

.button--ghost {
  border-color: #dce7ff;
  background: #edf2ff;
  color: #284181;
}

.button:focus-visible,
.major-tab:focus-visible,
input:focus-visible,
select:focus-visible,
.logs-row:focus-visible,
.text-button:focus-visible {
  outline: 3px solid rgba(47, 111, 243, 0.18);
  outline-offset: 2px;
}

.audit-panel {
  display: grid;
  gap: 16px;
  padding: 18px 22px;
}

.summary-strip {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 10px;
}

.summary-strip article {
  display: grid;
  gap: 6px;
  border: 1px solid #dce7ff;
  border-radius: 12px;
  padding: 12px 14px;
  background: #f8fbff;
}

.summary-strip span {
  color: #617196;
  font-size: 12px;
  font-weight: 700;
}

.summary-strip strong {
  color: #13203a;
  font-size: 18px;
}

.filters {
  display: grid;
  gap: 14px;
}

.major-tabs {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.major-tab {
  display: grid;
  gap: 4px;
  border: 1px solid #dce7ff;
  border-radius: 12px;
  padding: 12px 14px;
  background: #ffffff;
  color: #36507b;
  cursor: pointer;
  text-align: left;
}

.major-tab span {
  color: #6c7a9f;
  font-size: 12px;
  line-height: 1.5;
}

.major-tab--active {
  border-color: #8eb4ff;
  background: #edf4ff;
}

.filter-grid {
  display: grid;
  grid-template-columns: repeat(6, minmax(0, 1fr));
  gap: 10px;
}

.filter-grid label {
  display: grid;
  gap: 7px;
  min-width: 0;
}

.filter-grid span {
  color: #617196;
  font-size: 12px;
  font-weight: 700;
}

.filter-grid input,
.filter-grid select {
  width: 100%;
  min-width: 0;
  box-sizing: border-box;
  border: 1px solid #cfdcff;
  border-radius: 10px;
  padding: 10px 11px;
  background: #ffffff;
  color: #13203a;
  font-size: 13px;
}

.filter-grid input::placeholder {
  color: #6c7a9f;
}

.filter-grid select:disabled {
  cursor: not-allowed;
  opacity: 0.58;
}

.active-filters {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
}

.active-filters span {
  border: 1px solid #dce7ff;
  border-radius: 999px;
  padding: 6px 10px;
  background: #f6f9ff;
  color: #617196;
  font-size: 12px;
  font-weight: 700;
}

.text-button {
  border: 0;
  background: transparent;
  color: #2f6ff3;
  cursor: pointer;
  font-size: 12px;
  font-weight: 700;
}

.audit-layout {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(320px, 380px);
  gap: 18px;
}

.logs-ledger,
.logs-detail {
  padding: 18px;
  min-width: 0;
}

.ledger-headline {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
  margin-bottom: 14px;
}

.state-banner {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  border-radius: 10px;
  padding: 12px 14px;
  margin-bottom: 12px;
  font-size: 13px;
  font-weight: 700;
}

.state-banner--error {
  border: 1px solid rgba(191, 79, 89, 0.28);
  background: rgba(191, 79, 89, 0.08);
  color: #bf4f59;
}

.table-shell {
  position: relative;
  overflow: hidden;
  border: 1px solid #dce7ff;
  border-radius: 12px;
  min-height: 480px;
  background: #ffffff;
}

.logs-table {
  width: 100%;
  border-collapse: collapse;
  table-layout: fixed;
}

.logs-table th,
.logs-table td {
  border-bottom: 1px solid #edf2ff;
  padding: 12px 13px;
  text-align: left;
  color: #13203a;
  font-size: 13px;
  vertical-align: middle;
}

.logs-table th {
  height: 42px;
  background: #f6f9ff;
  color: #617196;
  font-size: 12px;
  font-weight: 700;
}

.logs-table tbody tr {
  height: 43px;
}

.logs-table th:nth-child(1),
.logs-table td:nth-child(1) {
  width: 18%;
}

.logs-table th:nth-child(2),
.logs-table td:nth-child(2) {
  width: 13%;
}

.logs-table th:nth-child(3),
.logs-table td:nth-child(3),
.logs-table th:nth-child(4),
.logs-table td:nth-child(4),
.logs-table th:nth-child(6),
.logs-table td:nth-child(6) {
  width: 13%;
}

.logs-table th:nth-child(5),
.logs-table td:nth-child(5) {
  width: 30%;
}

.logs-row {
  cursor: pointer;
}

.logs-row:hover,
.logs-row--active {
  background: #f7faff;
}

.logs-row--active td {
  box-shadow: inset 0 1px 0 #8eb4ff, inset 0 -1px 0 #8eb4ff;
}

.logs-table td {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.logs-badge,
.logs-status {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 999px;
  padding: 5px 9px;
  font-size: 12px;
  font-weight: 700;
  white-space: nowrap;
}

.logs-badge--user {
  background: #edf2ff;
  color: #4f67b5;
}

.logs-badge--doctor {
  background: #dff5eb;
  color: #1f8a61;
}

.logs-badge--warehouse {
  background: #fff1d8;
  color: #b57400;
}

.logs-badge--super {
  background: #fff0da;
  color: #b56a00;
}

.logs-badge--system {
  background: #e7f7ff;
  color: #24759e;
}

.logs-status--success {
  background: #dff5eb;
  color: #1f8a61;
}

.logs-status--warning {
  background: #fff1d8;
  color: #b57400;
}

.logs-status--failed {
  background: #fff0f1;
  color: #bf4f59;
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

.logs-footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding-top: 12px;
  color: #617196;
  font-size: 13px;
}

:deep(.pager) {
  justify-content: flex-end;
  padding-top: 0;
}

.logs-detail {
  display: grid;
  align-content: start;
  gap: 16px;
}

.detail-head h4 {
  font-size: 20px;
}

.detail-content {
  display: grid;
  gap: 14px;
  min-width: 0;
}

.detail-hero,
.detail-grid article,
.detail-trace article {
  border: 1px solid #dce7ff;
  border-radius: 12px;
  background: #f8fbff;
}

.detail-hero {
  display: grid;
  gap: 10px;
  padding: 16px;
}

.detail-hero strong {
  color: #13203a;
  font-size: 18px;
  overflow-wrap: anywhere;
}

.detail-hero p,
.detail-trace p {
  margin: 0;
  color: #617196;
  font-size: 13px;
  line-height: 1.7;
  overflow-wrap: anywhere;
}

.detail-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.detail-grid article,
.detail-trace article {
  display: grid;
  gap: 7px;
  min-width: 0;
  padding: 12px;
}

.detail-grid span,
.detail-trace span {
  color: #6c7a9f;
  font-size: 12px;
  font-weight: 700;
}

.detail-grid strong {
  color: #13203a;
  font-size: 13px;
  overflow-wrap: anywhere;
}

.detail-trace {
  display: grid;
  gap: 10px;
}

.empty-state--detail {
  min-height: 260px;
}

@media (max-width: 1180px) {
  .audit-layout {
    grid-template-columns: 1fr;
  }

  .filter-grid {
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }
}

@media (max-width: 900px) {
  .logs-head,
  .ledger-headline,
  .logs-footer {
    display: grid;
  }

  .summary-strip,
  .major-tabs,
  .filter-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .table-shell {
    overflow-x: auto;
  }

  .logs-table {
    min-width: 920px;
  }
}

@media (max-width: 640px) {
  .summary-strip,
  .major-tabs,
  .filter-grid,
  .detail-grid {
    grid-template-columns: 1fr;
  }
}
</style>
