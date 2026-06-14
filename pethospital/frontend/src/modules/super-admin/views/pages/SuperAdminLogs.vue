<template>
  <section class="logs-page">
    <section class="logs-hero">
      <div>
        <p class="logs-hero__eyebrow">Audit Center</p>
        <h3>系统日志审计台</h3>
        <span>
          先按“用户类日志 /
          系统日志”拆分浏览，再按角色快速筛选不同岗位的操作轨迹。
        </span>
      </div>

      <div class="logs-hero__metrics">
        <article>
          <strong>{{ filteredLogs.length }}</strong>
          <span>当前筛选结果</span>
        </article>
        <article>
          <strong>{{ userLogs.length }}</strong>
          <span>用户类日志</span>
        </article>
        <article>
          <strong>{{ systemLogs.length }}</strong>
          <span>系统日志</span>
        </article>
        <article>
          <strong>{{ todayCount }}</strong>
          <span>今日新增</span>
        </article>
      </div>
    </section>

    <section class="logs-shell">
      <div class="logs-stage">
        <section class="logs-panel">
          <div class="logs-panel__head">
            <div>
              <h4>{{ panelTitle }}</h4>
              <p>{{ panelDescription }}</p>
            </div>
            <button class="logs-ghost" type="button" @click="resetFilters">
              重置筛选
            </button>
          </div>

          <div class="logs-toolbar">
            <div class="logs-toolbar__major">
              <small>日志大类</small>
              <div class="logs-toolbar__major-grid">
                <button
                  v-for="tab in majorTabs"
                  :key="tab.key"
                  type="button"
                  class="logs-chip"
                  :class="{ 'logs-chip--active': activeMajorTab === tab.key }"
                  @click="activeMajorTab = tab.key"
                >
                  <strong>{{ tab.label }}</strong>
                  <span>{{ tab.hint }}</span>
                </button>
              </div>
            </div>

            <div class="logs-toolbar__filters">
              <div class="logs-filter__group">
                <small>用户类细分</small>
                <div class="logs-subchips">
                  <button
                    v-for="tab in userRoleTabs"
                    :key="tab.key"
                    type="button"
                    class="logs-subchip"
                    :class="{
                      'logs-subchip--active': activeUserRole === tab.key,
                    }"
                    :disabled="activeMajorTab !== 'user'"
                    @click="activeUserRole = tab.key"
                  >
                    {{ tab.label }}
                  </button>
                </div>
              </div>

              <div class="logs-filter__group">
                <small>检索条件</small>
                <input
                  v-model.trim="keyword"
                  type="text"
                  class="logs-search"
                  placeholder="搜索操作人 / 模块 / 动作 / 描述"
                />
              </div>
            </div>
          </div>

          <div class="logs-table">
            <header class="logs-table__head">
              <span>时间</span>
              <span>类别</span>
              <span>操作人</span>
              <span>模块</span>
              <span>动作</span>
              <span>结果</span>
            </header>

            <button
              v-for="item in pagedLogs"
              :key="item.id"
              type="button"
              class="logs-row"
              :class="{ 'logs-row--active': selectedLog?.id === item.id }"
              @click="selectedLogId = item.id"
            >
              <span>{{ item.time }}</span>
              <span>
                <em
                  class="logs-badge"
                  :class="badgeClass(item.category, getUserRole(item))"
                >
                  {{ displayCategory(item) }}
                </em>
              </span>
              <strong>{{ item.operator }}</strong>
              <span>{{ item.module }}</span>
              <span class="logs-row__action">{{ item.action }}</span>
              <span
                class="logs-status"
                :class="{
                  'logs-status--success': item.result === '成功',
                  'logs-status--warning': item.result === '警告',
                  'logs-status--failed': item.result === '失败',
                }"
              >
                {{ item.result }}
              </span>
            </button>

            <div v-if="filteredLogs.length === 0" class="logs-empty">
              <strong>当前筛选下没有日志</strong>
              <span
                >可以切换到其他角色或系统类日志查看不同范围的审计记录。</span
              >
            </div>
          </div>

          <div v-if="filteredLogs.length > 0" class="logs-footer">
            <span class="logs-footer__summary">
              共 {{ filteredLogs.length }} 条，当前第 {{ page }} /
              {{ totalPages }} 页
            </span>
            <AppPager
              :page="page"
              :total-pages="totalPages"
              @update:page="page = $event"
            />
          </div>
        </section>

        <section class="logs-detail">
          <div class="logs-detail__head">
            <small>DETAIL</small>
            <h4>日志详情</h4>
          </div>

          <div v-if="selectedLog" class="logs-detail__content">
            <article class="logs-detail__hero">
              <span
                class="logs-badge"
                :class="
                  badgeClass(selectedLog.category, getUserRole(selectedLog))
                "
              >
                {{ displayCategory(selectedLog) }}
              </span>
              <strong>{{ selectedLog.action }}</strong>
              <p>{{ selectedLog.summary }}</p>
            </article>

            <div class="logs-detail__grid">
              <article>
                <span>操作人</span>
                <strong>{{ selectedLog.operator }}</strong>
              </article>
              <article>
                <span>所属模块</span>
                <strong>{{ selectedLog.module }}</strong>
              </article>
              <article>
                <span>操作时间</span>
                <strong>{{ selectedLog.time }}</strong>
              </article>
              <article>
                <span>执行结果</span>
                <strong>{{ selectedLog.result }}</strong>
              </article>
            </div>

            <div class="logs-detail__trace">
              <article>
                <small>操作说明</small>
                <p>{{ selectedLog.summary }}</p>
              </article>
              <article>
                <small>记录来源</small>
                <p>{{ selectedLog.source }}</p>
              </article>
              <article>
                <small>附加信息</small>
                <p>{{ selectedLog.details }}</p>
              </article>
            </div>
          </div>

          <div v-else class="logs-empty logs-empty--detail">
            <strong>请选择一条日志</strong>
            <span>点击左侧任意记录后，这里会展示更完整的审计细节。</span>
          </div>
        </section>
      </div>
    </section>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref, watch } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import {
  ALL_ROLE_NAMES,
  isSuperAdminPortalRole,
} from "@/core/auth/utils/roleUtils";
import AppPager from "@/shared/components/AppPager.vue";
import {
  UserLogs,
  SystemLogs,
  MajorTab,
  UserRole,
  LogCategory,
  AuditLogItem,
} from "../../api/types";

const isUserLog = (item: AuditLogItem): item is UserLogs =>
  item.category === "用户类";

export default defineComponent({
  name: "SuperAdminLogs",
  components: { AppPager },
  setup() {
    const store = useStore(storeKey);
    /**
     * 获取日志的大类：用户类 / 系统类
     */
    const activeMajorTab = ref<MajorTab>("user");
    /**
     * 获取用户类日志时的角色筛选：全部用户 + 当前系统内已知角色
       仅在用户类日志下生效，系统类日志不区分角色
       选择“全部用户”时不过滤角色，选择其他选项时仅展示对应角色的日志
     */
    const activeUserRole = ref<UserRole>("all");
    // 搜索关键字
    const keyword = ref("");
    /**
     *
     * 选中的日志ID
     */
    const selectedLogId = ref<string>("");
    // 日志列表分页
    const page = ref(1);
    // 分页大小
    const pageSize = 5;

    /**
     * 调用API获取日志列表
     */
    const loadLogs = async () => {
      // 日志页也先读缓存，只有首次进入、过期或被标脏时才重新请求。
      await store.dispatch("superAdmin/ensureLogs", { force: true });
      selectedLogId.value = filteredLogs.value[0]?.id ?? "";
    };

    const majorTabs = [
      {
        key: "user" as MajorTab,
        label: "用户类日志",
        hint: "按全部角色快速切换，聚焦不同岗位的操作轨迹",
      },
      {
        key: "system" as MajorTab,
        label: "系统日志",
        hint: "查看迁移、鉴权、短信服务与运行时事件",
      },
    ];

    const userRoleTabs = [
      { key: "all" as UserRole, label: "全部用户" },
      ...ALL_ROLE_NAMES.map((role) => ({
        key: role as UserRole,
        label: role,
      })),
    ];

    const userLogs = computed<UserLogs[]>(
      () => store.state.superAdmin.logs.userLogs
    );
    const systemLogs = computed<SystemLogs[]>(
      () => store.state.superAdmin.logs.systemLogs
    );

    /**
     * 获取日志大类对应的角色和搜索关键词筛选后的结果列表,
      结果按照时间降序排序，最新的日志排在前面
     */
    const filteredLogs = computed<AuditLogItem[]>(() => {
      // 获取日志大类对应的基础数据
      const base =
        activeMajorTab.value === "user" ? userLogs.value : systemLogs.value;

      // 筛选角色
      const byRole =
        activeMajorTab.value === "user" && activeUserRole.value !== "all"
          ? base.filter((item) => {
              if (isUserLog(item)) {
                return item.userRole === activeUserRole.value;
              }
              return false;
            })
          : base;

      const search = keyword.value.trim().toLowerCase();
      const searched = !search
        ? byRole
        : byRole.filter((item) =>
            [
              item.operator,
              item.module,
              item.action,
              item.summary,
              item.details,
              isUserLog(item) ? item.userRole : null,
            ]
              // .filter() 方法用于从数组中筛选元素
              // (field): field is string => typeof field === "string" 是一个类型守卫函数
              // 它检查每个 field 是否为字符串类型
              // field is string 告诉TypeScript将过滤后的结果视为字符串数组
              // 最终返回原数组中所有字符串类型的元素
              .filter((field): field is string => typeof field === "string")

              // .some() 方法用于检查数组中是否有至少一个元素满足指定条件
              // 对数组中的每个 field 元素执行 field.toLowerCase().includes(search) 检查
              // 将字段转换为小写后检查是否包含搜索关键词 search
              // 如果任一字段包含搜索词则返回 true，否则返回 false
              .some((field) => field.toLowerCase().includes(search))
          );

      return [...searched].sort(
        (left, right) =>
          new Date(right.time.replace(" ", "T")).getTime() -
          new Date(left.time.replace(" ", "T")).getTime()
      );
    });

    const selectedLog = computed(() =>
      filteredLogs.value.find((item) => item.id === selectedLogId.value)
    );

    const totalPages = computed(() =>
      Math.max(1, Math.ceil(filteredLogs.value.length / pageSize))
    );

    const pagedLogs = computed(() => {
      const start = (page.value - 1) * pageSize;
      return filteredLogs.value.slice(start, start + pageSize);
    });

    const todayCount = computed(
      () =>
        userLogs.value
          .concat(systemLogs.value)
          .filter((item) =>
            item.time.startsWith(new Date().toISOString().split("T")[0])
          ).length
    );

    const panelTitle = computed(() => {
      if (activeMajorTab.value === "system") {
        return "系统运行日志";
      }

      return activeUserRole.value === "all"
        ? "全部用户操作日志"
        : `${activeUserRole.value}日志`;
    });

    const panelDescription = computed(() => {
      if (activeMajorTab.value === "system") {
        return "主要记录系统级任务、迁移回填、连接恢复、短信服务调用等运行事件。";
      }

      return activeUserRole.value === "all"
        ? "聚合查看所有用户类操作，适合从整体角度回看近期后台与业务动作。"
        : `当前仅展示${activeUserRole.value}相关的操作记录，便于按角色快速审计。`;
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
      if (userRole === "医生") {
        return "logs-badge--doctor";
      }
      if (userRole === "护士") {
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

    const resetFilters = () => {
      activeMajorTab.value = "user";
      activeUserRole.value = "all";
      keyword.value = "";
      page.value = 1;
    };

    watch(activeMajorTab, (value) => {
      if (value === "system") {
        activeUserRole.value = "all";
      }
      page.value = 1;
      selectedLogId.value = filteredLogs.value[0]?.id ?? "";
    });

    watch(activeUserRole, () => {
      page.value = 1;
      selectedLogId.value = filteredLogs.value[0]?.id ?? "";
    });

    watch(keyword, () => {
      page.value = 1;
      selectedLogId.value = filteredLogs.value[0]?.id ?? "";
    });

    watch(page, () => {
      selectedLogId.value = pagedLogs.value[0]?.id ?? "";
    });

    watch(totalPages, (value) => {
      if (page.value > value) {
        page.value = value;
      }
    });

    onMounted(() => {
      void loadLogs();
    });

    return {
      activeMajorTab,
      activeUserRole,
      keyword,
      selectedLogId,
      page,
      totalPages,
      majorTabs,
      userRoleTabs,
      userLogs,
      systemLogs,
      filteredLogs,
      pagedLogs,
      selectedLog,
      todayCount,
      panelTitle,
      panelDescription,
      getUserRole,
      displayCategory,
      badgeClass,
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

.logs-hero,
.logs-panel,
.logs-detail {
  border: 1px solid #dbe6fb;
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.9);
  box-shadow: 0 20px 40px rgba(88, 116, 170, 0.08);
}

.logs-hero {
  display: flex;
  justify-content: space-between;
  gap: 18px;
  padding: 24px 26px;
  background: linear-gradient(135deg, #eef7ff 0%, #f8fbff 45%, #f4f8ff 100%);
}

.logs-hero__eyebrow,
.logs-filter__group small,
.logs-detail__head small {
  margin: 0;
  color: #4f7cba;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.logs-hero h3,
.logs-panel__head h4,
.logs-detail__head h4 {
  margin: 0;
  color: #1c3159;
}

.logs-hero h3 {
  font-size: 32px;
}

.logs-hero span,
.logs-panel__head p {
  color: #627494;
  line-height: 1.8;
  font-size: 14px;
}

.logs-hero__metrics {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  min-width: 360px;
}

.logs-hero__metrics article {
  display: grid;
  gap: 4px;
  padding: 16px;
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.76);
}

.logs-hero__metrics strong {
  color: #214169;
  font-size: 28px;
  font-weight: 800;
}

.logs-hero__metrics span {
  color: #7082a4;
  font-size: 13px;
}

.logs-shell {
  display: block;
}

.logs-filter__group {
  display: grid;
  gap: 10px;
}

.logs-subchips {
  display: grid;
  grid-template-columns: repeat(5, minmax(0, 1fr));
  gap: 10px;
}

.logs-chip,
.logs-subchip {
  border: 1px solid #d7e4ff;
  background: linear-gradient(180deg, #ffffff, #f7faff);
  color: #36507b;
  cursor: pointer;
  transition: transform 0.2s ease, box-shadow 0.2s ease, border-color 0.2s ease;
}

.logs-chip {
  display: grid;
  gap: 3px;
  padding: 14px 16px;
  border-radius: 18px;
  text-align: left;
}

.logs-chip strong,
.logs-subchip {
  font-weight: 700;
}

.logs-chip span {
  color: #7385a6;
  font-size: 12px;
  line-height: 1.6;
}

.logs-chip--active,
.logs-subchip--active {
  border-color: rgba(77, 131, 232, 0.34);
  background: linear-gradient(135deg, #e9f4ff, #edf3ff);
  box-shadow: 0 12px 24px rgba(72, 120, 198, 0.14);
}

.logs-subchip {
  padding: 10px 12px;
  border-radius: 999px;
  font-size: 13px;
}

.logs-subchip:disabled {
  opacity: 0.42;
  cursor: not-allowed;
}

.logs-search {
  width: 100%;
  padding: 12px 14px;
  border: 1px solid #d2def9;
  border-radius: 16px;
  background: #fff;
  color: #1f3257;
  font-size: 14px;
}

.logs-stage {
  display: grid;
  grid-template-columns: minmax(0, 1.8fr) minmax(320px, 360px);
  gap: 18px;
  min-width: 0;
}

.logs-panel,
.logs-detail {
  padding: 18px;
  min-width: 0;
}

.logs-panel__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
  margin-bottom: 14px;
}

.logs-panel__head p {
  margin: 6px 0 0;
}

.logs-toolbar {
  display: grid;
  gap: 16px;
  margin-bottom: 18px;
}

.logs-toolbar__major,
.logs-toolbar__filters {
  display: grid;
  gap: 12px;
}

.logs-toolbar__major-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.logs-ghost {
  border: 0;
  padding: 10px 14px;
  border-radius: 999px;
  background: #eaf1ff;
  color: #3059a7;
  cursor: pointer;
  font-weight: 700;
}

.logs-table {
  display: grid;
  gap: 10px;
}

.logs-table__head,
.logs-row {
  display: grid;
  grid-template-columns:
    132px
    116px
    minmax(138px, 1.1fr)
    minmax(112px, 0.9fr)
    minmax(116px, 1fr)
    72px;
  gap: 10px;
  align-items: center;
}

.logs-table__head {
  padding: 0 12px 10px;
  color: #7182a3;
  font-size: 12px;
  font-weight: 700;
  border-bottom: 1px solid #e6eeff;
}

.logs-row {
  padding: 14px 12px;
  border: 1px solid #dfe8fb;
  border-radius: 18px;
  background: linear-gradient(180deg, #ffffff, #fbfdff);
  color: #314566;
  text-align: left;
  cursor: pointer;
  transition: transform 0.18s ease, box-shadow 0.18s ease,
    border-color 0.18s ease;
}

.logs-row:hover {
  transform: translateY(-1px);
  box-shadow: 0 14px 26px rgba(89, 118, 175, 0.1);
}

.logs-row--active {
  border-color: #8eb4ff;
  background: linear-gradient(180deg, #eef4ff, #f5f8ff);
  box-shadow: 0 16px 28px rgba(80, 122, 206, 0.14);
}

.logs-row strong,
.logs-row span {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: normal;
  word-break: break-word;
}

.logs-row__action {
  line-height: 1.5;
}

.logs-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  justify-self: start;
  width: fit-content;
  max-width: 100%;
  min-width: 88px;
  padding: 5px 10px;
  border-radius: 999px;
  font-size: 12px;
  font-style: normal;
  font-weight: 700;
  line-height: 1.35;
  overflow-wrap: anywhere;
  word-break: break-word;
  white-space: normal;
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

.logs-status {
  font-weight: 700;
}

.logs-status--success {
  color: #1f8a61;
}

.logs-status--warning {
  color: #b57400;
}

.logs-status--failed {
  color: #bf4f59;
}

.logs-detail {
  display: grid;
  align-content: start;
  gap: 16px;
  overflow: hidden;
}

.logs-detail__content {
  display: grid;
  gap: 16px;
  min-width: 0;
}

.logs-detail__hero {
  display: grid;
  gap: 10px;
  padding: 18px;
  border-radius: 20px;
  background: linear-gradient(180deg, #f7fbff, #ffffff);
  border: 1px solid #e0e9fb;
  min-width: 0;
}

.logs-detail__hero strong {
  color: #22375f;
  font-size: 24px;
  min-width: 0;
  overflow-wrap: anywhere;
  word-break: break-word;
}

.logs-detail__hero p,
.logs-detail__trace p {
  margin: 0;
  color: #627494;
  line-height: 1.8;
  font-size: 14px;
  min-width: 0;
  overflow-wrap: anywhere;
  word-break: break-word;
}

.logs-detail__grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.logs-detail__grid article,
.logs-detail__trace article {
  display: grid;
  gap: 8px;
  min-width: 0;
  padding: 16px;
  border-radius: 18px;
  background: rgba(248, 251, 255, 0.84);
  border: 1px solid #e2eafb;
  overflow: hidden;
}

.logs-detail__grid span,
.logs-detail__trace small {
  color: #7384a5;
  font-size: 12px;
  min-width: 0;
  overflow-wrap: anywhere;
}

.logs-detail__grid strong {
  color: #1f345b;
  min-width: 0;
  overflow-wrap: anywhere;
  word-break: break-word;
}

.logs-detail__trace {
  display: grid;
  gap: 12px;
  min-width: 0;
}

.logs-empty {
  display: grid;
  gap: 8px;
  justify-items: start;
  padding: 22px;
  border: 1px dashed #cfdbf5;
  border-radius: 18px;
  background: #fbfdff;
}

.logs-empty strong {
  color: #23385f;
}

.logs-empty span {
  color: #667895;
  line-height: 1.7;
  font-size: 14px;
}

.logs-empty--detail {
  min-height: 240px;
  align-content: center;
}

.logs-footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding-top: 30px;
}

.logs-footer__summary {
  color: #6f80a1;
  font-size: 13px;
}

@media (max-width: 1180px) {
  .logs-stage {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 900px) {
  .logs-hero {
    display: grid;
  }

  .logs-hero__metrics,
  .logs-detail__grid,
  .logs-toolbar__major-grid,
  .logs-subchips {
    grid-template-columns: 1fr 1fr;
    min-width: 0;
  }

  .logs-table__head,
  .logs-row {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .logs-footer {
    display: grid;
  }
}

@media (max-width: 640px) {
  .logs-hero__metrics,
  .logs-detail__grid,
  .logs-toolbar__major-grid,
  .logs-subchips {
    grid-template-columns: 1fr;
  }

  .logs-table__head,
  .logs-row {
    grid-template-columns: 1fr;
  }
}
</style>
