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
          :class="{ 'doctor-card--active': upDoctorId === doctor.id }"
        >
          <div class="doctor-card__badge">{{ doctor.specialty || "全科" }}</div>
          <strong>{{ doctor.name }}</strong>
          <span>已接入预约系统，可继续选择日期和时段。</span>
          <button
            class="doctor-card__action"
            @click="
              choiceDoctor(doctor);
              props.switchTab('showSlots');
            "
          >
            选择该医生
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
          @click="submit"
        >
          确认预约
        </button>
      </footer>

      <div v-if="submitAfter" class="success-card">
        <h3>预约成功</h3>
        <p>您已成功预约 {{ upYear }}-{{ upMonth }}-{{ upDay }} {{ upSlot }}</p>
        <button @click="removeSubmitAfter">确定</button>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";

const store = useStore(storeKey);

const props = defineProps<{
  activeTab: string;
  switchTab(_tab: string): void;
}>();

const emit = defineEmits(["close", "cancle", "submit-success"]);

interface Doctor {
  id: number;
  name: string;
  specialty?: string;
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

const doctorData = computed<Doctor[]>(() => {
  const data1 = {
    id: 1,
    name: "张三",
    specialty: "全科",
  };
  const data = store.state.reservation.doctorData;
  return Array.isArray(data) ? data.filter(Boolean) : [data1];
});

const availableDates = computed<DateItem[]>(() => {
  const years = store.state.reservation.year || [];
  const months = store.state.reservation.month || [];
  const days = store.state.reservation.day || [];
  const weekdays = store.state.reservation.weekday || [];
  const slotGroups = store.state.reservation.slots || [];

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

const selectedDate = computed(
  () => availableDates.value.find((item) => item.key === dateTab.value) || null
);

const selectedDoctorName = computed(() => {
  const target = doctorData.value.find(
    (doctor) => doctor.id === upDoctorId.value
  );
  return target?.name || "未选择";
});

const splitSlots = (slots: string[]) => {
  const normalized = slots.filter(Boolean);
  const noonIndex = normalized.findIndex(
    (slot) => slot.slice(6, 11) === "12:00"
  );
  const divider =
    noonIndex === -1 ? Math.min(3, normalized.length) : noonIndex + 1;

  return {
    morning: normalized.slice(0, divider),
    afternoon: normalized.slice(divider),
  };
};

const morningSlots = computed<SlotItem[]>(() => {
  const current = selectedDate.value;
  if (!current) return [];
  return splitSlots(current.slots).morning.map((value, index) => ({
    key: `morning-${index}`,
    value,
  }));
});

const afternoonSlots = computed<SlotItem[]>(() => {
  const current = selectedDate.value;
  if (!current) return [];
  return splitSlots(current.slots).afternoon.map((value, index) => ({
    key: `afternoon-${index}`,
    value,
  }));
});

const canSubmit = computed(() =>
  Boolean(upDoctorId.value && selectedDate.value && getChosenSlotValue())
);

function close() {
  emit("close");
}

function cancel() {
  emit("cancle");
}

function choiceDoctor(doctor: Doctor) {
  upDoctorId.value = doctor.id;
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

async function submit() {
  if (!selectedDate.value) return;

  const selectedSlot = getChosenSlotValue();
  if (!selectedSlot || !upDoctorId.value) return;

  upYear.value = selectedDate.value.year;
  upMonth.value = selectedDate.value.month;
  upDay.value = selectedDate.value.day;
  upSlot.value = selectedSlot;

  try {
    const response = await store.dispatch(
      "reservation/createReservationRecord",
      {
        upDoctorId: upDoctorId.value,
        upYear: upYear.value,
        upMonth: upMonth.value,
        upDay: upDay.value,
        upSlot: upSlot.value,
      }
    );

    if (response?.data?.success && response.status === 200) {
      submitAfter.value = true;
      emit("submit-success", {
        year: upYear.value,
        month: upMonth.value,
        day: upDay.value,
        slot: upSlot.value,
      });
      upDoctorId.value = 0;
      choiceActive.value = "";
    }
  } catch (error) {
    console.error("预约提交失败:", error);
  }
}

function removeSubmitAfter() {
  submitAfter.value = false;
  props.switchTab("reservation");
}
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
.slot-group,
.success-card {
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

.doctor-card--active {
  border-color: rgba(29, 134, 135, 0.22);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.22),
    rgba(243, 197, 155, 0.16)
  );
}

.doctor-card strong,
.empty-state strong,
.slot-group h4,
.success-card h3 {
  color: #163f42;
}

.doctor-card strong {
  font-size: 24px;
}

.doctor-card span,
.empty-state span,
.slot-group__header span,
.success-card p {
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

.success-card {
  padding: 22px;
  text-align: center;
}

.success-card h3 {
  margin: 0 0 8px;
  font-size: 28px;
}

.success-card p {
  margin: 0 0 16px;
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

@media (max-width: 960px) {
  .doctor-grid {
    grid-template-columns: 1fr;
  }

  .panel-header,
  .slot-group__header,
  .reservation-actions {
    flex-direction: column;
    align-items: stretch;
  }
}
</style>
