<template>
  <section class="dash">
    <!-- 欢迎横幅 -->
    <div class="welcome">
      <div class="welcome__av">{{ userInitial }}</div>
      <div class="welcome__txt">
        <h2>欢迎回来，{{ userDisplayName }}</h2>
        <p>今天也照顾好毛孩子 🐾</p>
      </div>
      <div class="welcome__cta">
        <button class="btn" @click="goTo('/user/services')">立即预约</button>
        <button class="btn btn--ghost" @click="goTo('/user/personal?tab=pet')">
          我的宠物
        </button>
      </div>
    </div>

    <!-- 快捷入口 -->
    <div class="quick">
      <button
        v-for="q in quickActions"
        :key="q.label"
        class="qc"
        @click="goTo(q.to)"
      >
        <span class="qc__ic" :class="`qc__ic--${q.tone}`">{{ q.icon }}</span>
        <b>{{ q.label }}</b>
        <small>{{ q.hint }}</small>
      </button>
    </div>

    <!-- 两栏内容 -->
    <div class="cols">
      <div class="col">
        <section class="card">
          <div class="card__head">
            <b>下个预约</b>
            <a class="link" @click="goTo('/user/order')">查看全部 →</a>
          </div>
          <div v-if="nextReservation" class="appt">
            <div class="appt__day">
              <b>{{ dayOf(nextReservation) }}</b>
              <small>{{ monthOf(nextReservation) }}</small>
            </div>
            <div class="appt__meta">
              <b
                >{{ nextReservation.reservation_type || "预约" }} ·
                {{ nextReservation.doctor_name || "待分配" }}</b
              >
              <p>
                {{
                  nextReservation.schedule ||
                  nextReservation.time_slot ||
                  "待同步"
                }}
                · {{ nextReservation.pet_name || "宠物" }}
              </p>
            </div>
            <span class="pill" :class="resvPill(nextReservation.status)">{{
              nextReservation.status
            }}</span>
          </div>
          <div v-else class="empty">
            暂无预约，<a class="link" @click="goTo('/user/services')"
              >去预约 →</a
            >
          </div>
        </section>

        <section class="card">
          <div class="card__head">
            <b>最近订单</b>
            <a class="link" @click="goTo('/user/order')">全部 →</a>
          </div>
          <template v-if="recentOrders.length">
            <button
              v-for="o in recentOrders"
              :key="o.id"
              class="order-row"
              @click="goTo('/user/order')"
            >
              <span class="order-row__id">#{{ o.id }}</span>
              <span class="order-row__type">{{ o.order_type || "诊疗" }}</span>
              <span class="order-row__pet">{{ o.pet_name || "—" }}</span>
              <span class="order-row__price"
                >¥{{ Number(o.order_totalprice || 0).toFixed(2) }}</span
              >
              <span class="pill" :class="orderPill(o.order_status)">{{
                o.order_status
              }}</span>
            </button>
          </template>
          <div v-else class="empty">暂无订单记录。</div>
        </section>
      </div>

      <div class="col">
        <button class="card entry" @click="goTo('/user/personal?tab=pet')">
          <div class="entry__row">
            <span class="entry__ic entry__ic--pet">🐶</span>
            <div>
              <b>宠物档案</b>
              <small>维护宠物资料与护理备注</small>
            </div>
            <span class="entry__go">→</span>
          </div>
        </button>

        <button class="card entry" @click="goTo('/user/personal')">
          <div class="entry__row">
            <span class="entry__ic entry__ic--me">👤</span>
            <div>
              <b>个人资料</b>
              <small>地址 · 手机号 · 邮箱</small>
            </div>
            <span class="entry__go">→</span>
          </div>
        </button>

        <section class="card intro">
          <b>了解医院</b>
          <p>环境 · 医生团队 · 设备 · 服务优势，到院前先了解更安心。</p>
          <button class="btn btn--ghost" @click="goTo('/user/services')">
            查看服务
          </button>
        </section>
      </div>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, onMounted } from "vue";
import { useStore } from "vuex";
import { useRouter } from "vue-router";
import { storeKey } from "@/app/store";
import type {
  ReservationSummary,
  OrderSummary,
} from "@/modules/user/api/types";

const store = useStore(storeKey);
const router = useRouter();

const userDisplayName = computed(
  () => store.getters["auth/formattedUserName"] || "体验用户"
);
const userInitial = computed(() =>
  String(userDisplayName.value || "U")
    .trim()
    .charAt(0)
    .toUpperCase()
);

const reservations = computed<ReservationSummary[]>(
  () => store.state.userPortal.reservationRecords
);
const orders = computed<OrderSummary[]>(
  () => store.state.userPortal.orderSummaries
);

const nextReservation = computed(() => {
  const list = reservations.value;
  return (
    list.find((r) => !["已取消", "已完成", "预约失败"].includes(r.status)) ||
    list[0] ||
    null
  );
});

const recentOrders = computed(() => orders.value.slice(0, 4));

const quickActions = [
  {
    icon: "📅",
    label: "服务预约",
    hint: "选医生 · 选时段",
    tone: "a",
    to: "/user/services",
  },
  {
    icon: "🧾",
    label: "我的订单",
    hint: "订单与预约记录",
    tone: "b",
    to: "/user/order",
  },
  {
    icon: "🐶",
    label: "宠物档案",
    hint: "资料与护理备注",
    tone: "c",
    to: "/user/personal?tab=pet",
  },
  {
    icon: "👤",
    label: "个人资料",
    hint: "地址 · 手机 · 邮箱",
    tone: "d",
    to: "/user/personal",
  },
];

const goTo = (to: string) => {
  void router.push(to);
};

const dayOf = (r: ReservationSummary) => {
  const d = (r.date || r.schedule || "").slice(0, 10).split("-");
  return d[2] || "--";
};
const monthOf = (r: ReservationSummary) => {
  const d = (r.date || r.schedule || "").slice(0, 10).split("-");
  return d[1] ? `${d[1]}月` : "";
};

const resvPill = (status: string) => {
  if (status === "已到院" || status === "预约成功") return "pill--ok";
  if (status === "已取消" || status === "预约失败") return "pill--bad";
  return "pill--warn";
};
const orderPill = (status: string) => {
  if (status === "已付款" || status === "已完成") return "pill--ok";
  if (status === "已取消" || status === "已退款") return "pill--bad";
  return "pill--warn";
};

onMounted(() => {
  void store.dispatch("userPortal/ensureReservationRecords");
  void store.dispatch("userPortal/ensureOrderSummaries");
});
</script>

<style scoped>
.dash {
  --teal: #2f9e8f;
  --teal-d: #1f7a6c;
  --teal-50: #e7f5f1;
  --warm: #c2671b;
  --ink: #1f3a36;
  --muted: #6b7d77;
  --faint: #94a3b8;
  --border: #efe7dc;

  display: grid;
  gap: 16px;
  color: var(--ink);
}

.btn {
  border: 0;
  cursor: pointer;
  border-radius: 12px;
  font-weight: 700;
  font-size: 14px;
  padding: 10px 18px;
  background: var(--teal);
  color: #fff;
}

.btn:hover {
  background: var(--teal-d);
}

.btn--ghost {
  background: #fff;
  color: var(--teal);
  box-shadow: inset 0 0 0 1px #cfe7e1;
}

.card {
  background: #fff;
  border: 1px solid var(--border);
  border-radius: 16px;
  box-shadow: 0 8px 20px rgba(47, 158, 143, 0.06);
}

/* 欢迎 */
.welcome {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 20px 22px;
  border-radius: 18px;
  background: linear-gradient(135deg, #fff4e6, #eafaf5);
  border: 1px solid var(--border);
}

.welcome__av {
  width: 54px;
  height: 54px;
  border-radius: 50%;
  display: grid;
  place-items: center;
  font-size: 22px;
  font-weight: 800;
  background: #ffe8d2;
  color: var(--warm);
}

.welcome__txt h2 {
  margin: 0;
  font-size: 22px;
  font-weight: 800;
}

.welcome__txt p {
  margin: 3px 0 0;
  color: var(--muted);
  font-size: 13px;
}

.welcome__cta {
  margin-left: auto;
  display: flex;
  gap: 10px;
}

/* 快捷入口 */
.quick {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 12px;
}

.qc {
  padding: 16px;
  display: grid;
  gap: 8px;
  cursor: pointer;
  text-align: left;
  background: #fff;
  border: 1px solid var(--border);
  border-radius: 16px;
  box-shadow: 0 8px 20px rgba(47, 158, 143, 0.06);
  transition: transform 0.15s ease, box-shadow 0.15s ease;
}

.qc:hover {
  transform: translateY(-2px);
  box-shadow: 0 12px 26px rgba(47, 158, 143, 0.1);
}

.qc__ic {
  width: 38px;
  height: 38px;
  border-radius: 11px;
  display: grid;
  place-items: center;
  font-size: 18px;
}

.qc__ic--a {
  background: #e7f5f1;
}
.qc__ic--b {
  background: #fff4e6;
}
.qc__ic--c {
  background: #eef2ff;
}
.qc__ic--d {
  background: #fdeef0;
}

.qc b {
  font-size: 14px;
}

.qc small {
  color: var(--muted);
  font-size: 12px;
}

/* 两栏 */
.cols {
  display: grid;
  grid-template-columns: 1.4fr 1fr;
  gap: 16px;
  align-items: start;
}

.col {
  display: grid;
  gap: 16px;
}

.card__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 18px 10px;
}

.card__head b {
  font-size: 15px;
}

.link {
  font-size: 12px;
  color: var(--teal);
  cursor: pointer;
}

/* 预约 */
.appt {
  display: flex;
  align-items: center;
  gap: 14px;
  margin: 0 14px 14px;
  padding: 14px;
  border-radius: 12px;
  background: var(--teal-50);
}

.appt__day {
  display: grid;
  place-items: center;
  width: 54px;
  height: 54px;
  border-radius: 12px;
  background: #fff;
  color: var(--teal-d);
}

.appt__day b {
  font-size: 18px;
  line-height: 1;
}

.appt__day small {
  font-size: 11px;
}

.appt__meta {
  min-width: 0;
}

.appt__meta b {
  font-size: 15px;
}

.appt__meta p {
  margin: 2px 0 0;
  color: var(--muted);
  font-size: 13px;
}

.appt .pill {
  margin-left: auto;
}

.empty {
  margin: 0 18px 16px;
  color: var(--muted);
  font-size: 13px;
}

/* 订单行 */
.order-row {
  width: 100%;
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 11px 18px;
  border: 0;
  border-top: 1px solid #f4f1ec;
  background: #fff;
  cursor: pointer;
  font-size: 13px;
  text-align: left;
}

.order-row:hover {
  background: #fbfaf7;
}

.order-row__id {
  font-weight: 700;
  color: var(--muted);
  font-variant-numeric: tabular-nums;
}

.order-row__pet {
  color: var(--muted);
}

.order-row__price {
  margin-left: auto;
  font-weight: 700;
  font-variant-numeric: tabular-nums;
}

.pill {
  display: inline-flex;
  align-items: center;
  height: 22px;
  padding: 0 9px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.pill--ok {
  background: #ecfdf5;
  color: #047857;
}

.pill--warn {
  background: #fff4e6;
  color: #b45309;
}

.pill--bad {
  background: #fdeef0;
  color: #be4b5b;
}

/* 入口卡 */
.entry {
  padding: 16px 18px;
  cursor: pointer;
  text-align: left;
}

.entry:hover {
  border-color: #cfe7e1;
}

.entry__row {
  display: flex;
  align-items: center;
  gap: 12px;
}

.entry__ic {
  width: 40px;
  height: 40px;
  border-radius: 12px;
  display: grid;
  place-items: center;
  font-size: 18px;
}

.entry__ic--pet {
  background: #fff4e6;
}

.entry__ic--me {
  background: #eef2ff;
}

.entry__row b {
  font-size: 14px;
}

.entry__row small {
  display: block;
  color: var(--muted);
  font-size: 12px;
}

.entry__go {
  margin-left: auto;
  color: var(--faint);
}

.intro {
  padding: 18px;
  display: grid;
  gap: 10px;
  background: linear-gradient(135deg, #fff4e6, #eafaf5);
}

.intro b {
  font-size: 15px;
}

.intro p {
  margin: 0;
  color: var(--muted);
  font-size: 13px;
  line-height: 1.6;
}

.intro .btn--ghost {
  justify-self: start;
}

@media (max-width: 1100px) {
  .quick {
    grid-template-columns: repeat(2, 1fr);
  }
  .cols {
    grid-template-columns: 1fr;
  }
}
</style>
