<template>
  <section class="panel">
    <div v-if="!openReservationDetail" class="reservation-list-view">
      <div class="panel-head">
        <div class="panel-head__intro">
          <h3>预约订单</h3>
          <p>以预约卡片快速处理预约成功、已取消和到院状态。</p>
        </div>
        <div class="panel-head__actions">
          <AppPager
            :page="page"
            :total-pages="totalPages"
            @update:page="page = $event"
          />
        </div>
      </div>

      <div class="search-bar">
        <span class="search-type">预约</span>
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
            placeholder="输入宠物名、医生名或联系人后按 Enter"
            @keyup.enter="searchReservations"
          />
        </label>
        <div class="search-meta">
          <span class="search-meta__count">
            {{ searchSummary }}
          </span>
          <button
            v-if="searchKeyword"
            type="button"
            class="search-meta__reset"
            @click="resetReservationSearch"
          >
            清空
          </button>
        </div>
      </div>

      <div class="filter-strip" aria-label="预约筛选">
        <div class="filter-group">
          <span class="filter-label">状态</span>
          <button
            v-for="option in statusFilterOptions"
            :key="option.value"
            type="button"
            class="filter-chip"
            :class="{ 'filter-chip--active': activeStatus === option.value }"
            @click="activeStatus = option.value"
          >
            <span>{{ option.label }}</span>
            <strong>{{ option.count }}</strong>
          </button>
        </div>

        <div class="filter-group">
          <span class="filter-label">日期</span>
          <button
            v-for="option in dateFilterOptions"
            :key="option.value"
            type="button"
            class="filter-chip"
            :class="{
              'filter-chip--active': activeDateFilter === option.value,
            }"
            @click="activeDateFilter = option.value"
          >
            <span>{{ option.label }}</span>
            <strong>{{ option.count }}</strong>
          </button>
        </div>
      </div>

      <AsyncViewState
        v-if="listLoading || listErrorMessage"
        :loading="listLoading"
        :error="listErrorMessage"
        loading-text="正在同步医生端预约记录"
        @retry="loadReservations"
      />

      <div
        v-else
        ref="cardsRef"
        class="cards"
        :style="{ '--reservation-page-size': pageSize }"
      >
        <article
          v-for="item in visibleItems"
          :key="item.id"
          class="card"
          @click="selectReservation(item)"
        >
          <div class="card-top">
            <strong>{{ item.pet_name || "预约记录" }}</strong>
            <span class="status" :class="statusClassName(item.status)">
              {{ item.status }}
            </span>
          </div>
          <p>{{ item.reservation_type }}</p>
          <p class="card-phone">预约编号 {{ item.id }}</p>
          <small>{{ item.schedule }} · {{ item.doctor_name }}</small>
        </article>
        <article
          v-for="placeholder in placeholderCards"
          :key="`placeholder-${placeholder}`"
          class="card card--placeholder"
        ></article>
        <div v-if="visibleItems.length === 0" class="empty-state">
          {{ emptyStateText }}
        </div>
      </div>
    </div>
    <div v-else-if="selectedReservation" class="reservation-detail">
      <header class="reservation-hero">
        <button type="button" class="back-button" @click="closeReservation">
          返回列表
        </button>

        <div class="reservation-hero__title">
          <p>预约详情</p>
          <h3>{{ selectedReservation.pet_name || "预约记录" }}</h3>
          <span>
            {{ selectedReservation.reservation_type }} ·
            {{ selectedReservation.schedule }}
          </span>
        </div>

        <div class="reservation-hero__action">
          <span
            class="status reservation-status"
            :class="statusClassName(selectedReservation.status)"
          >
            {{ selectedReservation.status }}
          </span>
          <button
            type="button"
            class="checkin-button"
            :disabled="checkInDisabled"
            @click="checkInReservation"
          >
            {{ checkInButtonText }}
          </button>
        </div>
      </header>

      <p
        v-if="actionMessage"
        class="action-message"
        :class="`action-message--${actionMessage.type}`"
      >
        {{ actionMessage.text }}
      </p>

      <section class="reservation-facts">
        <article class="reservation-fact">
          <small>预约时间</small>
          <strong>{{ selectedReservation.schedule || "待同步" }}</strong>
          <span>到院后可直接进入候诊处理。</span>
        </article>
        <article class="reservation-fact">
          <small>预约医生</small>
          <strong>{{ selectedReservation.doctor_name || "未分配" }}</strong>
          <span>当前记录绑定的接诊医生。</span>
        </article>
        <article class="reservation-fact">
          <small>预约类型</small>
          <strong>{{ selectedReservation.reservation_type }}</strong>
          <span>用于工作台识别服务项目。</span>
        </article>
        <article class="reservation-fact">
          <small>创建时间</small>
          <strong>{{ selectedReservation.created_at || "待同步" }}</strong>
          <span>预约记录写入系统的时间。</span>
        </article>
      </section>

      <section class="reservation-owner">
        <div class="reservation-owner__head">
          <div>
            <p>联系人信息</p>
            <h4>预约联系人</h4>
          </div>
          <span>NO.{{ selectedReservation.id }}</span>
        </div>

        <div class="reservation-owner__grid">
          <div>
            <small>联系人</small>
            <strong>{{ selectedReservation.user_name || "未登记" }}</strong>
          </div>
          <div>
            <small>联系电话</small>
            <strong>{{ selectedReservation.phone || "未登记手机号" }}</strong>
          </div>
          <div>
            <small>就诊宠物</small>
            <strong>{{ selectedReservation.pet_name || "预约记录" }}</strong>
          </div>
          <div>
            <small>预约日期</small>
            <strong>{{ selectedReservation.date || "待同步" }}</strong>
          </div>
          <div>
            <small>预约时段</small>
            <strong>{{ selectedReservation.time_slot || "待同步" }}</strong>
          </div>
          <div>
            <small>记录编号</small>
            <strong>
              U{{ selectedReservation.user_id }} / P{{
                selectedReservation.pet_id
              }}
            </strong>
          </div>
        </div>
      </section>
    </div>
    <AsyncViewState
      v-else
      :loading="detailLoading"
      :error="detailErrorMessage"
      loading-text="正在读取预约详情"
      @retry="retryReservationDetail"
    />
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
import { getHttpErrorMessage } from "@/api/httpError";
import { storeKey } from "@/app/store";
import AppPager from "@/shared/components/AppPager.vue";
import AsyncViewState from "@/shared/components/AsyncViewState.vue";
import { doctorApi } from "@/modules/doctor/api/doctorApi";
import {
  readOrderSearchHistory,
  saveOrderSearchKeyword,
} from "@/modules/user/utils/userPreferenceStorage";
import {
  ReservationItem,
  ReservationSummaryItem,
} from "@/modules/doctor/api/types";

export default defineComponent({
  name: "DoctorReservations",
  components: { AppPager, AsyncViewState },
  setup() {
    const store = useStore(storeKey);
    const page = ref(1);
    const searchKeyword = ref("");
    const activeStatus = ref("全部");
    const activeDateFilter = ref<"全部" | "今日" | "未来" | "过期">("全部");
    const pageSize = ref(9);
    const cardsRef = ref<HTMLElement | null>(null);
    const openReservationDetail = ref(false);
    const selectedReservationId = ref<number | null>(null);
    const checkInLoading = ref(false);
    const actionMessage = ref<{
      type: "success" | "error";
      text: string;
    } | null>(null);
    const searchLoading = ref(false);
    const searchResults = ref<ReservationSummaryItem[] | null>(null);
    const searchHistory = ref<Array<{ id: number; pet_name: string }>>([]);
    const listErrorMessage = ref("");
    const detailErrorMessage = ref("");
    const items = computed<ReservationSummaryItem[]>(
      () => store.state.doctor.reservations
    );
    const listLoading = computed(() =>
      Boolean(store.state.doctor.reservationsMeta.loading)
    );
    const detailLoading = computed(() =>
      Boolean(store.state.doctor.currentReservationDetailMeta.loading)
    );

    const selectedReservation = computed<ReservationItem | null>(() =>
      selectedReservationId.value &&
      Number(store.state.doctor.currentReservationDetail?.id) ===
        Number(selectedReservationId.value)
        ? store.state.doctor.currentReservationDetail
        : null
    );
    const normalizedSearchKeyword = computed(() => searchKeyword.value.trim());
    const displayItems = computed(() => searchResults.value ?? items.value);
    const todayDate = computed(() => new Date().toISOString().slice(0, 10));

    const itemDateValue = (item: ReservationSummaryItem) =>
      String(item.date || item.schedule.slice(0, 10)).slice(0, 10);

    const matchesDateFilter = (
      item: ReservationSummaryItem,
      filter: typeof activeDateFilter.value
    ) => {
      if (filter === "全部") {
        return true;
      }

      const itemDate = itemDateValue(item);
      if (!itemDate) {
        return false;
      }

      if (filter === "今日") {
        return itemDate === todayDate.value;
      }

      if (filter === "未来") {
        return itemDate > todayDate.value;
      }

      return itemDate < todayDate.value;
    };

    const filteredItems = computed(() =>
      displayItems.value.filter((item) => {
        const statusMatches =
          activeStatus.value === "全部" || item.status === activeStatus.value;
        return statusMatches && matchesDateFilter(item, activeDateFilter.value);
      })
    );

    const totalPages = computed(() =>
      Math.max(1, Math.ceil(filteredItems.value.length / pageSize.value))
    );

    const visibleItems = computed(() => {
      const start = (page.value - 1) * pageSize.value;
      return filteredItems.value.slice(start, start + pageSize.value);
    });

    const placeholderCards = computed(() =>
      visibleItems.value.length === 0
        ? []
        : Array.from(
            { length: Math.max(0, pageSize.value - visibleItems.value.length) },
            (_, index) => index + 1
          )
    );

    const updatePageSize = () => {
      const cardsElement = cardsRef.value;
      const height = cardsElement?.clientHeight ?? 0;
      const width = cardsElement?.clientWidth ?? 0;
      const gap = 12;
      const rowHeight = 128;
      const columns = width >= 900 ? 3 : 1;
      const rows = Math.max(1, Math.floor((height + gap) / (rowHeight + gap)));
      const nextPageSize = Math.max(columns, columns * rows);

      if (Number.isFinite(nextPageSize) && nextPageSize !== pageSize.value) {
        pageSize.value = nextPageSize;
      }
    };

    const searchSummary = computed(() => {
      if (!normalizedSearchKeyword.value) {
        return `共 ${filteredItems.value.length} 条预约记录`;
      }

      return `匹配到 ${filteredItems.value.length} 条预约记录`;
    });

    const statusFilterOptions = computed(() => {
      const statuses = Array.from(
        new Set(displayItems.value.map((item) => item.status || "预约成功"))
      );

      return ["全部", ...statuses].map((status) => ({
        value: status,
        label: status,
        count:
          status === "全部"
            ? displayItems.value.length
            : displayItems.value.filter((item) => item.status === status)
                .length,
      }));
    });

    const dateFilterOptions = computed(() =>
      (["全部", "今日", "未来", "过期"] as const).map((filter) => ({
        value: filter,
        label: filter,
        count:
          filter === "全部"
            ? displayItems.value.length
            : displayItems.value.filter((item) =>
                matchesDateFilter(item, filter)
              ).length,
      }))
    );

    const emptyStateText = computed(() =>
      normalizedSearchKeyword.value
        ? "没有找到符合当前关键词和筛选条件的预约。"
        : "当前筛选条件下暂无预约记录。"
    );

    const checkInDisabled = computed(() => {
      const status = selectedReservation.value?.status;

      return checkInLoading.value || status === "已到院" || status === "已取消";
    });

    const checkInButtonText = computed(() => {
      const status = selectedReservation.value?.status;

      if (checkInLoading.value) return "签到中";
      if (status === "已到院") return "已到院";
      if (status === "已取消") return "已取消";
      return "到院签到";
    });

    function statusClassName(status: string) {
      if (status === "已到院") return "status--arrived";
      if (status === "已取消" || status === "预约失败")
        return "status--cancelled";
      return "status--active";
    }

    async function loadReservations() {
      listErrorMessage.value = "";
      try {
        await store.dispatch("doctor/ensureReservations", { force: true });
        searchResults.value = null;
      } catch (error) {
        listErrorMessage.value = getHttpErrorMessage(
          error,
          "预约记录加载失败，请稍后重试"
        );
      }
    }

    async function searchReservations() {
      const keyword = normalizedSearchKeyword.value;
      if (!keyword) {
        searchResults.value = null;
        return;
      }

      listErrorMessage.value = "";
      searchLoading.value = true;
      try {
        searchResults.value = await doctorApi.searchReservationSummaries(
          keyword
        );
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
          "reservations"
        );
        page.value = 1;
      } catch (error) {
        listErrorMessage.value = getHttpErrorMessage(
          error,
          "预约记录搜索失败，请稍后重试"
        );
      } finally {
        searchLoading.value = false;
      }
    }

    function resetReservationSearch() {
      searchKeyword.value = "";
      searchResults.value = null;
      page.value = 1;
    }

    async function loadReservationDetail(reservationId: number) {
      detailErrorMessage.value = "";
      try {
        await store.dispatch("doctor/ensureReservationDetail", reservationId);
      } catch (error) {
        detailErrorMessage.value = getHttpErrorMessage(
          error,
          "预约详情加载失败，请稍后重试"
        );
      }
    }

    async function selectReservation(item: ReservationSummaryItem) {
      selectedReservationId.value = item.id;
      openReservationDetail.value = true;
      await loadReservationDetail(item.id);
    }

    function retryReservationDetail() {
      if (selectedReservationId.value) {
        void loadReservationDetail(selectedReservationId.value);
      }
    }

    function closeReservation() {
      openReservationDetail.value = false;
      selectedReservationId.value = null;
      detailErrorMessage.value = "";
      actionMessage.value = null;
    }

    async function checkInReservation() {
      const reservation = selectedReservation.value;

      if (!reservation || checkInDisabled.value) {
        return;
      }

      actionMessage.value = null;
      checkInLoading.value = true;
      try {
        await store.dispatch("doctor/updateReservationStatus", {
          reservationId: reservation.id,
          status: "已到院",
        });
        actionMessage.value = {
          type: "success",
          text: "到院签到已更新。",
        };
      } catch (error) {
        actionMessage.value = {
          type: "error",
          text: getHttpErrorMessage(error, "到院签到失败，请稍后重试。"),
        };
      } finally {
        checkInLoading.value = false;
      }
    }

    watch(totalPages, (value) => {
      if (page.value > value) {
        page.value = value;
      }
    });

    watch(searchKeyword, () => {
      page.value = 1;
      if (!normalizedSearchKeyword.value) {
        searchResults.value = null;
      }
    });

    watch([activeStatus, activeDateFilter], () => {
      page.value = 1;
    });

    watch(listLoading, (loading) => {
      if (!loading) {
        void nextTick(updatePageSize);
      }
    });

    let resizeObserver: ResizeObserver | null = null;

    onMounted(() => {
      searchHistory.value = readOrderSearchHistory("doctor", "reservations");
      void loadReservations();
      void nextTick(() => {
        updatePageSize();
        if (cardsRef.value && typeof ResizeObserver !== "undefined") {
          resizeObserver = new ResizeObserver(() => updatePageSize());
          resizeObserver.observe(cardsRef.value);
        }
      });
    });

    onBeforeUnmount(() => {
      resizeObserver?.disconnect();
    });

    return {
      page,
      searchKeyword,
      activeStatus,
      activeDateFilter,
      totalPages,
      listLoading,
      detailLoading,
      searchLoading,
      actionMessage,
      listErrorMessage,
      detailErrorMessage,
      visibleItems,
      placeholderCards,
      pageSize,
      cardsRef,
      searchSummary,
      statusFilterOptions,
      dateFilterOptions,
      emptyStateText,
      openReservationDetail,
      selectedReservation,
      checkInDisabled,
      checkInButtonText,
      loadReservations,
      searchReservations,
      resetReservationSearch,
      retryReservationDetail,
      selectReservation,
      closeReservation,
      checkInReservation,
      statusClassName,
    };
  },
});
</script>

<style scoped>
.panel {
  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr);
  gap: 14px;
  height: var(--doctor-page-card-height, 860px);
  border: 1px solid rgba(148, 163, 184, 0.24);
  border-radius: 14px;
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.96), #f8fafc);
  padding: 22px;
  box-shadow: 0 20px 38px rgba(16, 24, 40, 0.06);
  box-sizing: border-box;
  overflow: hidden;
}

.reservation-list-view {
  display: grid;
  grid-template-rows: auto auto auto minmax(0, 1fr);
  gap: 12px;
  min-height: 0;
}

.panel-head {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 12px;
}

.panel-head__intro {
  display: grid;
  gap: 6px;
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
  padding: 8px 12px;
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
  padding: 0 2px;
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
  font-size: 15px;
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

.filter-strip {
  display: grid;
  gap: 10px;
  padding: 12px 14px;
  border-radius: 14px;
  border: 1px solid rgba(148, 163, 184, 0.22);
  background: rgba(255, 255, 255, 0.58);
}

.filter-group {
  display: flex;
  align-items: center;
  gap: 8px;
  min-width: 0;
  flex-wrap: wrap;
}

.filter-label {
  color: #64748b;
  font-size: 12px;
  font-weight: 800;
  min-width: 32px;
}

.filter-chip {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  min-height: 30px;
  border-radius: 12px;
  padding: 6px 10px;
  border: 1px solid rgba(148, 163, 184, 0.34);
  background: rgba(255, 255, 255, 0.82);
  color: #4f46e5;
  box-shadow: none;
}

.filter-chip strong {
  min-width: 20px;
  border-radius: 999px;
  padding: 2px 6px;
  background: rgba(16, 24, 40, 0.08);
  color: #64748b;
  font-size: 11px;
}

.filter-chip--active {
  border-color: rgba(79, 70, 229, 0.28);
  background: #245849;
  color: #fff;
}

.filter-chip--active strong {
  background: rgba(255, 255, 255, 0.18);
  color: #fff;
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

button:disabled {
  opacity: 0.52;
  cursor: not-allowed;
  box-shadow: none;
}

.cards {
  position: relative;
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  grid-auto-rows: minmax(var(--doctor-card-row-height, 132px), 1fr);
  align-content: stretch;
  gap: 12px;
  min-height: 0;
  height: 100%;
  overflow: hidden;
}

.cards::after {
  content: "";
  position: absolute;
  right: 0;
  bottom: 0;
  left: 0;
  height: 1px;
  background: rgba(148, 163, 184, 0.86);
  pointer-events: none;
}

.card {
  position: relative;
  min-height: 0;
  padding: 14px;
  border-radius: 10px;
  background: linear-gradient(180deg, #ffffff, #f8fafc);
  border: 1px solid rgba(148, 163, 184, 0.22);
  box-shadow: 0 16px 30px rgba(16, 24, 40, 0.05);
  cursor: pointer;
}

.card::after {
  content: "";
  position: absolute;
  right: 14px;
  top: 14px;
  width: 34px;
  height: 34px;
  border-radius: 12px;
  background: rgba(215, 235, 227, 0.84);
}

.card-top {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
  position: relative;
  z-index: 1;
  gap: 12px;
}

.card-top strong {
  color: #0f172a;
  font-size: 16px;
}

.status {
  font-size: 12px;
  color: #49645f;
  padding: 5px 8px;
  border-radius: 999px;
  background: rgba(235, 245, 240, 0.94);
  border: 1px solid rgba(148, 163, 184, 0.22);
}

.status--active {
  color: #246563;
  background: rgba(226, 242, 235, 0.96);
  border-color: rgba(16, 24, 40, 0.24);
}

.status--arrived {
  color: #ffffff;
  background: linear-gradient(135deg, #315f62, #64748b);
  border-color: rgba(50, 95, 98, 0.28);
}

.status--cancelled {
  color: #dc2626;
  background: rgba(255, 238, 234, 0.96);
  border-color: rgba(178, 86, 73, 0.2);
}

.card p,
.card small {
  display: block;
  margin: 0;
  color: #5f7773;
  position: relative;
  z-index: 1;
}

.card-phone {
  margin-top: 8px !important;
  color: #355658 !important;
  font-weight: 600;
  letter-spacing: 0.04em;
}

.card small {
  margin-top: 6px;
}

.card--placeholder {
  visibility: hidden;
  pointer-events: none;
}

.empty-state {
  position: absolute;
  inset: 0;
  display: grid;
  grid-column: 1 / -1;
  place-items: center;
  min-height: 0;
  border-radius: 12px;
  border: 1px dashed rgba(160, 188, 181, 0.42);
  color: #64748b;
  background: rgba(255, 255, 255, 0.42);
}

.reservation-detail {
  display: grid;
  gap: 18px;
}

.reservation-hero,
.reservation-fact,
.reservation-owner {
  border: 1px solid rgba(148, 163, 184, 0.22);
  background: linear-gradient(180deg, #ffffff, #f8fafc);
  box-shadow: 0 16px 30px rgba(16, 24, 40, 0.05);
}

.reservation-hero {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr) auto;
  align-items: center;
  gap: 18px;
  padding: 22px;
  border-radius: 26px;
}

.back-button {
  background: rgba(238, 247, 243, 0.92);
  color: #31595b;
  box-shadow: none;
}

.reservation-hero__title {
  display: grid;
  gap: 8px;
  min-width: 0;
}

.reservation-hero__title p,
.reservation-owner__head p {
  margin: 0;
  color: #1c8584;
  font-size: 12px;
  font-weight: 800;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.reservation-hero__title h3,
.reservation-owner__head h4 {
  margin: 0;
  color: #0f172a;
}

.reservation-hero__title h3 {
  font-size: 34px;
  line-height: 1.15;
}

.reservation-hero__title span,
.reservation-fact span,
.reservation-owner__head span {
  color: #64748b;
  line-height: 1.7;
}

.reservation-hero__action {
  display: flex;
  align-items: center;
  gap: 12px;
}

.reservation-status {
  padding: 11px 14px;
  font-size: 13px;
  font-weight: 800;
  white-space: nowrap;
}

.checkin-button {
  min-width: 132px;
  min-height: 48px;
  border-radius: 999px;
  font-weight: 800;
}

.action-message {
  margin: 0;
  border-radius: 10px;
  padding: 10px 12px;
  font-size: 13px;
  font-weight: 700;
}

.action-message--success {
  border: 1px solid rgba(22, 163, 74, 0.26);
  background: rgba(22, 163, 74, 0.08);
  color: #16a34a;
}

.action-message--error {
  border: 1px solid rgba(220, 38, 38, 0.26);
  background: rgba(220, 38, 38, 0.08);
  color: #dc2626;
}

.reservation-facts {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 16px;
}

.reservation-fact {
  display: grid;
  gap: 8px;
  min-height: 132px;
  padding: 18px;
  border-radius: 12px;
}

.reservation-fact small,
.reservation-owner__grid small {
  color: #6c837d;
  font-size: 12px;
}

.reservation-fact strong,
.reservation-owner__grid strong {
  color: #0f172a;
  font-size: 20px;
}

.reservation-owner {
  padding: 22px;
  border-radius: 26px;
}

.reservation-owner__head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 14px;
  margin-bottom: 18px;
}

.reservation-owner__grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
}

.reservation-owner__grid div {
  display: grid;
  gap: 8px;
  padding: 16px;
  border-radius: 10px;
  background: rgba(255, 255, 255, 0.62);
}

@media (max-width: 960px) {
  .panel {
    min-height: auto;
  }

  .panel-head {
    flex-direction: column;
    align-items: flex-start;
  }

  .search-bar,
  .panel-head__actions,
  .panel-head__actions,
  .filter-strip {
    width: 100%;
    flex-wrap: wrap;
  }

  .search-bar {
    align-items: stretch;
  }

  .search-meta {
    justify-content: space-between;
  }

  .cards {
    grid-template-columns: 1fr;
    grid-auto-rows: minmax(var(--doctor-card-row-height, 132px), auto);
    overflow: visible;
  }

  .reservation-hero,
  .reservation-facts,
  .reservation-owner__grid {
    grid-template-columns: 1fr;
  }

  .reservation-hero__action {
    align-items: stretch;
    flex-direction: column;
  }
}
</style>
