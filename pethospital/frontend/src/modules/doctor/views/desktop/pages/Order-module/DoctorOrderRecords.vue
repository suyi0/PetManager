<template>
  <section class="panel">
    <div class="panel-head">
      <div>
        <h3>订单记录</h3>
        <p>按诊单编号、宠物、主人和费用追踪历史记录。</p>
      </div>
      <div class="panel-head__actions">
        <AppPager
          :page="page"
          :total-pages="totalPages"
          @update:page="page = $event"
        />
        <button>导出记录</button>
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
      <span class="search-type">orders</span>
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
          placeholder="输入诊单编号、宠物名、医生名或订单状态"
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
        <button
          type="button"
          class="search-meta__submit"
          :disabled="searchLoading"
          @click="searchOrderRecords"
        >
          {{ searchLoading ? "搜索中" : "搜索" }}
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

    <div v-else class="table-shell">
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
import { computed, defineComponent, ref, watch, onMounted } from "vue";
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
    const pageSize = 10;
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

      return `orders · 匹配到 ${items.value.length} 条订单记录`;
    });

    const emptyStateText = computed(() =>
      normalizedSearchKeyword.value
        ? "没有找到符合关键词的订单，请检查诊单编号、宠物名、医生名或订单状态。"
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

    watch(totalPages, (value) => {
      if (page.value > value) {
        page.value = value;
      }
    });

    onMounted(() => {
      searchHistory.value = readOrderSearchHistory("doctor", "orders");
      void loadOrderRecords();
    });

    return {
      activeStatus,
      page,
      searchKeyword,
      totalPages,
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

.status-filters {
  display: grid;
  grid-template-columns: repeat(6, minmax(0, 1fr));
  gap: 12px;
  margin-bottom: 18px;
}

.search-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 14px;
  margin-bottom: 18px;
  padding: 14px 16px;
  border-radius: 22px;
  background: radial-gradient(
      circle at left top,
      rgba(215, 233, 225, 0.72),
      transparent 55%
    ),
    linear-gradient(
      135deg,
      rgba(255, 255, 255, 0.96),
      rgba(241, 249, 244, 0.88)
    );
  border: 1px solid rgba(163, 192, 184, 0.24);
}

.search-type {
  flex: 0 0 auto;
  border-radius: 999px;
  padding: 8px 12px;
  background: rgba(41, 86, 90, 0.1);
  color: #29565a;
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
  width: 42px;
  height: 42px;
  border-radius: 14px;
  display: grid;
  place-items: center;
  color: #426260;
  background: rgba(223, 238, 232, 0.95);
  box-shadow: inset 0 0 0 1px rgba(163, 192, 184, 0.2);
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
  color: #173739;
  font-size: 16px;
  line-height: 1.5;
}

.search-field input::placeholder {
  color: #7d938d;
}

.search-meta {
  display: flex;
  align-items: center;
  gap: 12px;
}

.search-meta__count {
  font-size: 13px;
  color: #5d7671;
  white-space: nowrap;
}

.search-meta__reset {
  border: 0;
  padding: 0;
  border-radius: 0;
  background: transparent;
  color: #355c5f;
  box-shadow: none;
}

.search-meta__submit {
  padding: 9px 14px;
  border-radius: 14px;
}

.status-filter {
  display: grid;
  gap: 6px;
  justify-items: start;
  padding: 14px 16px;
  border: 1px solid rgba(155, 185, 177, 0.22);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.72);
  color: #24484b;
  box-shadow: 0 12px 26px rgba(49, 82, 77, 0.06);
}

.status-filter span {
  font-size: 13px;
  color: #69817d;
}

.status-filter strong {
  font-size: 22px;
  line-height: 1;
}

.status-filter--active {
  border-color: rgba(41, 97, 94, 0.28);
  background: linear-gradient(135deg, #edf8f3, #fff7ef);
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

table {
  width: 100%;
  border-collapse: collapse;
  table-layout: fixed;
}

.table-shell {
  min-height: 0;
  overflow: hidden;
}

th,
td {
  text-align: left;
  padding: 14px 12px;
  border-bottom: 1px solid rgba(226, 236, 232, 0.92);
  font-size: 13px;
  text-align: center;
}

th {
  color: #6f8582;
  font-weight: 600;
}

td {
  color: #19383b;
}

tbody tr {
  background: rgba(255, 255, 255, 0.46);
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
  background: #fff1db;
  color: #b86c11;
}

.status-pill--done {
  background: #dff4e9;
  color: #1f8960;
}

.status-pill--cancelled {
  background: #f7e2e2;
  color: #bf4747;
}

.empty-cell {
  color: #708682;
  padding: 26px 12px;
}

.placeholder-row td {
  height: 52px;
  background: rgba(255, 255, 255, 0.32);
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
    overflow: auto;
  }

  table {
    min-width: 760px;
  }
}
</style>
