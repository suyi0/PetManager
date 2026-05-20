<template>
  <section class="detail-page">
    <header class="detail-hero">
      <div class="detail-hero__media">
        <button class="detail-hero__media-button" @click="changeImg('magnify')">
          <div class="detail-hero__media-art">
            <span>{{ detailTag }}</span>
            <strong>{{ detail.title }}</strong>
          </div>
        </button>
      </div>

      <div class="detail-hero__copy">
        <div class="detail-hero__topline">
          <p>{{ detailTag }}</p>
          <button
            v-if="tabValue === 'reservation'"
            type="button"
            class="detail-cancel-button"
            :disabled="cancelLoading"
            @click="cancelReservation"
          >
            {{ cancelLoading ? "取消中" : "取消预约" }}
          </button>
        </div>
        <h2>{{ detail.title }}</h2>
        <span>{{ detail.description }}</span>

        <div class="detail-hero__chips">
          <span>{{ detail.status }}</span>
          <span>{{ detail.price }}</span>
          <span>{{ detail.reference }}</span>
        </div>
      </div>
    </header>

    <div
      v-if="orderImg === 'magnify'"
      class="detail-lightbox"
      @click="changeImg('reduce')"
    >
      <div class="detail-lightbox__card">
        <span>{{ detailTag }}</span>
        <strong>{{ detail.title }}</strong>
        <p>{{ detail.description }}</p>
      </div>
    </div>

    <section class="detail-grid">
      <article class="detail-card">
        <p>订单编号</p>
        <strong>{{ detail.reference }}</strong>
        <span>用于售后、咨询与历史记录追踪。</span>
      </article>
      <article class="detail-card">
        <p>创建时间</p>
        <strong>{{ detail.createTime }}</strong>
        <span>记录创建时间会随接口返回或本地预览数据同步。</span>
      </article>
      <article class="detail-card">
        <p>当前状态</p>
        <strong>{{ detail.status }}</strong>
        <span>可用于快速判断是否已完成、待处理或待到院。</span>
      </article>
      <article v-if="tabValue === 'reservation'" class="detail-card">
        <p>预约时间</p>
        <strong>{{ detail.reservateTime }}</strong>
        <span>若是预约记录，会额外展示预约到院时段。</span>
      </article>
    </section>

    <section class="detail-panel">
      <div class="detail-panel__header">
        <div>
          <p>Detail Notes</p>
          <h3>说明与摘要</h3>
        </div>
        <span>这里优先展示用户最关心的状态、时间与费用信息。</span>
      </div>

      <div class="detail-panel__content">
        <div>
          <small>项目名称</small>
          <strong>{{ detail.title }}</strong>
        </div>
        <div>
          <small>费用</small>
          <strong>{{ detail.price }}</strong>
        </div>
        <div>
          <small>记录描述</small>
          <strong>{{ detail.description }}</strong>
        </div>
      </div>
    </section>
  </section>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { useRoute, useRouter } from "vue-router";
import { OrderRecordItem, OrderSummary } from "@/modules/user/api/types";

const store = useStore(storeKey);
const route = useRoute();
const router = useRouter();

const tabValue = ref<string>("order");
const orderImg = ref("reduce");
const previewRecord = ref<(OrderRecordItem & { tab?: string }) | null>(null);
const detailRecord = ref<OrderSummary | null>(null);
const reservationRecord = ref<OrderRecordItem | null>(null);
const cancelLoading = ref(false);

const currentReservationId = computed(() =>
  Number(
    reservationRecord.value?.id ||
      previewRecord.value?.id ||
      route.query.id ||
      0
  )
);

const detail = computed(() => {
  const preview = previewRecord.value;
  const summary = detailRecord.value;
  const reservation = reservationRecord.value;

  const reservationDate = reservation?.date || preview?.date || "";
  const reservationSlot = reservation?.time_slot || preview?.time_slot || "";
  const reservationTime = [reservationDate, reservationSlot]
    .filter(Boolean)
    .join(" ");
  const reservationCreatedAt =
    reservation?.created_at || preview?.created_at || "";

  const title =
    summary?.pet_name ||
    reservation?.pet_name ||
    preview?.pet_name ||
    preview?.name ||
    (tabValue.value === "reservation" ? "预约记录" : "订单记录");

  const dateText = summary?.order_data || formatDate(preview?.time);
  const priceText =
    typeof summary?.order_totalprice === "number"
      ? `¥ ${summary.order_totalprice.toFixed(2)}`
      : typeof preview?.price === "number"
      ? `¥ ${preview.price.toFixed(2)}`
      : "待结算";

  return {
    title,
    description:
      tabValue.value === "reservation"
        ? "这是一条预约记录详情，可用于确认预约时间、状态和关联项目。"
        : "这是一条订单详情，可用于回看消费项目、创建时间和当前状态。",
    reference:
      summary?.id !== undefined
        ? `NO.${summary.id}`
        : preview?.id !== undefined
        ? `NO.${preview.id}`
        : "NO.--",
    createTime:
      tabValue.value === "reservation"
        ? reservationCreatedAt || "待同步"
        : summary?.created_at || dateText || "待同步",
    reservateTime:
      tabValue.value === "reservation"
        ? reservationTime || "待同步"
        : summary?.order_data || dateText || "待同步",
    status:
      summary?.order_status ||
      reservation?.status ||
      preview?.status ||
      (tabValue.value === "reservation" ? "预约处理中" : "处理中"),
    price: priceText,
  };
});

const detailTag = computed(() =>
  tabValue.value === "reservation" ? "Reservation Detail" : "Order Detail"
);

function changeImg(type: string) {
  orderImg.value = type === "magnify" ? "magnify" : "reduce";
}

function formatDate(value?: number) {
  if (typeof value !== "number") return "";
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return "";
  return date.toLocaleString("zh-CN", {
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
  });
}

const cancelReservation = async () => {
  const reservationId = currentReservationId.value;

  if (!reservationId || cancelLoading.value) {
    return;
  }

  const confirmed = window.confirm("确认取消并删除这条预约记录吗？");
  if (!confirmed) {
    return;
  }

  cancelLoading.value = true;
  try {
    await store.dispatch("userPortal/deleteReservationRecord", reservationId);
    reservationRecord.value = null;
    previewRecord.value = null;
    sessionStorage.removeItem("userOrderDetailPreview");
    await router.push({
      path: "/user/order",
      query: { tab: "reservation" },
    });
  } catch {
    window.alert("取消预约失败，请稍后重试。");
  } finally {
    cancelLoading.value = false;
  }
};

onMounted(async () => {
  tabValue.value = (route.query.tab as string) || "order";

  const cached = sessionStorage.getItem("userOrderDetailPreview");
  if (cached) {
    try {
      previewRecord.value = JSON.parse(cached);
    } catch {
      previewRecord.value = null;
    }
  }

  try {
    const currentId = Number(route.query.id || previewRecord.value?.id || 0);

    if (tabValue.value === "reservation") {
      const rows = (await store.dispatch(
        "userPortal/ensureReservationRecords"
      )) as OrderRecordItem[];
      reservationRecord.value =
        rows.find((item) => Number(item.id) === currentId) ||
        previewRecord.value ||
        null;
    } else {
      /**
       * 订单详情页优先从缓存摘要里定位当前记录，只有首次进入时才会触发拉取。
       */
      const rows = (await store.dispatch(
        "userPortal/ensureOrderSummaries"
      )) as OrderSummary[];

      if (currentId > 0) {
        detailRecord.value =
          rows.find((item) => Number(item.id) === currentId) || null;
      }
    }
  } catch {
    detailRecord.value = null;
    reservationRecord.value = previewRecord.value;
  }

  if (!detailRecord.value) {
    detailRecord.value = null;
  }
});
</script>

<style scoped lang="scss">
.detail-page {
  display: grid;
  gap: 18px;
  min-height: calc(100vh - 170px);
}

.detail-hero,
.detail-card,
.detail-panel {
  border-radius: 30px;
  border: 1px solid rgba(21, 91, 92, 0.1);
  background: rgba(255, 250, 242, 0.82);
  box-shadow: 0 24px 55px rgba(25, 92, 93, 0.08);
}

.detail-hero {
  display: grid;
  grid-template-columns: 320px minmax(0, 1fr);
  gap: 20px;
  padding: 24px;
}

.detail-hero__media-button {
  width: 100%;
  border: none;
  background: transparent;
  cursor: pointer;
}

.detail-hero__media-art {
  display: grid;
  align-content: end;
  min-height: 260px;
  padding: 22px;
  border-radius: 26px;
  background: linear-gradient(145deg, #9ee0d8, #1a7777);
  color: #fffdf8;
}

.detail-hero__media-art span,
.detail-panel__header p {
  margin: 0 0 8px;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 12px;
  font-weight: 700;
}

.detail-hero__media-art strong {
  font-size: 34px;
  line-height: 1.1;
}

.detail-hero__copy {
  display: grid;
  align-content: center;
  gap: 12px;
}

.detail-hero__topline {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 14px;
}

.detail-hero__copy p {
  margin: 0;
  color: #1e8a88;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 12px;
  font-weight: 700;
}

.detail-cancel-button {
  min-width: 176px;
  min-height: 56px;
  padding: 16px 28px;
  border: 1px solid rgba(191, 72, 72, 0.24);
  border-radius: 999px;
  background: rgba(255, 241, 238, 0.9);
  color: #b94747;
  cursor: pointer;
  font-size: 16px;
  font-weight: 800;
  box-shadow: 0 18px 36px rgba(147, 58, 58, 0.12);
  transition: transform 0.18s ease, border-color 0.18s ease,
    background 0.18s ease;
}

.detail-cancel-button:hover:not(:disabled) {
  border-color: rgba(191, 72, 72, 0.36);
  background: #ffe7e3;
  transform: translateY(-1px);
}

.detail-cancel-button:disabled {
  cursor: wait;
  opacity: 0.68;
}

.detail-hero__copy h2 {
  margin: 0;
  color: #143d40;
  font-size: clamp(32px, 4vw, 46px);
}

.detail-hero__copy span,
.detail-panel__header span,
.detail-card span {
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.detail-hero__chips {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.detail-hero__chips span {
  padding: 10px 14px;
  border-radius: 999px;
  background: rgba(29, 134, 135, 0.08);
  color: #176867;
  font-weight: 700;
}

.detail-lightbox {
  position: fixed;
  inset: 0;
  display: grid;
  place-items: center;
  background: rgba(20, 61, 64, 0.26);
  z-index: 40;
}

.detail-lightbox__card {
  width: min(520px, calc(100vw - 32px));
  padding: 28px;
  border-radius: 28px;
  background: rgba(255, 250, 242, 0.96);
  box-shadow: 0 24px 55px rgba(25, 92, 93, 0.2);
}

.detail-lightbox__card span {
  display: block;
  margin-bottom: 10px;
  color: #1e8a88;
  text-transform: uppercase;
  letter-spacing: 0.1em;
  font-size: 12px;
  font-weight: 700;
}

.detail-lightbox__card strong {
  display: block;
  margin-bottom: 10px;
  color: #143d40;
  font-size: 30px;
}

.detail-lightbox__card p {
  margin: 0;
  color: #607975;
  line-height: 1.8;
}

.detail-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 16px;
}

.detail-card {
  padding: 20px;
}

.detail-card p {
  margin: 0 0 10px;
  color: #1d8b89;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.detail-card strong,
.detail-panel__content strong {
  display: block;
  color: #143d40;
  font-size: 22px;
}

.detail-panel {
  padding: 24px;
}

.detail-panel__header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 16px;
  margin-bottom: 18px;
}

.detail-panel__header h3 {
  margin: 0;
  color: #143d40;
  font-size: 30px;
}

.detail-panel__content {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 16px;
}

.detail-panel__content div {
  padding: 18px;
  border-radius: 22px;
  background: rgba(255, 255, 255, 0.62);
}

.detail-panel__content small {
  display: block;
  margin-bottom: 8px;
  color: #69817e;
  font-size: 12px;
}

@media (max-width: 1100px) {
  .detail-hero,
  .detail-grid,
  .detail-panel__content {
    grid-template-columns: 1fr;
  }

  .detail-hero__topline {
    align-items: flex-start;
    flex-direction: column;
  }
}
</style>
