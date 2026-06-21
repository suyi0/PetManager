<template>
  <section class="detail-page">
    <button class="back" @click="goBack">← 返回</button>

    <div class="detail-head">
      <div class="detail-head__title">
        <span class="tag">{{
          tabValue === "reservation" ? "预约详情" : "订单详情"
        }}</span>
        <h2>{{ detail.title }}</h2>
        <div class="chips">
          <span class="chip">{{ detail.status }}</span>
          <span class="chip">{{ detail.price }}</span>
          <span class="chip chip--muted">{{ detail.reference }}</span>
        </div>
      </div>
      <button
        v-if="tabValue === 'reservation'"
        type="button"
        class="cancel-btn"
        :disabled="cancelLoading"
        @click="cancelReservation"
      >
        {{ cancelLoading ? "取消中…" : "取消预约" }}
      </button>
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
        <div v-if="tabValue === 'reservation'">
          <small>预约人</small>
          <strong>{{ detail.ownerName }}</strong>
        </div>
        <div v-if="tabValue === 'reservation'">
          <small>联系电话</small>
          <strong>{{ detail.phone }}</strong>
        </div>
        <div v-if="tabValue === 'reservation'">
          <small>接诊医生</small>
          <strong>{{ detail.doctorName }}</strong>
        </div>
        <div v-if="tabValue === 'reservation'">
          <small>预约类型</small>
          <strong>{{ detail.reservationTypeText }}</strong>
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
import {
  OrderDetail,
  ReservationOrderRecordItem,
  OrderSummary,
} from "@/modules/user/api/types";

const store = useStore(storeKey);
const route = useRoute();
const router = useRouter();

const tabValue = ref<string>("order");
const detailRecord = ref<OrderDetail | OrderSummary | null>(null);
const reservationRecord = ref<ReservationOrderRecordItem | null>(null);
const cancelLoading = ref(false);

const currentReservationId = computed(() =>
  Number(reservationRecord.value?.id || route.query.id || 0)
);

const detail = computed(() => {
  const summary = detailRecord.value;
  const reservation = reservationRecord.value;

  const reservationDate = reservation?.date || "";
  const reservationSlot = reservation?.time_slot || "";
  const reservationTime =
    reservation?.schedule ||
    [reservationDate, reservationSlot].filter(Boolean).join(" ");
  const reservationCreatedAt = reservation?.created_at || "";
  const reservationTypeText = reservation?.reservation_type || "预约";

  const title =
    summary?.pet_name ||
    reservation?.pet_name ||
    (tabValue.value === "reservation" ? "预约记录" : "订单记录");

  const dateText = summary?.order_data || "";
  const reservationPrice = reservation?.price;
  const priceText =
    typeof summary?.order_totalprice === "number"
      ? `¥ ${summary.order_totalprice.toFixed(2)}`
      : typeof reservationPrice === "number"
      ? `¥ ${reservationPrice.toFixed(2)}`
      : "待结算";

  return {
    title,
    description:
      tabValue.value === "reservation"
        ? `这是一条${reservationTypeText}记录详情，可用于确认预约时间、状态和关联项目。`
        : "这是一条完整订单详情，可用于回看消费项目、创建时间和当前状态。",
    reference:
      summary?.id !== undefined
        ? `NO.${summary.id}`
        : reservation?.id !== undefined
        ? `NO.${reservation.id}`
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
      (tabValue.value === "reservation" ? "预约处理中" : "处理中"),
    price: priceText,
    ownerName: reservation?.user_name || "待同步",
    phone: reservation?.phone || "待同步",
    doctorName: summary?.doctor_name || reservation?.doctor_name || "待同步",
    reservationTypeText,
  };
});

const goBack = () => {
  void router.push("/user/order");
};

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

  try {
    const currentId = Number(route.query.id || 0);

    if (tabValue.value === "reservation") {
      if (currentId > 0) {
        reservationRecord.value = (await store.dispatch(
          "userPortal/ensureReservationDetail",
          currentId
        )) as ReservationOrderRecordItem | null;
      }
    } else {
      if (currentId > 0) {
        detailRecord.value = (await store.dispatch(
          "userPortal/ensureOrderDetail",
          currentId
        )) as OrderDetail | null;
      }

      if (!detailRecord.value) {
        const rows = (await store.dispatch(
          "userPortal/ensureOrderSummaries"
        )) as OrderSummary[];
        const summary = rows.find((item) => Number(item.id) === currentId);
        detailRecord.value = summary || null;
      }
    }
  } catch {
    detailRecord.value = null;
    reservationRecord.value = null;
  }

  if (!detailRecord.value) {
    detailRecord.value = null;
  }
});
</script>

<style scoped lang="scss">
.detail-page {
  display: grid;
  gap: 14px;
  align-content: start;
}

.detail-card,
.detail-panel {
  border-radius: 16px;
  border: 1px solid #efe7dc;
  background: #ffffff;
  box-shadow: 0 8px 20px rgba(47, 158, 143, 0.06);
}

.back {
  justify-self: start;
  border: 0;
  padding: 4px 6px;
  background: transparent;
  color: #6b7d77;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
}

.back:hover {
  color: #2f9e8f;
}

/* 头部 */
.detail-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
  padding: 18px 20px;
  border-radius: 16px;
  border: 1px solid #efe7dc;
  background: linear-gradient(135deg, #fff4e6, #eafaf5);
}

.tag {
  display: inline-flex;
  color: #c2671b;
  letter-spacing: 0.06em;
  font-size: 12px;
  font-weight: 700;
}

.detail-head__title h2 {
  margin: 6px 0 10px;
  color: #1f3a36;
  font-size: 22px;
  font-weight: 800;
  letter-spacing: -0.01em;
}

.chips {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.chip {
  display: inline-flex;
  align-items: center;
  height: 24px;
  padding: 0 10px;
  border-radius: 999px;
  background: #e7f5f1;
  color: #1f7a6c;
  font-size: 12px;
  font-weight: 700;
}

.chip--muted {
  background: #f1f2f4;
  color: #6b7d77;
}

.cancel-btn {
  flex: 0 0 auto;
  height: 38px;
  padding: 0 18px;
  border: 1px solid #f3c9cd;
  border-radius: 9px;
  background: #fff;
  color: #be4b5b;
  cursor: pointer;
  font-size: 13px;
  font-weight: 700;
}

.cancel-btn:hover:not(:disabled) {
  background: #fdeef0;
}

.cancel-btn:disabled {
  cursor: wait;
  opacity: 0.65;
}

/* 信息卡 */
.detail-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
}

.detail-card {
  padding: 16px 18px;
}

.detail-card p {
  margin: 0 0 6px;
  color: #c2671b;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.06em;
}

.detail-card strong {
  display: block;
  color: #1f3a36;
  font-size: 17px;
}

.detail-card span {
  color: #6b7d77;
  line-height: 1.6;
  font-size: 12px;
}

/* 说明面板 */
.detail-panel {
  padding: 18px 20px;
}

.detail-panel__header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 16px;
  margin-bottom: 14px;
}

.detail-panel__header p {
  margin: 0 0 4px;
  color: #2f9e8f;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 10px;
  font-weight: 700;
}

.detail-panel__header h3 {
  margin: 0;
  color: #1f3a36;
  font-size: 16px;
}

.detail-panel__header span {
  color: #6b7d77;
  font-size: 13px;
}

.detail-panel__content {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
}

.detail-panel__content div {
  padding: 14px 16px;
  border-radius: 12px;
  border: 1px solid #f4f1ec;
  background: #fffdfa;
}

.detail-panel__content small {
  display: block;
  margin-bottom: 6px;
  color: #6b7d77;
  font-size: 12px;
}

.detail-panel__content strong {
  display: block;
  color: #1f3a36;
  font-size: 15px;
}

@media (max-width: 1100px) {
  .detail-head,
  .detail-grid,
  .detail-panel__content {
    flex-direction: column;
    grid-template-columns: 1fr;
  }

  .detail-hero__topline {
    align-items: flex-start;
    flex-direction: column;
  }
}
</style>
