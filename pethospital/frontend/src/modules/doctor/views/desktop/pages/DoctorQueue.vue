<template>
  <section class="page">
    <div class="panel">
      <div class="panel-head">
        <div>
          <h3>待接诊队列</h3>
          <p>按照到院时间和优先等级快速排序，保持桌面式临床清单感。</p>
        </div>
        <div class="panel-head__actions">
          <AppPager
            :page="page"
            :total-pages="totalPages"
            @update:page="page = $event"
          />
          <button @click="loadQueueItems">刷新队列</button>
        </div>
      </div>

      <div
        ref="tableShellRef"
        class="table-shell"
        :style="{ '--record-page-size': pageSize }"
      >
        <table>
          <thead>
            <tr>
              <th>宠物</th>
              <th>主人</th>
              <th>主诉</th>
              <th>优先级</th>
              <th>到院时间</th>
              <th class="action-column-th">操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="item in visibleItems" :key="item.id">
              <td>{{ item.petName }}</td>
              <td>{{ item.ownerName }}</td>
              <td>{{ item.symptom }}</td>
              <td>
                <span class="tag" :class="item.level">{{ item.level }}</span>
              </td>
              <td>{{ item.arrivedAt }}</td>
              <td class="action-column-td">
                <button
                  type="button"
                  class="action-button"
                  @click="goToCreateOrder(item)"
                >
                  就医
                </button>
              </td>
            </tr>
            <tr
              v-for="placeholder in placeholderRows"
              :key="`placeholder-${placeholder}`"
              class="placeholder-row"
            >
              <td colspan="6"></td>
            </tr>
          </tbody>
        </table>
        <div v-if="visibleItems.length === 0" class="empty-overlay">
          当前页暂无待接诊记录。
        </div>
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
  calculateTotalPages,
  createPlaceholderIndexes,
  getPagedItems,
} from "@/shared/utils/pagination";
import { QueueItem } from "@/modules/doctor/api/types";

export default defineComponent({
  name: "DoctorQueue",
  components: { AppPager },
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    /**
     * 待接诊队列数据列表
     */
    const queueItems = computed<QueueItem[]>(
      () => store.state.doctor.queueItems
    );
    const page = ref(1);
    const pageSize = ref(10);
    const tableShellRef = ref<HTMLElement | null>(null);
    const basePath = computed(() => "/doctor");

    /**
     * 进入队列页时通过 RESTful 获取一次最新队列。
     */
    const loadQueueItems = async () => {
      await store.dispatch("doctor/ensureQueueItems", { force: true });
    };

    const goToCreateOrder = (item: QueueItem) => {
      router.push({
        path: `${basePath.value}/create-order/${item.id}`,
        query: {
          ownerId: item.ownerId,
          petId: item.petId,
          petName: item.petName,
          sex: item.sex || "",
          breed: item.breed || "",
          age: item.age || "",
          ownerName: item.ownerName,
          symptom: item.symptom,
        },
      });
    };

    const totalPages = computed(() =>
      calculateTotalPages(queueItems.value.length, pageSize.value)
    );

    const visibleItems = computed(() =>
      getPagedItems(queueItems.value, page.value, pageSize.value)
    );

    // 始终把整页占位空行补满（0 条也先铺好一整页 ledger 排布）。
    const placeholderRows = computed(() =>
      createPlaceholderIndexes(pageSize.value, visibleItems.value.length)
    );

    // 按表格可用高度反推每页行数（与管理端一致），用占位空行补满。
    const updatePageSize = () => {
      const shell = tableShellRef.value;
      if (!shell) return;
      const shellHeight = shell.clientHeight;
      if (!shellHeight) return;
      const thead = shell.querySelector("thead");
      const headerHeight = thead ? thead.getBoundingClientRect().height : 40;
      const firstRow = shell.querySelector("tbody tr:not(.placeholder-row)");
      const rowHeight = firstRow ? firstRow.getBoundingClientRect().height : 48;
      const next = Math.max(
        4,
        Math.floor((shellHeight - headerHeight) / rowHeight)
      );
      if (Number.isFinite(next) && next !== pageSize.value) {
        pageSize.value = next;
      }
    };

    watch(totalPages, (value) => {
      if (page.value > value) {
        page.value = value;
      }
    });

    let resizeObserver: ResizeObserver | null = null;

    onMounted(() => {
      void loadQueueItems();
      void nextTick(() => {
        updatePageSize();
        if (tableShellRef.value && typeof ResizeObserver !== "undefined") {
          resizeObserver = new ResizeObserver(() => updatePageSize());
          resizeObserver.observe(tableShellRef.value);
        }
      });
    });

    onBeforeUnmount(() => {
      resizeObserver?.disconnect();
    });

    return {
      page,
      pageSize,
      tableShellRef,
      totalPages,
      visibleItems,
      placeholderRows,
      loadQueueItems,
      goToCreateOrder,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
}

.panel {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  height: var(--doctor-page-card-height, 860px);
  border: 1px solid rgba(148, 163, 184, 0.24);
  border-radius: 14px;
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.96), #f8fafc);
  padding: 18px 18px 22px;
  box-shadow: 0 20px 38px rgba(16, 24, 40, 0.06);
  box-sizing: border-box;
  overflow: hidden;
}

.panel-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  margin-bottom: 18px;
  gap: 12px;
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
  display: flex;
  align-items: center;
  gap: 12px;
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

.action-button {
  padding: 7px 12px;
  border-radius: 12px;
  background: linear-gradient(135deg, #eef2ff, #e0e7ff);
  color: #1e293b;
  box-shadow: none;
}

button:disabled {
  opacity: 0.52;
  cursor: not-allowed;
  box-shadow: none;
}

.table-shell {
  position: relative;
  min-height: 0;
  height: 100%;
  overflow: hidden;
}

table {
  width: 100%;
  border-collapse: collapse;
  table-layout: fixed;
}

th,
td {
  text-align: left;
  height: 48px;
  padding: 0 14px;
  border-bottom: 1px solid #e5e7eb;
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

.action-column-th {
  width: 110px;
  padding-right: 24px;
  text-align: right;
}

.action-column-td {
  width: 110px;
  text-align: right;
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

.tag {
  display: inline-flex;
  padding: 7px 11px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.tag.普通 {
  background: #f8fafc;
  color: #0f172a;
}

.tag.优先 {
  background: #fffbeb;
  color: #b45309;
}

.tag.紧急 {
  background: #fef2f2;
  color: #dc2626;
}

@media (max-width: 960px) {
  .panel-head {
    flex-direction: column;
    align-items: flex-start;
  }

  .panel-head__actions {
    width: 100%;
    flex-wrap: wrap;
  }

  .panel {
    overflow: visible;
  }

  table {
    min-width: 680px;
  }
}
</style>
