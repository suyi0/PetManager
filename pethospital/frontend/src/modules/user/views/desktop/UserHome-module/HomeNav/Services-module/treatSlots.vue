<template>
  <div class="reservation-slots">
    <button class="close-button" @click="close">关闭</button>

    <section
      v-if="props.activeTab === 'reservation-treatSlots'"
      class="reservation-slots-page"
    >
      <header class="panel-header">
        <div>
          <p>Step 01</p>
          <h3>选择医生</h3>
        </div>
        <span>先确定接诊医生，再进入下方时段选择。</span>
      </header>

      <div v-if="doctorData.length > 0" class="doctor-grid">
        <article
          v-for="doctor in doctorData"
          :key="doctor.id"
          class="doctor-card"
          :class="{
            'doctor-card--active': upDoctorId === doctor.id,
            'doctor-card--disabled': doctor.status !== 'online',
          }"
        >
          <div class="doctor-card__badge">{{ doctor.specialty || "全科" }}</div>
          <div
            class="doctor-card__status"
            :class="{
              'doctor-card__status--online': doctor.status === 'online',
              'doctor-card__status--offline': doctor.status !== 'online',
            }"
          >
            {{ formatDoctorStatus(doctor.status) }}
          </div>
          <strong>{{ doctor.name }}</strong>
          <span>
            {{
              doctor.status === "online"
                ? "当前医生已在线，可继续选择日期和时段。"
                : "当前医生已接入预约系统，可继续选择日期和时段。"
            }}
          </span>
          <button
            class="doctor-card__action"
            :disabled="doctor.status !== 'online'"
            @click="
              choiceDoctor(doctor);
              props.switchTab('showSlots');
            "
          >
            {{ doctor.status === "online" ? "选择该医生" : "当前不可预约" }}
          </button>
        </article>
      </div>

      <div v-else class="empty-state">
        <strong>暂时没有可预约医生</strong>
        <span>医生排班还未同步完成，请稍后再试。</span>
      </div>
    </section>

    <section
      v-if="props.activeTab === 'showSlots'"
      class="reservation-slots-page"
    >
      <header class="panel-header">
        <div>
          <p>Step 02</p>
          <h3>选择预约时间</h3>
        </div>
        <span>
          当前医生：{{ selectedDoctorName }}，请选择日期和一个可用时段后提交。
        </span>
      </header>

      <div v-if="availableDates.length > 0" class="date-grid">
        <button
          v-for="date in availableDates"
          :key="date.key"
          class="date-pill"
          :class="{ 'date-pill--active': dateTab === date.key }"
          @click="switchDate(date.key)"
        >
          <strong>{{ date.weekday }}</strong>
          <span>{{ date.year }}年 {{ date.month }}月{{ date.day }}日</span>
        </button>
      </div>

      <div v-else class="empty-state">
        <strong>暂无可用日期</strong>
        <span>预约日期还未生成，请稍后刷新或联系前台。</span>
      </div>

      <div v-if="selectedDate" class="slots-panel">
        <section class="slot-group">
          <div class="slot-group__header">
            <h4>上午</h4>
            <span>优先展示午前可预约时段</span>
          </div>
          <div class="slot-grid">
            <button
              v-for="slot in morningSlots"
              :key="slot.key"
              class="slot-button"
              :class="{ 'slot-button--active': choiceActive === slot.key }"
              :disabled="!slot.value"
              @click="switchChoice(slot.key)"
            >
              {{ slot.value || "暂无时段" }}
            </button>
          </div>
        </section>

        <section class="slot-group">
          <div class="slot-group__header">
            <h4>下午</h4>
            <span>午后到傍晚的可预约时段</span>
          </div>
          <div class="slot-grid">
            <button
              v-for="slot in afternoonSlots"
              :key="slot.key"
              class="slot-button"
              :class="{ 'slot-button--active': choiceActive === slot.key }"
              :disabled="!slot.value"
              @click="switchChoice(slot.key)"
            >
              {{ slot.value || "暂无时段" }}
            </button>
          </div>
        </section>
      </div>

      <footer class="reservation-actions">
        <button class="reservation-actions__ghost" @click="cancel">
          返回上一步
        </button>
        <button
          class="reservation-actions__primary"
          :disabled="!canSubmit"
          @click="selectPet"
        >
          确认预约
        </button>
      </footer>

      <div
        v-if="openSelectPetModal"
        class="selectPet-modal"
        role="dialog"
        aria-modal="true"
        aria-labelledby="select-pet-title"
      >
        <div class="selectPet-card">
          <span class="selectPet-card__eyebrow">Pet Selection</span>
          <h3 id="select-pet-title">本次给哪只宠物预约？</h3>

          <div v-if="petProfiles.length > 0" class="selectPet-list">
            <button
              v-for="pet in petProfiles"
              :key="pet.id"
              type="button"
              class="selectPet-option"
              :class="{ 'selectPet-option--active': selectedPetId === pet.id }"
              @click="selectedPetId = pet.id"
            >
              {{ pet.name }}
            </button>
          </div>

          <div v-else class="selectPet-empty">
            <strong>暂无宠物档案</strong>
            <span>请先在个人中心添加宠物后再预约。</span>
          </div>

          <p class="selectPet-current">
            已选：{{ selectedPetName || "未选择" }}
          </p>

          <div class="selectPet-footer">
            <button class="selectPet-footer__ghost" @click="cancelSelectPet">
              取消
            </button>
            <button
              class="selectPet-footer__primary"
              :disabled="!selectedPetId"
              @click="submit"
            >
              确定
            </button>
          </div>
        </div>
      </div>

      <div
        v-if="submitAfter"
        class="success-modal"
        role="dialog"
        aria-modal="true"
        aria-labelledby="reservation-success-title"
      >
        <div class="success-card">
          <h3 id="reservation-success-title">预约成功</h3>
          <div class="success-card__details">
            <div class="success-card__detail-row">
              <div>
                <span>预约用户</span>
                <strong>{{ reservationUserName }}</strong>
              </div>
              <div>
                <span>预约宠物</span>
                <strong>{{ selectedPetName }}</strong>
              </div>
            </div>
            <div>
              <span>预约医生</span>
              <strong>{{ selectedDoctorName }}</strong>
              <div class="success-card__detail-sign">
                <span>预约类型</span>
                <strong>{{ selectedServiceType }}</strong>
              </div>
            </div>
            <div>
              <span>预约时间</span>
              <strong>{{ reservationDateText }} {{ upSlot }}</strong>
            </div>
          </div>
          <button @click="removeSubmitAfter">确定</button>
        </div>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, watch } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { reservationApi } from "@/modules/user/api/userApi";
import { DoctorDataItem } from "@/modules/doctor/api/types";
import { ReservationScheduleState, PetProfile } from "@/modules/user/api/types";

const store = useStore(storeKey);

const props = defineProps<{
  activeTab: string;
  doctorData: DoctorDataItem[];
  petProfiles: PetProfile[];
  serviceType: string;
  scheduleData: Omit<ReservationScheduleState, "doctorData">;
  switchTab(_tab: string): void;
}>();

const emit = defineEmits(["close", "cancle", "submit-success"]);

interface Doctor {
  id: number;
  name: string;
  specialty?: string;
  status?: string;
}

type DateItem = {
  key: string;
  index: number;
  year: string;
  month: string;
  day: string;
  weekday: string;
  slots: string[];
};

type SlotItem = {
  key: string;
  value: string;
};

const serviceType = computed<string>(() =>
  props.serviceType != null ? String(props.serviceType) : ""
);

const doctorData = computed<Doctor[]>(() =>
  Array.isArray(props.doctorData) ? props.doctorData.filter(Boolean) : []
);

const formatDoctorStatus = (status?: string) => {
  if (status === "online") return "今日在线";
  if (status === "offline") return "今日离线";
  return "待排班";
};

/**
 * 获取可用日期数据
 * @returns DateItem[]
 */
const availableDates = computed<DateItem[]>(() => {
  const years = props.scheduleData.year || [];
  const months = props.scheduleData.month || [];
  const days = props.scheduleData.day || [];
  const weekdays = props.scheduleData.weekday || [];
  const slotGroups = props.scheduleData.slots || [];

  return years.map((year, index) => ({
    key: `date${index + 1}`,
    index,
    year,
    month: months[index] || "",
    day: days[index] || "",
    weekday: weekdays[index] || "",
    slots: slotGroups[index] || [],
  }));
});

const dateTab = ref("date1");
const choiceActive = ref("");
const upDoctorId = ref<number>(0);
const upYear = ref("");
const upMonth = ref("");
const upDay = ref("");
const upSlot = ref("");
const submitAfter = ref(false);
const openSelectPetModal = ref(false);
const selectedPetId = ref("");

const petProfiles = computed(() => props.petProfiles || []);

/**
 * 获取选中的宠物名称
 */
const selectedPetName = computed(() => {
  const target = petProfiles.value.find(
    (pet) => pet.id === selectedPetId.value
  );
  return target?.name || "";
});

/**
 * 获取选中的日期数据
 */
const selectedDate = computed(
  () => availableDates.value.find((item) => item.key === dateTab.value) || null
);

/**
 * 获取时间段的开始时间
 * @param slot 时间段字符串，格式示例："9:00-9:30"
 * @returns 开始时间字符串，格式示例："09:00"
 */
const getSlotStartTime = (slot: string) => {
  const matched = slot.match(/\d{1,2}:\d{2}/)?.[0] ?? "";
  const [hour = "", minute = ""] = matched.split(":");
  if (!hour || !minute) return "";
  return `${hour.padStart(2, "0")}:${minute}`;
};

/**
 * 获取当前选中天数的上午时间段
 */
const selectedMorningDate = computed(() => {
  const currentDate = availableDates.value.find(
    (item) => item.key === dateTab.value
  );
  if (currentDate) {
    const hasMorningSlot = currentDate.slots.some(
      (slot) => getSlotStartTime(slot) && getSlotStartTime(slot) < "12:00"
    );
    return hasMorningSlot ? currentDate : null;
  }
  return null;
});

/**
 * 获取当前选中天数的下午时间段
 */
const selectedAfternoonDate = computed(() => {
  const currentDate = availableDates.value.find(
    (item) => item.key === dateTab.value
  );
  if (currentDate) {
    const hasAfternoonSlot = currentDate.slots.some(
      (slot) => getSlotStartTime(slot) >= "12:00"
    );
    return hasAfternoonSlot ? currentDate : null;
  }
  return null;
});

/**
 * 获取当前选中的医生名称
 */
const selectedDoctorName = computed(() => {
  const target = doctorData.value.find(
    (doctor) => doctor.id === upDoctorId.value
  );
  return target?.name || "未选择";
});

/**
 * 获取当前选中的服务类型
 */
const selectedServiceType = computed(() => {
  const target = serviceType.value;
  return target || "未选择";
});

/**
 * 获取当前用户名称
 */
const reservationUserName = computed(
  () =>
    store.getters["auth/formattedUserName"] ||
    store.state.currentUser.userEmail ||
    store.state.currentUser.userPhone ||
    "当前用户"
);

/**
 * 获取当前用户预约的日期文本
 */
const reservationDateText = computed(() => {
  if (!upYear.value || !upMonth.value || !upDay.value) {
    return "";
  }

  return `${upYear.value}年${upMonth.value}月${upDay.value}日`;
});

const splitSlots = (slots: string[]) => {
  const normalized = slots.filter(Boolean);

  return {
    morning: normalized.filter((slot) => {
      const startTime = getSlotStartTime(slot);
      return Boolean(startTime && startTime < "12:00");
    }),
    afternoon: normalized.filter((slot) => {
      const startTime = getSlotStartTime(slot);
      return Boolean(startTime && startTime >= "12:00");
    }),
  };
};

/**
 * 获取当前选中的日期的预约信息(上午)
 */
const morningSlots = computed<SlotItem[]>(() => {
  const current = selectedMorningDate.value;
  if (!current) return [];

  return splitSlots(current.slots).morning.map((value, index) => ({
    key: `morning-${index}`,
    value,
  }));
});

/**
 * 获取当前选中的日期的预约信息(下午)
 */
const afternoonSlots = computed<SlotItem[]>(() => {
  const current = selectedAfternoonDate.value;
  if (!current) return [];

  return splitSlots(current.slots).afternoon.map((value, index) => ({
    key: `afternoon-${index}`,
    value,
  }));
});

/**
 * 提交按钮是否可用
 */
const canSubmit = computed(() =>
  Boolean(
    upDoctorId.value &&
      selectedDate.value &&
      selectedPetId.value &&
      getChosenSlotValue()
  )
);

function close() {
  emit("close");
}

function cancel() {
  emit("cancle");
}

function choiceDoctor(doctor: Doctor) {
  upDoctorId.value = doctor.id;
  if (!selectedPetId.value && petProfiles.value.length > 0) {
    selectedPetId.value = petProfiles.value[0].id;
  }
}

function switchDate(tab: string) {
  dateTab.value = tab;
  choiceActive.value = "";
}

function switchChoice(tab: string) {
  if (choiceActive.value === tab) {
    choiceActive.value = "";
    return;
  }
  choiceActive.value = tab;
}

function getChosenSlotValue() {
  const allSlots = [...morningSlots.value, ...afternoonSlots.value];
  return allSlots.find((slot) => slot.key === choiceActive.value)?.value || "";
}

async function selectPet() {
  openSelectPetModal.value = true;
}

async function cancelSelectPet() {
  openSelectPetModal.value = false;
}

/**
 * 提交预约操作
 */
async function submit() {
  if (!selectedDate.value) return;

  const selectedSlot = getChosenSlotValue();
  const petId = Number(selectedPetId.value || 0);
  if (!selectedSlot || !upDoctorId.value || !petId) return;

  upYear.value = selectedDate.value.year;
  upMonth.value = selectedDate.value.month;
  upDay.value = selectedDate.value.day;
  upSlot.value = selectedSlot;

  try {
    const response = await reservationApi.record({
      name: store.state.currentUser.userName,
      phone: store.state.currentUser.userPhone,
      email: store.state.currentUser.userEmail,
      doctorId: upDoctorId.value,
      petId,
      reservationType: selectedServiceType.value,
      date: `${upYear.value}-${upMonth.value}-${upDay.value}`,
      slot: upSlot.value,
    });

    if (response?.data?.success && response.status === 200) {
      /**
       * 预约成功后主动让记录类缓存失效，用户回到订单页时会拿到最新数据。
       */
      store.commit("userPortal/markReservationRecordsDirty");
      store.commit("userPortal/markOrderSummariesDirty");
      openSelectPetModal.value = false;
      submitAfter.value = true;
      emit("submit-success", {
        year: upYear.value,
        month: upMonth.value,
        day: upDay.value,
        slot: upSlot.value,
      });
    }
  } catch (error) {
    console.error("预约提交失败:", error);
  }
}

function removeSubmitAfter() {
  submitAfter.value = false;
  upDoctorId.value = 0;
  choiceActive.value = "";
  props.switchTab("reservation");
}

watch(
  petProfiles,
  (pets) => {
    if (!selectedPetId.value && pets.length > 0) {
      selectedPetId.value = pets[0].id;
    }
  },
  { immediate: true }
);
</script>

<style scoped lang="scss">
.reservation-slots {
  position: relative;
  display: grid;
  gap: 18px;
}

.close-button {
  justify-self: end;
  padding: 10px 14px;
  border: none;
  border-radius: 999px;
  background: rgba(29, 134, 135, 0.08);
  color: #165f61;
  font-weight: 700;
  cursor: pointer;
}

.reservation-slots-page {
  display: grid;
  gap: 18px;
}

.panel-header,
.doctor-card,
.slot-group {
  border-radius: 26px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: rgba(255, 255, 255, 0.62);
  box-shadow: 0 18px 40px rgba(25, 92, 93, 0.07);
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 16px;
  padding: 20px 22px;
}

.panel-header p,
.doctor-card__badge {
  margin: 0 0 8px;
  color: #1e8a88;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 12px;
  font-weight: 700;
}

.panel-header h3 {
  margin: 0;
  color: #143d40;
  font-size: 28px;
}

.panel-header span {
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.doctor-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 16px;
}

.doctor-card {
  display: grid;
  gap: 12px;
  padding: 22px;
}

.doctor-card__status {
  display: inline-flex;
  align-items: center;
  width: fit-content;
  padding: 6px 12px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.04em;
}

.doctor-card__status--online {
  background: rgba(49, 173, 109, 0.12);
  color: #1e8a57;
}

.doctor-card__status--offline {
  background: rgba(29, 134, 135, 0.08);
  color: #4c6f73;
}

.doctor-card--active {
  border-color: rgba(29, 134, 135, 0.22);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.22),
    rgba(243, 197, 155, 0.16)
  );
}

.doctor-card--disabled {
  opacity: 0.74;
}

.doctor-card strong,
.empty-state strong,
.slot-group h4,
.success-card h3 {
  background: linear-gradient(135deg, #21928b, #8fbf88);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.doctor-card strong {
  font-size: 24px;
}

.doctor-card span,
.empty-state span,
.slot-group__header span {
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.doctor-card__action,
.reservation-actions__ghost,
.reservation-actions__primary,
.success-card button {
  padding: 12px 16px;
  border: none;
  border-radius: 16px;
  cursor: pointer;
  font-weight: 700;
}

.doctor-card__action,
.reservation-actions__ghost {
  background: rgba(29, 134, 135, 0.08);
  color: #166968;
}

.doctor-card__action:disabled {
  background: rgba(120, 138, 145, 0.12);
  color: #7c8c95;
  cursor: not-allowed;
}

.date-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
  gap: 12px;
}

.date-pill {
  display: grid;
  gap: 4px;
  padding: 16px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  border-radius: 22px;
  background: rgba(255, 255, 255, 0.62);
  color: #173f42;
  text-align: left;
  cursor: pointer;
  box-shadow: 0 18px 40px rgba(25, 92, 93, 0.06);
}

.date-pill strong {
  font-size: 18px;
}

.date-pill span {
  color: #68817e;
  font-size: 13px;
}

.date-pill--active {
  border-color: rgba(29, 134, 135, 0.2);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.24),
    rgba(243, 197, 155, 0.18)
  );
}

.slots-panel {
  display: grid;
  gap: 16px;
}

.slot-group {
  padding: 20px;
}

.slot-group__header {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  margin-bottom: 14px;
}

.slot-group h4 {
  margin: 0;
  font-size: 20px;
}

.slot-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  gap: 12px;
}

.slot-button {
  min-height: 52px;
  padding: 12px 16px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.74);
  color: #173f42;
  cursor: pointer;
}

.slot-button--active {
  border-color: rgba(29, 134, 135, 0.22);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.24),
    rgba(243, 197, 155, 0.18)
  );
  color: #14595c;
  font-weight: 700;
}

.slot-button:disabled {
  cursor: not-allowed;
  opacity: 0.48;
}

.reservation-actions {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
}

.reservation-actions__primary {
  background: linear-gradient(135deg, #268f90, #156b6b);
  color: #fffdf7;
}

.reservation-actions__primary:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.success-modal {
  position: fixed;
  inset: 0;
  z-index: 30;
  display: grid;
  place-items: center;
  padding: 24px;
  background: rgba(12, 38, 39, 0.28);
  backdrop-filter: blur(10px);
}

.success-card {
  width: min(460px, 100%);
  padding: 26px;
  border: 1px solid rgba(21, 91, 92, 0.12);
  border-radius: 26px;
  background: radial-gradient(
      circle at left top,
      rgba(136, 214, 206, 0.26),
      transparent 45%
    ),
    linear-gradient(
      135deg,
      rgba(255, 253, 248, 0.98),
      rgba(246, 251, 248, 0.96)
    );
  box-shadow: 0 30px 80px rgba(25, 92, 93, 0.24);
  text-align: center;
}

.success-card__eyebrow {
  display: block;
  margin-bottom: 8px;
  color: #1e8a88;
  font-size: 12px;
  font-weight: 800;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.success-card h3 {
  margin: 0 0 8px;
  font-size: 28px;
}

.success-card__details {
  display: grid;
  gap: 12px;
  margin: 18px 0 20px;
  text-align: left;
}

.success-card__details div {
  display: grid;
  gap: 6px;
  padding: 14px 16px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.68);
  text-align: center;
}

.success-card__details > .success-card__detail-row {
  grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
  text-align: center;
  border: none;
  background: rgba(255, 255, 255, 0);
  padding: 0;
}

.success-card__details .success-card__detail-sign {
  position: absolute;
  top: 106px;
  right: 530px;
  border: none;
  background: rgba(255, 255, 255, 0);
  padding: 12px;

  span {
    background: linear-gradient(135deg, #6a96c5, #f87e7e);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
  }

  strong {
    background: linear-gradient(135deg, #6a96c5, #f87e7e);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
  }
}

.success-card__details span {
  color: #6a807d;
  font-size: 12px;
  font-weight: 700;
}

.success-card__details strong {
  color: #153f42;
  font-size: 18px;
  line-height: 1.5;
}

.success-card button {
  background: linear-gradient(135deg, #e89a79, #d46f58);
  color: #fffaf6;
}

.empty-state {
  padding: 32px 22px;
  border-radius: 26px;
  background: rgba(255, 255, 255, 0.58);
  text-align: center;
}

.selectPet-modal {
  position: fixed;
  inset: 0;
  z-index: 40;
  display: grid;
  place-items: center;
  padding: 24px;
  background: rgba(12, 38, 39, 0.34);
  backdrop-filter: blur(10px);
}

.selectPet-card {
  width: min(520px, 100%);
  display: grid;
  gap: 18px;
  padding: 28px;
  border: 1px solid rgba(21, 91, 92, 0.12);
  border-radius: 26px;
  background: radial-gradient(
      circle at left top,
      rgba(136, 214, 206, 0.24),
      transparent 44%
    ),
    linear-gradient(
      135deg,
      rgba(255, 253, 248, 0.98),
      rgba(246, 251, 248, 0.96)
    );
  box-shadow: 0 34px 88px rgba(25, 92, 93, 0.28);
}

.selectPet-card__eyebrow {
  color: #1e8a88;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 12px;
  font-weight: 800;
}

.selectPet-card h3 {
  margin: 0;
  color: #163f42;
  font-size: 28px;
  line-height: 1.25;
}

.selectPet-list {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  max-height: 180px;
  overflow-y: auto;
  padding: 2px;
}

.selectPet-option {
  min-width: 96px;
  min-height: 48px;
  padding: 12px 18px;
  border: 1px solid rgba(21, 91, 92, 0.1);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.76);
  color: #173f42;
  cursor: pointer;
  font-size: 16px;
  font-weight: 800;
  transition: transform 0.18s ease, border-color 0.18s ease,
    background 0.18s ease, box-shadow 0.18s ease;
}

.selectPet-option:hover {
  transform: translateY(-1px);
  border-color: rgba(29, 134, 135, 0.22);
  box-shadow: 0 14px 28px rgba(25, 92, 93, 0.1);
}

.selectPet-option--active {
  border-color: rgba(29, 134, 135, 0.3);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.34),
    rgba(243, 197, 155, 0.24)
  );
  color: #14595c;
}

.selectPet-empty {
  display: grid;
  gap: 8px;
  padding: 18px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.68);
  text-align: center;
}

.selectPet-empty strong {
  color: #163f42;
}

.selectPet-empty span,
.selectPet-current {
  color: #607975;
  line-height: 1.7;
  font-size: 14px;
}

.selectPet-current {
  margin: 0;
  font-weight: 700;
}

.selectPet-footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
}

.selectPet-footer button {
  min-width: 104px;
  min-height: 48px;
  padding: 12px 18px;
  border: none;
  border-radius: 16px;
  cursor: pointer;
  font-weight: 800;
}

.selectPet-footer__ghost {
  background: rgba(29, 134, 135, 0.08);
  color: #166968;
}

.selectPet-footer__primary {
  background: linear-gradient(135deg, #268f90, #156b6b);
  color: #fffdf7;
}

.selectPet-footer__primary:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

@media (max-width: 960px) {
  .doctor-grid {
    grid-template-columns: 1fr;
  }

  .panel-header,
  .slot-group__header,
  .reservation-actions,
  .selectPet-footer {
    flex-direction: column;
    align-items: stretch;
  }

  .selectPet-card {
    padding: 22px;
  }
}
</style>
