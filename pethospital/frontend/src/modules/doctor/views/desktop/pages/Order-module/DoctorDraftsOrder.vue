<template>
  <section class="panel" :class="{ 'panel--has-message': draftMessage }">
    <div class="panel-head">
      <div>
        <h3>诊单草稿</h3>
        <p>查看未提交的诊单内容，按来源和更新时间继续编辑。</p>
      </div>
      <div class="panel-head__actions">
        <div class="panel-head__pager">
          <AppPager
            :page="page"
            :total-pages="totalPages"
            @update:page="page = $event"
          />
        </div>
      </div>
    </div>

    <p
      v-if="draftMessage"
      class="draft-message"
      :class="`draft-message--${draftMessage.type}`"
    >
      {{ draftMessage.text }}
    </p>

    <div class="status-filters">
      <div class="status-filter-row">
        <button
          v-for="tab in sourceTabs"
          :key="tab.key"
          type="button"
          class="status-filter"
          :class="{ 'status-filter--active': activeSource === tab.key }"
          @click="activeSource = tab.key"
        >
          <span>{{ tab.label }}</span>
          <strong>{{ tab.count }}</strong>
        </button>
      </div>
      <button type="button" class="manage-button" @click="toggleManageMode">
        {{ isManaging ? "完成" : "管理" }}
      </button>
    </div>

    <div
      ref="tableShellRef"
      class="table-shell"
      :style="{ '--record-page-size': pageSize }"
    >
      <table>
        <thead>
          <tr>
            <th>草稿编号</th>
            <th>宠物</th>
            <th>主人</th>
            <th>主诉</th>
            <th>来源</th>
            <th>药品数</th>
            <th>预估费用</th>
            <th>剩余有效期</th>
            <th>更新时间</th>
            <th class="action-column-th">操作</th>
          </tr>
        </thead>
        <tbody>
          <tr
            v-for="item in pagedItems"
            :key="item.storageKey"
            :class="{
              'record-row--pending-delete': pendingDeleteKeys.has(
                item.storageKey
              ),
            }"
          >
            <td>{{ item.visitCode }}</td>
            <td>{{ item.petName }}</td>
            <td>{{ item.ownerName }}</td>
            <td>{{ item.symptom }}</td>
            <td>
              <span class="source-pill" :class="sourceClassName(item.source)">
                {{ item.source }}
              </span>
            </td>
            <td>{{ item.medicineCount }}</td>
            <td>¥{{ item.estimatedTotal.toFixed(2) }}</td>
            <td>{{ formatRemainingTime(item.remainingMs) }}</td>
            <td>{{ formatDate(item.updatedAt) }}</td>
            <td class="action-column-td">
              <button
                type="button"
                class="action-button"
                :class="{ 'action-button--delete': isManaging }"
                @click="
                  isManaging
                    ? togglePendingDelete(item.storageKey)
                    : openDraft(item)
                "
              >
                {{
                  isManaging
                    ? pendingDeleteKeys.has(item.storageKey)
                      ? "已标记"
                      : "删除"
                    : "继续编辑"
                }}
              </button>
            </td>
          </tr>
          <tr
            v-for="placeholder in placeholderRows"
            :key="`placeholder-${placeholder}`"
            class="placeholder-row"
          >
            <td colspan="10"></td>
          </tr>
        </tbody>
      </table>
      <div v-if="visibleItems.length === 0" class="empty-overlay">
        当前暂无可继续编辑的诊单草稿。
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
  ref,
  watch,
} from "vue";
import { useRouter } from "vue-router";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import AppPager from "@/shared/components/AppPager.vue";
import {
  DoctorOrderDraft,
  DoctorOrderDraftSummary,
} from "@/modules/doctor/utils/orderDrafts";

export default defineComponent({
  name: "DoctorDrafts",
  components: { AppPager },
  setup() {
    const router = useRouter();
    const store = useStore(storeKey);
    const activeSource = ref<"全部" | DoctorOrderDraftSummary["source"]>(
      "全部"
    );
    const drafts = ref<DoctorOrderDraftSummary[]>([]);
    const page = ref(1);
    const pageSize = ref(10);
    const tableShellRef = ref<HTMLElement | null>(null);
    const isManaging = ref(false);
    const pendingDeleteKeys = ref(new Set<string>());
    const draftMessage = ref<{
      type: "success" | "error";
      text: string;
    } | null>(null);
    let remainingTimer: number | undefined;

    const basePath = computed(() => "/doctor");

    const loadDrafts = async () => {
      drafts.value = (await store.dispatch(
        "doctor/listOrderDrafts"
      )) as DoctorOrderDraftSummary[];
    };

    const togglePendingDelete = (storageKey: string) => {
      const nextKeys = new Set(pendingDeleteKeys.value);

      if (nextKeys.has(storageKey)) {
        nextKeys.delete(storageKey);
      } else {
        nextKeys.add(storageKey);
      }

      pendingDeleteKeys.value = nextKeys;
    };

    const applyPendingDeletes = () => {
      pendingDeleteKeys.value.forEach((storageKey) => {
        void store.dispatch("doctor/removeOrderDraft", storageKey);
      });
      pendingDeleteKeys.value = new Set<string>();
      void loadDrafts();
    };

    const toggleManageMode = () => {
      if (isManaging.value) {
        applyPendingDeletes();
      }

      isManaging.value = !isManaging.value;
    };

    const visibleItems = computed(() => {
      if (activeSource.value === "全部") {
        return drafts.value;
      }

      return drafts.value.filter((item) => item.source === activeSource.value);
    });

    const totalPages = computed(() =>
      Math.max(1, Math.ceil(visibleItems.value.length / pageSize.value))
    );

    const pagedItems = computed(() => {
      const start = (page.value - 1) * pageSize.value;
      return visibleItems.value.slice(start, start + pageSize.value);
    });

    // 始终把整页占位空行补满（0 条也先铺好一整页 ledger 排布）。
    const placeholderRows = computed(() =>
      Array.from(
        { length: Math.max(0, pageSize.value - pagedItems.value.length) },
        (_, index) => index + 1
      )
    );

    const updatePageSize = () => {
      const shell = tableShellRef.value;
      if (!shell) return;
      const shellHeight = shell.clientHeight;
      if (!shellHeight) return;
      const thead = shell.querySelector("thead");
      const headerHeight = thead ? thead.getBoundingClientRect().height : 40;
      const firstRow = shell.querySelector("tbody tr:not(.placeholder-row)");
      const rowHeight = firstRow ? firstRow.getBoundingClientRect().height : 48;
      const nextPageSize = Math.max(
        4,
        Math.floor((shellHeight - headerHeight) / rowHeight)
      );

      if (Number.isFinite(nextPageSize) && nextPageSize !== pageSize.value) {
        pageSize.value = nextPageSize;
      }
    };

    let resizeObserver: ResizeObserver | null = null;

    const sourceTabs = computed(() => {
      const sources: Array<"全部" | DoctorOrderDraftSummary["source"]> = [
        "全部",
        "队列接诊",
        "临时新建",
      ];

      return sources.map((source) => ({
        key: source,
        label: source,
        count:
          source === "全部"
            ? drafts.value.length
            : drafts.value.filter((item) => item.source === source).length,
      }));
    });

    const formatDate = (timestamp: number) => {
      if (!timestamp) {
        return "暂无记录";
      }

      const date = new Date(timestamp);
      const year = date.getFullYear();
      const month = String(date.getMonth() + 1).padStart(2, "0");
      const day = String(date.getDate()).padStart(2, "0");
      const hours = String(date.getHours()).padStart(2, "0");
      const minutes = String(date.getMinutes()).padStart(2, "0");
      return `${year}-${month}-${day} ${hours}:${minutes}`;
    };

    const formatRemainingTime = (remainingMs: number) => {
      if (remainingMs <= 0) {
        return "已过期";
      }

      const totalMinutes = Math.ceil(remainingMs / 60000);
      const hours = Math.floor(totalMinutes / 60);
      const minutes = totalMinutes % 60;

      if (hours <= 0) {
        return `${minutes} 分钟`;
      }

      return minutes > 0 ? `${hours} 小时 ${minutes} 分钟` : `${hours} 小时`;
    };

    const openDraft = async (item: DoctorOrderDraftSummary) => {
      const draft = (await store.dispatch(
        "doctor/readOrderDraft",
        item.storageKey
      )) as DoctorOrderDraft | null;

      if (!draft) {
        draftMessage.value = {
          type: "error",
          text: "该诊单草稿不存在或已超过 24 小时，已刷新草稿列表。",
        };
        void syncDrafts();
        return;
      }

      const path =
        item.queueId === "default"
          ? `${basePath.value}/create-order`
          : `${basePath.value}/create-order/${item.queueId}`;

      router.push({
        path,
        query: {
          ownerId: item.ownerId,
          petId: item.petId,
          draftKey: item.storageKey,
        },
      });
    };

    const sourceClassName = (source: DoctorOrderDraftSummary["source"]) =>
      source === "队列接诊" ? "source-pill--queue" : "source-pill--manual";

    const syncDrafts = async () => {
      await loadDrafts();
      if (!isManaging.value) {
        pendingDeleteKeys.value = new Set<string>();
      }
    };

    const handleFocus = () => {
      void syncDrafts();
    };

    watch(activeSource, () => {
      page.value = 1;
    });

    watch(totalPages, (value) => {
      if (page.value > value) {
        page.value = value;
      }
    });

    onMounted(() => {
      void syncDrafts();
      remainingTimer = window.setInterval(() => {
        void syncDrafts();
      }, 60000);
      window.addEventListener("focus", handleFocus);
      void nextTick(() => {
        updatePageSize();
        if (tableShellRef.value && typeof ResizeObserver !== "undefined") {
          resizeObserver = new ResizeObserver(() => updatePageSize());
          resizeObserver.observe(tableShellRef.value);
        }
      });
    });

    onBeforeUnmount(() => {
      if (remainingTimer) {
        window.clearInterval(remainingTimer);
      }
      window.removeEventListener("focus", handleFocus);
      resizeObserver?.disconnect();
    });

    return {
      activeSource,
      page,
      totalPages,
      sourceTabs,
      visibleItems,
      pagedItems,
      placeholderRows,
      pageSize,
      tableShellRef,
      isManaging,
      draftMessage,
      pendingDeleteKeys,
      toggleManageMode,
      togglePendingDelete,
      formatDate,
      formatRemainingTime,
      openDraft,
      sourceClassName,
    };
  },
});
</script>

<style scoped>
.panel {
  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr);
  gap: 16px;
  height: var(--doctor-page-card-height, 860px);
  border: 1px solid rgba(148, 163, 184, 0.24);
  border-radius: 14px;
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.96), #f8fafc);
  padding: 18px 18px 22px;
  box-shadow: 0 20px 38px rgba(16, 24, 40, 0.06);
  box-sizing: border-box;
  overflow: hidden;
}

.panel--has-message {
  grid-template-rows: auto auto auto minmax(0, 1fr);
}

.panel-head {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 20px;
}

.panel-head h3,
.panel-head p {
  margin: 0;
}

.panel-head p {
  margin-top: 6px;
  color: #64748b;
  line-height: 1.6;
}

.panel-head__actions {
  display: grid;
  justify-items: end;
  gap: 14px;
  min-width: min(100%, 520px);
}

.panel-head__pager {
  display: flex;
  justify-content: flex-end;
}

.panel-head__pager :deep(.pager) {
  padding-top: 0;
  justify-content: flex-end;
}

.manage-button {
  min-width: 58px;
}

button {
  border: 1px solid rgba(148, 163, 184, 0.24);
  border-radius: 16px;
  padding: 11px 16px;
  background: linear-gradient(135deg, #4f46e5, #4338ca);
  color: #ffffff;
  cursor: pointer;
  box-shadow: 0 12px 24px rgba(16, 24, 40, 0.12);
}

.draft-message {
  margin: 0;
  border-radius: 10px;
  padding: 10px 12px;
  font-size: 13px;
  font-weight: 700;
}

.draft-message--success {
  border: 1px solid rgba(22, 163, 74, 0.26);
  background: rgba(22, 163, 74, 0.08);
  color: #16a34a;
}

.draft-message--error {
  border: 1px solid rgba(220, 38, 38, 0.26);
  background: rgba(220, 38, 38, 0.08);
  color: #dc2626;
}

.status-filters {
  display: flex;
  justify-content: space-between;
}

.status-filter {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  padding: 10px 14px;
  border-radius: 10px;
  border: 1px solid rgba(148, 163, 184, 0.3);
  background: rgba(255, 255, 255, 0.72);
  color: #64748b;
  box-shadow: none;
}

.status-filter-row {
  display: flex;
  gap: 14px;
}

.status-filter strong {
  font-size: 13px;
}

.status-filter--active {
  border-color: rgba(16, 24, 40, 0.38);
  background: linear-gradient(135deg, #f8fafc, #eef2ff);
  color: #0f172a;
}

.table-shell {
  position: relative;
  display: grid;
  min-height: 0;
  height: 100%;
  overflow: hidden;
}

.table-shell::after {
  content: "";
  position: absolute;
  right: 0;
  bottom: 0;
  left: 0;
  height: 1px;
  background: rgba(148, 163, 184, 0.86);
  pointer-events: none;
}

table {
  width: 100%;
  border-collapse: collapse;
  table-layout: fixed;
}

th,
td {
  height: 48px;
  padding: 0 14px;
  border-bottom: 1px solid #e5e7eb;
  text-align: left;
  font-size: 13px;
}

th {
  height: 40px;
  background: #f8fafc;
  color: #64748b;
  font-size: 12px;
  font-weight: 700;
}

td {
  color: #0f172a;
}

.source-pill {
  display: inline-flex;
  padding: 7px 11px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.source-pill--queue {
  background: #eef2ff;
  color: #0f172a;
}

.source-pill--manual {
  background: #fef2f2;
  color: #64748b;
}

.action-column-th {
  width: 110px;
  padding-right: 24px;
  text-align: right;
}

.action-column-td {
  width: 110px;
  text-align: right;
}

.action-button {
  padding: 7px 12px;
  border-radius: 12px;
  background: linear-gradient(135deg, #eef2ff, #e0e7ff);
  color: #1e293b;
  box-shadow: none;
}

.action-button--delete {
  background: linear-gradient(135deg, #fef2f2, #fef2f2);
  color: #64748b;
}

.record-row--pending-delete {
  background: rgba(254, 242, 242, 0.72);
}

.placeholder-row td {
  height: 48px;
  background: #ffffff;
}

.empty-overlay {
  position: absolute;
  inset: 48px 0 0;
  display: grid;
  place-items: center;
  color: #64748b;
  font-size: 13px;
  pointer-events: none;
}
</style>
