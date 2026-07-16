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
        <section class="card card--resv">
          <div class="card__head">
            <b>预约</b>
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
  --transition-speed: 0.3s;
  --transition-bounce: cubic-bezier(0.34, 1.56, 0.64, 1);
  
  --bg-app: linear-gradient(180deg, #fdfbf7 0%, #f2f7f4 100%);
  --color-text-main: #1f3a36;
  --color-text-muted: #6b7d77;
  --color-text-faint: #94a3b8;
  
  /* Hospital Theme Colors */
  --color-accent: #2f9e8f; /* Brand Teal */
  --color-accent-hover: #1f7a6c;
  --color-accent-bg: #e7f5f1;
  --color-warm: #c2671b; /* Brand Orange */
  --color-warm-bg: #ffe8d2;
  
  --card-bg: #ffffff;
  --card-border: 1px solid #efe7dc;
  --card-radius: 24px;
  --card-shadow: 0 10px 30px rgba(47, 158, 143, 0.04);
  --card-shadow-hover: 0 20px 40px rgba(47, 158, 143, 0.09);
  --welcome-bg: linear-gradient(135deg, #fff4e6 0%, #eafaf5 100%);
  --welcome-border: 1px solid #efe7dc;
  --btn-radius: 14px;
  --avatar-bg: #ffe8d2;
  
  --badge-ok-bg: #ecfdf5;
  --badge-ok-color: #047857;
  --badge-warn-bg: #fff4e6;
  --badge-warn-color: #b45309;
  --badge-bad-bg: #fdeef0;
  --badge-bad-color: #be4b5b;

  display: grid;
  gap: 16px;
  color: var(--color-text-main);
}

.btn {
  border: 0;
  cursor: pointer;
  border-radius: var(--btn-radius);
  font-weight: 700;
  font-size: 14px;
  padding: 12px 22px;
  background: var(--color-accent);
  color: #fff;
  transition: all 0.25s var(--transition-bounce);
  display: inline-flex;
  align-items: center;
  gap: 8px;
}

.btn:hover {
  background: var(--color-accent-hover);
  transform: translateY(-2px);
  box-shadow: 0 8px 20px rgba(47, 158, 143, 0.2);
}

.btn:active {
  transform: translateY(1px);
}

.btn--ghost {
  background: transparent;
  color: var(--color-accent);
  border: 1.5px solid var(--color-accent);
  box-shadow: none;
}

.btn--ghost:hover {
  background: var(--color-accent-bg);
  color: var(--color-accent);
}

.card {
  background: var(--card-bg);
  border: var(--card-border);
  border-radius: var(--card-radius);
  box-shadow: var(--card-shadow);
  transition: all var(--transition-speed) ease;
  overflow: hidden;
}

.card--resv {
  height: 198px;
  box-sizing: border-box;
  display: flex;
  flex-direction: column;
}

.card--resv .appt {
  flex: 1;
  margin-bottom: 20px;
}

.card--resv .empty {
  flex: 1;
  display: flex;
  align-items: center;
  margin-top: 0;
  margin-bottom: 20px;
}

/* 欢迎 */
.welcome {
  display: flex;
  align-items: center;
  gap: 20px;
  padding: 24px 28px;
  border-radius: var(--card-radius);
  background: var(--welcome-bg);
  border: var(--welcome-border);
  box-shadow: var(--card-shadow);
  transition: all var(--transition-speed) ease;
}

.welcome__av {
  width: 60px;
  height: 60px;
  border-radius: 50%;
  display: grid;
  place-items: center;
  font-size: 26px;
  font-weight: 800;
  background: var(--avatar-bg);
  color: var(--color-warm);
  border: 2px solid #ffffff;
  box-shadow: 0 4px 10px rgba(0,0,0,0.05);
}

.welcome__txt h2 {
  margin: 0;
  font-size: 22px;
  font-weight: 800;
}

.welcome__txt p {
  margin: 4px 0 0;
  color: var(--color-text-muted);
  font-size: 14px;
}

.welcome__cta {
  margin-left: auto;
  display: flex;
  gap: 12px;
}

/* 快捷入口 */
.quick {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16px;
}

.qc {
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 10px;
  cursor: pointer;
  text-align: left;
  background: var(--card-bg);
  border: var(--card-border);
  border-radius: var(--card-radius);
  box-shadow: var(--card-shadow);
  transition: all var(--transition-speed) var(--transition-bounce);
}

.qc:hover {
  transform: translateY(-6px);
  box-shadow: var(--card-shadow-hover);
  border-color: var(--color-accent);
}

.qc__ic {
  width: 44px;
  height: 44px;
  border-radius: calc(var(--card-radius) - 8px);
  display: grid;
  place-items: center;
  font-size: 20px;
  transition: transform 0.3s ease;
}

.qc:hover .qc__ic {
  transform: scale(1.1) rotate(5deg);
}

.qc__ic--a {
  background: var(--color-accent-bg);
  color: var(--color-accent);
}
.qc__ic--b {
  background: var(--color-warm-bg);
  color: var(--color-warm);
}
.qc__ic--c {
  background: #eef2ff;
  color: #4f46e5;
}
.qc__ic--d {
  background: var(--badge-bad-bg);
  color: var(--badge-bad-color);
}

.qc b {
  font-size: 15px;
  font-weight: 700;
}

.qc small {
  color: var(--color-text-muted);
  font-size: 12px;
  line-height: 1.4;
}

/* 两栏 */
.cols {
  display: grid;
  grid-template-columns: 1.5fr 1fr;
  gap: 20px;
  align-items: start;
}

.col {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.card__head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 20px 24px 12px;
}

.card__head b {
  font-size: 16px;
  font-weight: 700;
  color: var(--color-text-main);
}

.link {
  font-size: 13px;
  font-weight: 600;
  color: var(--color-accent);
  cursor: pointer;
  text-decoration: none;
}
.link:hover {
  color: var(--color-accent-hover);
  text-decoration: underline;
}

/* 预约 */
.appt {
  display: flex;
  align-items: center;
  gap: 16px;
  margin: 0 20px 20px;
  padding: 16px;
  border-radius: calc(var(--card-radius) - 6px);
  background: var(--color-accent-bg);
  border: 1px solid rgba(47, 158, 143, 0.08);
  transition: all 0.3s ease;
}

.appt__day {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 58px;
  height: 58px;
  border-radius: calc(var(--card-radius) - 10px);
  background: #ffffff;
  color: var(--color-accent-hover);
  box-shadow: 0 4px 10px rgba(0,0,0,0.03);
}

.appt__day b {
  font-size: 20px;
  font-weight: 800;
  line-height: 1;
}

.appt__day small {
  font-size: 11px;
  font-weight: 600;
  margin-top: 2px;
}

.appt__meta {
  flex: 1;
  min-width: 0;
}

.appt__meta b {
  font-size: 15px;
  font-weight: 700;
  display: block;
}

.appt__meta p {
  margin-top: 4px;
  color: var(--color-text-muted);
  font-size: 13px;
}

.appt .pill {
  margin-left: auto;
}

.empty {
  margin: 0 24px 20px;
  color: var(--color-text-muted);
  font-size: 13px;
}

/* 订单行 */
.order-row {
  width: 100%;
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 14px 24px;
  border: 0;
  border-top: 1px solid rgba(239, 231, 220, 0.4);
  background: transparent;
  cursor: pointer;
  font-size: 14px;
  color: var(--color-text-main);
  font-family: inherit;
  transition: background 0.2s;
  text-align: left;
}

.order-row:hover {
  background: rgba(0, 0, 0, 0.015);
}

.order-row__id {
  font-weight: 700;
  color: var(--color-text-muted);
  font-variant-numeric: tabular-nums;
}

.order-row__pet {
  color: var(--color-text-muted);
}

.order-row__price {
  margin-left: auto;
  font-weight: 700;
  color: var(--color-warm);
  font-variant-numeric: tabular-nums;
}

.pill {
  display: inline-flex;
  align-items: center;
  height: 24px;
  padding: 0 10px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.pill--ok {
  background: var(--badge-ok-bg);
  color: var(--badge-ok-color);
}

.pill--warn {
  background: var(--badge-warn-bg);
  color: var(--badge-warn-color);
}

.pill--bad {
  background: var(--badge-bad-bg);
  color: var(--badge-bad-color);
}

/* 入口卡 */
.entry {
  padding: 20px 24px;
  cursor: pointer;
  text-align: left;
  width: 100%;
  border: 0;
  border-top: var(--card-border);
  background: transparent;
  transition: all 0.2s ease;
}

.entry:hover {
  background: rgba(0, 0, 0, 0.01);
}

.entry__row {
  display: flex;
  align-items: center;
  gap: 16px;
}

.entry__ic {
  width: 48px;
  height: 48px;
  border-radius: calc(var(--card-radius) - 10px);
  display: grid;
  place-items: center;
  font-size: 22px;
  background: var(--color-accent-bg);
  color: var(--color-accent);
}

.entry__ic--pet {
  background: var(--color-warm-bg);
  color: var(--color-warm);
}

.entry__ic--me {
  background: #eef2ff;
  color: #4f46e5;
}

.entry__row b {
  font-size: 15px;
  font-weight: 700;
  display: block;
}

.entry__row small {
  display: block;
  color: var(--color-text-muted);
  font-size: 12px;
  margin-top: 4px;
}

.entry__go {
  margin-left: auto;
  font-size: 18px;
  color: var(--color-text-faint);
  transition: transform 0.2s ease;
}

.entry:hover .entry__go {
  transform: translateX(4px);
  color: var(--color-accent);
}

.intro {
  padding: 24px;
  display: flex;
  flex-direction: column;
  gap: 12px;
  background: var(--welcome-bg);
  border: var(--welcome-border);
}

.intro b {
  font-size: 16px;
  font-weight: 700;
}

.intro p {
  margin: 0;
  color: var(--color-text-muted);
  font-size: 13px;
  line-height: 1.6;
}

.intro .btn--ghost {
  align-self: flex-start;
  margin-top: 4px;
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

