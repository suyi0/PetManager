<template>
  <section class="services-page">
    <div class="svc-head">
      <div class="svc-head__title">
        <h2>服务预约</h2>
        <p>选服务 → 选医生 → 选时段，预约成功后自动回到这里。</p>
      </div>
      <div class="seg">
        <button
          class="seg__btn"
          :class="{ 'seg__btn--active': isReservationTab(activeTab) }"
          @click="switchTab('reservation')"
        >
          预约服务
        </button>
        <button
          class="seg__btn"
          :class="{ 'seg__btn--active': activeTab === 'afterSale' }"
          @click="switchTab('afterSale')"
        >
          复诊服务
        </button>
      </div>
    </div>

    <section v-if="activeTab === 'reservation'" class="service-grid">
      <button
        v-for="service in serviceCards"
        :key="service.key"
        class="service-card"
        @click="selectService(service)"
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
        :pet-profiles="petProfiles"
        :service-type="serviceType"
        :schedule-data="scheduleData"
        :switchTab="switchTab"
        @close="close"
        @cancle="cancel"
        @submit-success="handleSubmitSuccess"
        @refresh-doctors="reloadDoctors"
      />
    </section>

    <div v-if="submitAfter" class="submit-mask"></div>
  </section>
</template>

<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from "vue";
import treatSlots from "@/modules/user/views/desktop/UserHome-module/HomeNav/Services-module/treatSlots.vue";
import { DoctorDataItem } from "@/modules/doctor/api/types";
import { ReservationScheduleState, PetProfile } from "@/modules/user/api/types";
import { petApi, reservationApi } from "@/modules/user/api/userApi";
import { subscribeReservationDoctors } from "@/modules/user/utils/reservationDoctorStream";

const activeTab = ref("reservation");
const submitAfter = ref(false);

/**
 * 预约服务卡片数据结构
 */
type ServiceCard = {
  key: string;
  tab: string;
  label: string;
  title: string;
  description: string;
};

const doctorData = ref<DoctorDataItem[]>([]);
const petProfiles = ref<PetProfile[]>([]);
const scheduleData = ref<Omit<ReservationScheduleState, "doctorData">>({
  year: [],
  month: [],
  day: [],
  weekday: [],
  slots: [],
});
let unsubscribeReservationDoctors: (() => void) | null = null;

const serviceCards: ServiceCard[] = [
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

const selectedService = ref<ServiceCard>(serviceCards[0]);

const serviceType = computed(() => selectedService.value.title);

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
  return `${selectedService.value.title}预约`;
});

const normalizedSlotTab = computed(() =>
  activeTab.value === "showSlots" ? "showSlots" : "reservation-treatSlots"
);

const selectService = (service: ServiceCard) => {
  selectedService.value = service;
  switchTab(service.tab);
};

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

const reloadDoctors = async () => {
  try {
    doctorData.value = await reservationApi.getDoctor();
  } catch {
    // 忽略刷新失败
  }
};

onMounted(async () => {
  const [doctors, schedule, pets] = await Promise.all([
    reservationApi.getDoctor(),
    reservationApi.getDate(),
    petApi.getPetProfiles(),
  ]);

  doctorData.value = doctors;
  scheduleData.value = schedule;
  petProfiles.value = pets;
  unsubscribeReservationDoctors = subscribeReservationDoctors(() => {
    void reloadDoctors();
  });
});

onUnmounted(() => {
  unsubscribeReservationDoctors?.();
  unsubscribeReservationDoctors = null;
});
</script>

<style scoped lang="scss">
.services-page {
  display: grid;
  gap: 16px;
  position: relative;
  min-height: 0;
}

.svc-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;
}

.svc-head__title h2 {
  margin: 0;
  font-size: 20px;
  font-weight: 800;
  letter-spacing: -0.01em;
  color: #1f3a36;
}

.svc-head__title p {
  margin: 4px 0 0;
  color: #6b7d77;
  font-size: 13px;
}

.seg {
  display: inline-flex;
  padding: 4px;
  gap: 2px;
  background: #ffffff;
  border: 1px solid #efe7dc;
  border-radius: 12px;
}

.seg__btn {
  height: 36px;
  padding: 0 18px;
  border: 0;
  border-radius: 9px;
  background: transparent;
  color: #6b7d77;
  font-size: 14px;
  font-weight: 700;
  cursor: pointer;
}

.seg__btn--active {
  background: #e7f5f1;
  color: #1f7a6c;
}

.service-grid,
.after-sale-panel {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.service-card,
.after-sale-panel article,
.slot-shell {
  border-radius: 16px;
  border: 1px solid #efe7dc;
  background: #ffffff;
  box-shadow: 0 8px 20px rgba(47, 158, 143, 0.06);
}

.service-card,
.after-sale-panel article {
  padding: 18px;
  text-align: left;
}

.service-card {
  cursor: pointer;
  transition: transform 0.15s ease, box-shadow 0.15s ease;
}

.service-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 12px 26px rgba(47, 158, 143, 0.1);
}

.service-card__badge,
.after-sale-panel p {
  display: inline-flex;
  margin-bottom: 10px;
  padding: 4px 10px;
  border-radius: 999px;
  background: #e7f5f1;
  color: #1f7a6c;
  font-size: 12px;
  font-weight: 700;
}

.service-card strong,
.after-sale-panel strong,
.slot-shell__header h3 {
  display: block;
  margin-bottom: 6px;
  color: #1f3a36;
  font-size: 18px;
}

.service-card span,
.after-sale-panel span {
  color: #6b7d77;
  line-height: 1.6;
  font-size: 13px;
}

.slot-shell {
  padding: 20px;
  overflow: visible;
}

.slot-shell__header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 16px;
  margin-bottom: 16px;
}

.slot-shell__header p {
  margin: 0 0 6px;
  color: #2f9e8f;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 12px;
  font-weight: 700;
}

.slot-shell__header span {
  color: #6b7d77;
  font-size: 13px;
  line-height: 1.6;
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
  background: rgba(31, 58, 54, 0.08);
  pointer-events: none;
}

@media (max-width: 1100px) {
  .service-grid,
  .after-sale-panel {
    grid-template-columns: 1fr;
  }

  .svc-head {
    flex-direction: column;
    align-items: stretch;
  }
}
</style>
