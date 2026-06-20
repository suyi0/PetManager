<template>
  <section class="panel">
    <div class="panel-head">
      <div>
        <h3>订单记录</h3>
        <p>按宠物、主人、医生和费用追踪历史记录。</p>
      </div>
      <div class="panel-head__actions">
        <AppPager
          :page="page"
          :total-pages="totalPages"
          @update:page="page = $event"
        />
      </div>
    </div>

    <div class="status-filters">
      <button
        v-for="tab in statusTabs"
        :key="tab.key"
        type="button"
        class="status-filter"
        :class="{ 'status-filter--active': activeStatus === tab.key }"
        @click="activeStatus = tab.key"
      >
        <span>{{ tab.label }}</span>
        <strong>{{ tab.count }}</strong>
      </button>
    </div>

    <div class="search-bar">
      <span class="search-type">诊单</span>
      <label class="search-field">
        <span class="search-field__icon" aria-hidden="true">
          <svg viewBox="0 0 24 24" fill="none">
            <circle
              cx="11"
              cy="11"
              r="6.5"
              stroke="currentColor"
              stroke-width="2"
            />
            <path
              d="M16 16L21 21"
              stroke="currentColor"
              stroke-width="2"
              stroke-linecap="round"
            />
          </svg>
        </span>
        <input
          v-model.trim="searchKeyword"
          type="text"
          placeholder="输入宠物名、医生名、主人名或订单状态后按 Enter"
          @keyup.enter="searchOrderRecords"
        />
      </label>
      <div class="search-meta">
        <span class="search-meta__count">{{ searchSummary }}</span>
        <button
          v-if="searchKeyword"
          type="button"
          class="search-meta__reset"
          @click="resetOrderSearch"
        >
          清空
        </button>
      </div>
    </div>

    <AsyncViewState
      v-if="isLoading || errorMessage"
      :loading="isLoading"
      :error="errorMessage"
      loading-text="正在同步医生端订单记录"
      @retry="loadOrderRecords"
    />

    <div
      v-else
      ref="tableShellRef"
      class="table-shell"
      :style="{ '--record-page-size': pageSize }"
    >
      <table>
        <thead>
          <tr>
            <th>诊单编号</th>
            <th>宠物</th>
            <th>医生</th>
            <th>类型</th>
            <th>总费用</th>
            <th>状态</th>
          </tr>
        </thead>
        <tbody>
          <tr
            v-for="item in pagedItems"
            :key="item.id"
            class="record-row"
            @click="openOrderDetail(item.id)"
          >
            <td>{{ item.id }}</td>
            <td>{{ item.pet_name }}</td>
            <td>{{ item.doctor_name || "未记录" }}</td>
            <td>{{ item.order_type || "诊疗" }}</td>
            <td>¥{{ item.order_totalprice.toFixed(2) }}</td>
            <td>
              <span
                class="status-pill"
                :class="statusClassName(item.order_status)"
              >
                {{ item.order_status }}
              </span>
            </td>
          </tr>
          <tr
            v-for="placeholder in placeholderRows"
            :key="`placeholder-${placeholder}`"
            class="placeholder-row"
          >
            <td colspan="6"></td>
          </tr>
          <tr v-if="visibleItems.length === 0">
            <td colspan="6" class="empty-cell">{{ emptyStateText }}</td>
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
  nextTick,
  onBeforeUnmount,
  onMounted,
  ref,
  watch,
} from "vue";
import { useRouter } from "vue-router";
import { useStore } from "vuex";
import { getHttpErrorMessage } from "@/api/httpError";
import { storeKey } from "@/app/store";
import AppPager from "@/shared/components/AppPager.vue";
import AsyncViewState from "@/shared/components/AsyncViewState.vue";
import { doctorApi } from "@/modules/doctor/api/doctorApi";
import {
  readOrderSearchHistory,
  saveOrderSearchKeyword,
} from "@/modules/user/utils/userPreferenceStorage";
import { OrderSummaryItem } from "@/modules/doctor/api/types";

export default defineComponent({
  name: "DoctorOrderRecords",
  components: { AppPager, AsyncViewState },
  setup() {
    const store = useStore(storeKey);
    const router = useRouter();
    const activeStatus = ref<"全部" | OrderSummaryItem["order_status"]>("全部");
    const page = ref(1);
    const searchKeyword = ref("");
    const searchLoading = ref(false);
    const searchResults = ref<OrderSummaryItem[] | null>(null);
    const searchHistory = ref<Array<{ id: number; pet_name: string }>>([]);
    const errorMessage = ref("");
    const pageSize = ref(10);
    const tableShellRef = ref<HTMLElement | null>(null);
    const orderRecords = computed<OrderSummaryItem[]>(
      () => store.state.doctor.orderRecords
    );

    const basePath = computed(() => "/doctor");

    /**
     * 进入订单记录页时通过 RESTful 获取一次最新摘要。
     */
    const loadOrderRecords = async () => {
      errorMessage.value = "";
      try {
        await store.dispatch("doctor/ensureOrderRecords", { force: true });
        searchResults.value = null;
      } catch (error) {
        errorMessage.value = getHttpErrorMessage(
          error,
          "订单记录加载失败，请稍后重试"
        );
      }
    };

    const isLoading = computed(() =>
      Boolean(store.state.doctor.orderRecordsMeta.loading)
    );

    /**
     * 按订单编号降序展示，确保新创建的订单摘要排在前面。
     */
    const items = computed(() =>
      [...(searchResults.value ?? orderRecords.value)].sort(
        (a, b) => Number(b.id) - Number(a.id)
      )
    );

    /**
     * 根据订单状态筛选出当前页需要展示的订单记录数据，如果状态为“全部”则返回所有订单记录
     */
    const visibleItems = computed(() => {
      if (activeStatus.value === "全部") {
        return items.value;
      }
      return items.value.filter(
        (item) => item.order_status === activeStatus.value
      );
    });

    const totalPages = computed(() =>
      Math.max(1, Math.ceil(visibleItems.value.length / pageSize.value))
    );

    const pagedItems = computed(() => {
      const start = (page.value - 1) * pageSize.value;
      return visibleItems.value.slice(start, start + pageSize.value);
    });

    const placeholderRows = computed(() =>
      pagedItems.value.length === 0
        ? []
        : Array.from(
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

    /**
     * 状态标签
     */
    const statusTabs = computed(() => {
      const statuses: Array<"全部" | OrderSummaryItem["order_status"]> = [
        "全部",
        "待付款",
        "已付款",
        "已取消",
        "已退款",
        "部分退款",
      ];

      return statuses.map((status) => ({
        key: status,
        label: status,
        count:
          status === "全部"
            ? items.value.length
            : items.value.filter((item) => item.order_status === status).length,
      }));
    });

    const normalizedSearchKeyword = computed(() => searchKeyword.value.trim());

    const searchSummary = computed(() => {
      if (!normalizedSearchKeyword.value) {
        return `共 ${orderRecords.value.length} 条订单记录`;
      }

      return `匹配到 ${items.value.length} 条订单记录`;
    });

    const emptyStateText = computed(() =>
      normalizedSearchKeyword.value
        ? "没有找到符合关键词的订单，请检查宠物名、医生名、主人名或订单状态。"
        : "当前分类下暂无订单记录。"
    );

    const statusClassName = (status: OrderSummaryItem["order_status"]) => {
      if (status === "待付款") return "status-pill--pending";
      if (status === "已付款") return "status-pill--done";
      return "status-pill--cancelled";
    };

    const openOrderDetail = (orderId: number) => {
      router.push({
        path: `${basePath.value}/orders/${orderId}`,
        query: { from: "records" },
      });
    };

    const searchOrderRecords = async () => {
      const keyword = normalizedSearchKeyword.value;
      if (!keyword) {
        searchResults.value = null;
        return;
      }

      errorMessage.value = "";
      searchLoading.value = true;
      try {
        searchResults.value = await doctorApi.searchOrderSummaries(keyword);
        await doctorApi.updateSearchHistory(keyword);
        searchHistory.value = saveOrderSearchKeyword(
          searchHistory.value,
          keyword,
          (nextKeyword) => ({
            id: Date.now(),
            pet_name: nextKeyword,
          }),
          (item) => item.pet_name,
          15,
          "doctor",
          "orders"
        );
        page.value = 1;
      } catch (error) {
        errorMessage.value = getHttpErrorMessage(
          error,
          "订单记录搜索失败，请稍后重试"
        );
      } finally {
        searchLoading.value = false;
      }
    };

    const resetOrderSearch = () => {
      searchKeyword.value = "";
      searchResults.value = null;
      page.value = 1;
    };

    watch(activeStatus, () => {
      page.value = 1;
    });

    watch(searchKeyword, () => {
      page.value = 1;
      if (!normalizedSearchKeyword.value) {
        searchResults.value = null;
      }
    });

    watch(isLoading, (loading) => {
      if (!loading) {
        void nextTick(updatePageSize);
      }
    });

    watch(totalPages, (value) => {
      if (page.value > value) {
        page.value = value;
      }
    });

    let resizeObserver: ResizeObserver | null = null;

    onMounted(() => {
      searchHistory.value = readOrderSearchHistory("doctor", "orders");
      void loadOrderRecords();
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
      activeStatus,
      page,
      searchKeyword,
      totalPages,
      pageSize,
      tableShellRef,
      isLoading,
      searchLoading,
      errorMessage,
      statusTabs,
      searchSummary,
      emptyStateText,
      visibleItems,
      pagedItems,
      placeholderRows,
      statusClassName,
      openOrderDetail,
      searchOrderRecords,
      resetOrderSearch,
      loadOrderRecords,
    };
  },
});
</script>

<style scoped>
.panel {
  display: grid;
  grid-template-rows: auto auto auto minmax(0, 1fr);
  gap: 12px;
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
  justify-content: space-between;
  align-items: flex-start;
  gap: 12px;
}

.panel-head h3,
.panel-head p {
  margin: 0;
}

.panel-head p {
  margin-top: 4px;
  color: #64748b;
  line-height: 1.4;
}

.panel-head__actions {
  display: flex;
  align-items: center;
  gap: 12px;
}

.status-filters {
  display: grid;
  grid-template-columns: repeat(6, minmax(0, 1fr));
  gap: 10px;
}

.search-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  padding: 10px 12px;
  border-radius: 16px;
  background: radial-gradient(
      circle at left top,
      rgba(238, 242, 255, 0.72),
      transparent 55%
    ),
    linear-gradient(
      135deg,
      rgba(255, 255, 255, 0.96),
      rgba(248, 250, 252, 0.88)
    );
  border: 1px solid rgba(148, 163, 184, 0.24);
}

.search-type {
  flex: 0 0 auto;
  border-radius: 999px;
  padding: 6px 10px;
  background: rgba(16, 24, 40, 0.1);
  color: #4f46e5;
  font-size: 12px;
  font-weight: 800;
}

.search-field {
  display: flex;
  align-items: center;
  gap: 12px;
  min-width: 0;
  flex: 1;
}

.search-field__icon {
  width: 36px;
  height: 36px;
  border-radius: 12px;
  display: grid;
  place-items: center;
  color: #64748b;
  background: rgba(238, 242, 255, 0.95);
  box-shadow: inset 0 0 0 1px rgba(148, 163, 184, 0.2);
}

.search-field__icon svg {
  width: 18px;
  height: 18px;
}

.search-field input {
  width: 100%;
  border: 0;
  outline: none;
  background: transparent;
  color: #0f172a;
  font-size: 14px;
  line-height: 1.5;
}

.search-field input::placeholder {
  color: #64748b;
}

.search-meta {
  display: flex;
  align-items: center;
  gap: 12px;
}

.search-meta__count {
  font-size: 13px;
  color: #64748b;
  white-space: nowrap;
}

.search-meta__reset {
  border: 0;
  padding: 0;
  border-radius: 0;
  background: transparent;
  color: #4f46e5;
  box-shadow: none;
}

.status-filter {
  display: grid;
  gap: 4px;
  justify-items: start;
  padding: 10px 12px;
  border: 1px solid rgba(155, 185, 177, 0.22);
  border-radius: 14px;
  background: rgba(255, 255, 255, 0.72);
  color: #24484b;
  box-shadow: 0 12px 26px rgba(16, 24, 40, 0.06);
}

.status-filter span {
  font-size: 13px;
  color: #69817d;
}

.status-filter strong {
  font-size: 18px;
  line-height: 1;
}

.status-filter--active {
  border-color: rgba(79, 70, 229, 0.28);
  background: linear-gradient(135deg, #eef2ff, #f8fafc);
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

table {
  width: 100%;
  border-collapse: collapse;
  table-layout: fixed;
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

.record-row {
  cursor: pointer;
  transition: background 0.18s ease, transform 0.18s ease;
}

.record-row:hover {
  background: rgba(238, 247, 242, 0.95);
}

.status-pill {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 72px;
  padding: 6px 12px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.status-pill--pending {
  background: #fffbeb;
  color: #b45309;
}

.status-pill--done {
  background: #ecfdf5;
  color: #1f8960;
}

.status-pill--cancelled {
  background: #fef2f2;
  color: #dc2626;
}

.empty-cell {
  height: calc(48px * var(--record-page-size, 10));
  color: #64748b;
  padding: 26px 12px;
  text-align: center;
}

.placeholder-row td {
  height: 48px;
  background: #ffffff;
}

@media (max-width: 960px) {
  .panel-head {
    flex-direction: column;
    align-items: flex-start;
  }

  .panel-head__actions,
  .panel-head__actions {
    width: 100%;
    flex-wrap: wrap;
  }

  .status-filters {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .panel {
    overflow: visible;
  }

  table {
    min-width: 760px;
  }
}
</style>
