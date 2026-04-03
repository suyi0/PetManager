<template>
  <section class="services-page">
    <aside class="services-sidebar">
      <button
        class="services-sidebar__item"
        :class="{
          'services-sidebar__item--active': isReservationTab(activeTab),
        }"
        @click="switchTab('reservation')"
      >
        <span>预约服务</span>
        <small>医生、日期与时段统一预约</small>
      </button>
      <button
        class="services-sidebar__item"
        :class="{ 'services-sidebar__item--active': activeTab === 'afterSale' }"
        @click="switchTab('afterSale')"
      >
        <span>复诊服务</span>
        <small>复诊说明、护理建议与咨询入口</small>
      </button>
    </aside>

    <div class="services-stage">
      <section class="services-hero">
        <div class="services-hero__copy">
          <p>Services</p>
          <h2>把高频服务整理成容易看懂、能直接预约的入口。</h2>
          <span>
            现在保留原有预约能力，并把服务类型、预约流程和下层弹窗排在同一条阅读路径里。
          </span>
        </div>
        <div class="services-hero__metrics">
          <article>
            <strong>4</strong>
            <span>当前可预约服务类型</span>
          </article>
          <article>
            <strong>7</strong>
            <span>连续日期时段支持选择</span>
          </article>
          <article>
            <strong>1</strong>
            <span>预约成功后自动回到服务首页</span>
          </article>
        </div>
      </section>

      <section v-if="activeTab === 'reservation'" class="service-grid">
        <button
          v-for="service in serviceCards"
          :key="service.key"
          class="service-card"
          @click="switchTab(service.tab)"
        >
          <div class="service-card__badge">{{ service.label }}</div>
          <strong>{{ service.title }}</strong>
          <span>{{ service.description }}</span>
        </button>
      </section>

      <section v-else-if="activeTab === 'afterSale'" class="after-sale-panel">
        <article v-for="item in afterSaleCards" :key="item.title">
          <p>{{ item.label }}</p>
          <strong>{{ item.title }}</strong>
          <span>{{ item.description }}</span>
        </article>
      </section>

      <section
        v-if="
          activeTab === 'reservation-treatSlots' ||
          activeTab === 'showSlots' ||
          activeTab === 'reservation-sterilizateSlots' ||
          activeTab === 'reservation-beautySlots' ||
          activeTab === 'reservation-SPASlots'
        "
        class="slot-shell"
      >
        <div class="slot-shell__header">
          <div>
            <p>预约流程</p>
            <h3>{{ activeServiceTitle }}</h3>
          </div>
          <span>选择医生后进入时段选择，成功提交后会给出结果提示。</span>
        </div>
        <treatSlots
          :active-tab="normalizedSlotTab"
          :doctor-data="doctorData"
          :schedule-data="scheduleData"
          :switchTab="switchTab"
          @close="close"
          @cancle="cancel"
          @submit-success="handleSubmitSuccess"
        />
      </section>
    </div>

    <div v-if="submitAfter" class="submit-mask"></div>
  </section>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import treatSlots from "@/modules/user/views/Services-module/treatSlots.vue";
import { reservationApi } from "@/modules/user/api/userApi";
import { DoctorDataItem } from "@/modules/doctor/api/types";
import { ReservationScheduleState } from "@/modules/user/api/types";

const activeTab = ref("reservation");
const submitAfter = ref(false);
const doctorData = ref<DoctorDataItem[]>([]);
const scheduleData = ref<Omit<ReservationScheduleState, "doctorData">>({
  year: [],
  month: [],
  day: [],
  weekday: [],
  slots: [],
});

const serviceCards = [
  {
    key: "treat",
    tab: "reservation-treatSlots",
    label: "门诊",
    title: "宠物医治",
    description: "适合常规问诊、复诊跟进与症状检查。",
  },
  {
    key: "sterilize",
    tab: "reservation-sterilizateSlots",
    label: "外科",
    title: "宠物绝育",
    description: "围绕术前沟通、手术安排与恢复观察展开。",
  },
  {
    key: "beauty",
    tab: "reservation-beautySlots",
    label: "护理",
    title: "宠物美容",
    description: "用于基础清洁、护理预约与形象打理。",
  },
  {
    key: "spa",
    tab: "reservation-SPASlots",
    label: "舒缓",
    title: "宠物 SPA",
    description: "适合舒缓护理和精细化清洁服务。",
  },
];

const afterSaleCards = [
  {
    label: "复诊提醒",
    title: "术后护理节奏",
    description: "查看恢复期注意事项、喂药时间与复诊建议。",
  },
  {
    label: "咨询支持",
    title: "轻症先咨询",
    description: "遇到术后恢复问题，可先联系医院获取初步建议。",
  },
  {
    label: "记录整合",
    title: "和订单页联动",
    description: "后续可以回到订单记录页统一查看预约与消费信息。",
  },
];

const isReservationTab = (tab: string) => {
  const reservationTabs = [
    "reservation",
    "reservation-treatSlots",
    "reservation-sterilizateSlots",
    "reservation-beautySlots",
    "reservation-SPASlots",
    "showSlots",
  ];
  return reservationTabs.includes(tab);
};

const activeServiceTitle = computed(() => {
  if (activeTab.value === "reservation-sterilizateSlots") return "宠物绝育预约";
  if (activeTab.value === "reservation-beautySlots") return "宠物美容预约";
  if (activeTab.value === "reservation-SPASlots") return "宠物 SPA 预约";
  return "宠物医治预约";
});

const normalizedSlotTab = computed(() =>
  activeTab.value === "showSlots" ? "showSlots" : "reservation-treatSlots"
);

const switchTab = (tab: string) => {
  if (
    tab === "reservation-sterilizateSlots" ||
    tab === "reservation-beautySlots" ||
    tab === "reservation-SPASlots"
  ) {
    activeTab.value = "reservation-treatSlots";
    return;
  }
  activeTab.value = tab;
};

const handleSubmitSuccess = () => {
  submitAfter.value = true;
};

const cancel = () => {
  activeTab.value = "reservation-treatSlots";
};

const close = () => {
  if (activeTab.value === "showSlots") {
    activeTab.value = "reservation-treatSlots";
    return;
  }
  activeTab.value = "reservation";
};

onMounted(() => {
  void reservationApi.getDoctorOptions().then((doctorList) => {
    doctorData.value = doctorList;
  });

  void reservationApi.getScheduleOptions().then((schedule) => {
    scheduleData.value = schedule;
  });
});
</script>

<style scoped lang="scss">
.services-page {
  display: grid;
  grid-template-columns: 250px minmax(0, 1fr);
  gap: 20px;
  min-height: 0;
  align-items: start;
  position: relative;
}

.services-sidebar {
  position: sticky;
  top: 128px;
  align-self: start;
  display: grid;
  gap: 10px;
  padding: 18px;
  border-radius: 30px;
  border: 1px solid rgba(21, 91, 92, 0.1);
  background: rgba(255, 250, 242, 0.8);
  box-shadow: 0 22px 50px rgba(29, 93, 95, 0.08);
}

.services-sidebar__item {
  display: grid;
  justify-items: start;
  gap: 4px;
  padding: 14px 16px;
  border: 1px solid transparent;
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.58);
  color: #163f42;
  cursor: pointer;
  text-align: left;
}

.services-sidebar__item span {
  font-size: 15px;
  font-weight: 700;
}

.services-sidebar__item small {
  color: #67817d;
  font-size: 12px;
}

.services-sidebar__item--active {
  border-color: rgba(29, 134, 135, 0.18);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.28),
    rgba(243, 197, 155, 0.22)
  );
  box-shadow: 0 16px 30px rgba(28, 98, 99, 0.1);
}

.services-stage {
  display: grid;
  gap: 18px;
  min-width: 0;
  min-height: 0;
}

.services-hero {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 360px;
  gap: 18px;
  padding: 26px;
  border-radius: 34px;
  border: 1px solid rgba(21, 91, 92, 0.1);
  background: rgba(255, 250, 242, 0.82);
  box-shadow: 0 24px 55px rgba(25, 92, 93, 0.08);
}

.services-hero__copy p,
.slot-shell__header p {
  margin: 0 0 8px;
  color: #1e8a88;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 12px;
  font-weight: 700;
}

.services-hero__copy h2 {
  margin: 0 0 10px;
  font-family: "ZCOOL XiaoWei", "Noto Serif SC", serif;
  font-size: clamp(34px, 4vw, 48px);
  line-height: 1.12;
  color: #143d40;
}

.services-hero__copy span,
.slot-shell__header span {
  color: #5d7875;
  font-size: 15px;
  line-height: 1.8;
}

.services-hero__metrics {
  display: grid;
  gap: 12px;
}

.services-hero__metrics article,
.service-card,
.after-sale-panel article,
.slot-shell {
  border-radius: 28px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: rgba(255, 255, 255, 0.62);
  box-shadow: 0 20px 45px rgba(25, 92, 93, 0.07);
}

.services-hero__metrics article {
  padding: 18px 20px;
}

.services-hero__metrics strong {
  display: block;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 34px;
  color: #166a69;
}

.services-hero__metrics span {
  color: #617a77;
  font-size: 13px;
}

.service-grid,
.after-sale-panel {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.service-card,
.after-sale-panel article {
  padding: 22px;
  text-align: left;
}

.service-card {
  cursor: pointer;
}

.service-card__badge,
.after-sale-panel p {
  display: inline-flex;
  margin-bottom: 12px;
  padding: 6px 10px;
  border-radius: 999px;
  background: rgba(29, 134, 135, 0.08);
  color: #177170;
  font-size: 12px;
  font-weight: 700;
}

.service-card strong,
.after-sale-panel strong,
.slot-shell__header h3 {
  display: block;
  margin-bottom: 10px;
  color: #163f42;
  font-size: 24px;
}

.service-card span,
.after-sale-panel span {
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.slot-shell {
  padding: 22px;
  overflow: visible;
}

.slot-shell__header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 16px;
  margin-bottom: 16px;
}

.slot-shell :deep(.reservation-slots) {
  min-width: 0;
  margin: 0;
  padding: 0;
  box-shadow: none;
  border-radius: 0;
}

.submit-mask {
  position: absolute;
  inset: 0;
  background: rgba(17, 56, 57, 0.08);
  pointer-events: none;
}

@media (max-width: 1100px) {
  .services-page,
  .services-hero,
  .service-grid,
  .after-sale-panel {
    grid-template-columns: 1fr;
  }

  .services-sidebar {
    position: static;
  }
}
</style>
