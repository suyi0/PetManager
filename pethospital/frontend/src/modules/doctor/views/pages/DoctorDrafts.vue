<template>
  <section class="panel">
    <div class="panel-head">
      <div>
        <h3>诊单草稿</h3>
        <p>查看未提交的诊单内容，按来源和更新时间继续编辑。</p>
      </div>
      <div class="panel-head__actions">
        <AppPager
          :page="page"
          :total-pages="totalPages"
          @update:page="page = $event"
        />
        <button type="button" @click="toggleManageMode">
          {{ isManaging ? "完成" : "管理" }}
        </button>
      </div>
    </div>

    <div class="status-filters">
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

    <div class="table-shell">
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
            <td>{{ formatDate(item.updatedAt) }}</td>
            <td class="action-column-td">
              <button
                type="button"
                class="action-button"
                :class="{ 'action-button--delete': isManaging }"
                @click="
                  isManaging
                    ? togglePendingDelete(item.storageKey)
                    : openDraft(item.queueId)
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
            <td colspan="9"></td>
          </tr>
          <tr v-if="visibleItems.length === 0">
            <td colspan="9" class="empty-cell">
              当前暂无可继续编辑的诊单草稿。
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>
</template>

<script lang="ts">
import {
  computed,
  defineComponent,
  onBeforeUnmount,
  onMounted,
  ref,
  watch,
} from "vue";
import { useRoute, useRouter } from "vue-router";
import AppPager from "../../../../components/AppPager.vue";
import {
  DoctorOrderDraftSummary,
  listDoctorOrderDrafts,
  removeDoctorOrderDraft,
} from "../../utils/orderDrafts";

export default defineComponent({
  name: "DoctorDrafts",
  components: { AppPager },
  setup() {
    const route = useRoute();
    const router = useRouter();
    const activeSource = ref<"全部" | DoctorOrderDraftSummary["source"]>(
      "全部"
    );
    const drafts = ref<DoctorOrderDraftSummary[]>([]);
    const page = ref(1);
    const pageSize = 10;
    const isManaging = ref(false);
    const pendingDeleteKeys = ref(new Set<string>());

    const basePath = computed(() =>
      route.path.startsWith("/preview/doctor") ? "/preview/doctor" : "/doctor"
    );

    const loadDrafts = () => {
      drafts.value = listDoctorOrderDrafts();
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
        removeDoctorOrderDraft(storageKey);
      });
      pendingDeleteKeys.value = new Set<string>();
      loadDrafts();
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
      Math.max(1, Math.ceil(visibleItems.value.length / pageSize))
    );

    const pagedItems = computed(() => {
      const start = (page.value - 1) * pageSize;
      return visibleItems.value.slice(start, start + pageSize);
    });

    const placeholderRows = computed(() =>
      pagedItems.value.length === 0
        ? []
        : Array.from(
            { length: Math.max(0, pageSize - pagedItems.value.length) },
            (_, index) => index + 1
          )
    );

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

    const openDraft = (queueId: string) => {
      const path =
        queueId === "default"
          ? `${basePath.value}/create-order`
          : `${basePath.value}/create-order/${queueId}`;

      router.push(path);
    };

    const sourceClassName = (source: DoctorOrderDraftSummary["source"]) =>
      source === "队列接诊" ? "source-pill--queue" : "source-pill--manual";

    const syncDrafts = () => {
      loadDrafts();
      if (!isManaging.value) {
        pendingDeleteKeys.value = new Set<string>();
      }
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
      syncDrafts();
      window.addEventListener("focus", syncDrafts);
      window.addEventListener("storage", syncDrafts);
    });

    onBeforeUnmount(() => {
      window.removeEventListener("focus", syncDrafts);
      window.removeEventListener("storage", syncDrafts);
    });

    return {
      activeSource,
      page,
      totalPages,
      sourceTabs,
      visibleItems,
      pagedItems,
      placeholderRows,
      isManaging,
      pendingDeleteKeys,
      toggleManageMode,
      togglePendingDelete,
      formatDate,
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
  border: 1px solid rgba(157, 188, 178, 0.24);
  border-radius: 28px;
  background: linear-gradient(180deg, rgba(255, 253, 248, 0.96), #f6fbf8);
  padding: 22px;
  max-height: min(100vh - 140px, 780px);
  box-shadow: 0 20px 38px rgba(49, 82, 77, 0.06);
  box-sizing: border-box;
  overflow: hidden;
}

.panel-head {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 12px;
  margin-bottom: 14px;
}

.panel-head h3,
.panel-head p {
  margin: 0;
}

.panel-head p {
  margin-top: 6px;
  color: #67807b;
  line-height: 1.6;
}

.panel-head__actions {
  display: flex;
  align-items: center;
  gap: 12px;
}

button {
  border: 1px solid rgba(144, 175, 166, 0.24);
  border-radius: 16px;
  padding: 11px 16px;
  background: linear-gradient(135deg, #29565a, #7d5348);
  color: #fffdfb;
  cursor: pointer;
  box-shadow: 0 12px 24px rgba(49, 82, 87, 0.12);
}

.status-filters {
  display: flex;
  gap: 12px;
  margin-bottom: 18px;
}

.status-filter {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  padding: 10px 14px;
  border-radius: 18px;
  border: 1px solid rgba(160, 186, 178, 0.3);
  background: rgba(255, 255, 255, 0.72);
  color: #4e6762;
  box-shadow: none;
}

.status-filter strong {
  font-size: 13px;
}

.status-filter--active {
  border-color: rgba(95, 140, 131, 0.38);
  background: linear-gradient(135deg, #eef8f4, #e0eee7);
  color: #21464b;
}

.table-shell {
  min-height: 0;
  overflow: hidden;
}

table {
  width: 100%;
  border-collapse: collapse;
  table-layout: fixed;
}

th,
td {
  padding: 14px 12px;
  border-bottom: 1px solid rgba(226, 236, 232, 0.92);
  text-align: left;
  font-size: 13px;
}

th {
  color: #6f8582;
  font-weight: 600;
}

td {
  color: #19383b;
}

.source-pill {
  display: inline-flex;
  padding: 7px 11px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.source-pill--queue {
  background: #ecf8f3;
  color: #275b56;
}

.source-pill--manual {
  background: #f8efe7;
  color: #7a5841;
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
  background: linear-gradient(135deg, #ecf8f3, #d7ebe4);
  color: #214f4b;
  box-shadow: none;
}

.action-button--delete {
  background: linear-gradient(135deg, #fbeee8, #f5d8ca);
  color: #7a3f2f;
}

.record-row--pending-delete {
  background: rgba(250, 235, 228, 0.72);
}

.placeholder-row td {
  height: 52px;
  background: rgba(255, 255, 255, 0.32);
}

.empty-cell {
  height: 120px;
  color: #708682;
  text-align: center;
}
</style>
