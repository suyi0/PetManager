<template>
  <div class="pc-panel">
    <div class="pc-panel__head">
      <div>
        <h3>更新出生日期</h3>
        <p>
          生日会展示在个人资料中，并用于服务推荐与资料完整度提示。保存后以
          YYYY-MM-DD 形式存储。
        </p>
      </div>
      <button type="button" class="pc-btn pc-btn--ghost" @click="close">
        关闭
      </button>
    </div>

    <div class="pc-current">
      <span class="pc-current__lbl">当前日期</span>
      <span class="pc-current__val">{{ birthdayPreview }}</span>
    </div>

    <form class="pc-form" @submit.prevent="saveBirthday">
      <div class="bd-grid">
        <label class="pc-field">
          <span>年份</span>
          <select v-model="year" @change="isButtonActive = true">
            <option v-for="y in years" :key="y" :value="String(y)">
              {{ y }}
            </option>
          </select>
        </label>

        <label class="pc-field">
          <span>月份</span>
          <select v-model="month" @change="isButtonActive = true">
            <option v-for="m in months" :key="m" :value="String(m)">
              {{ m }}
            </option>
          </select>
        </label>

        <label class="pc-field">
          <span>日期</span>
          <select v-model="day" @change="isButtonActive = true">
            <option v-for="d in days" :key="d" :value="String(d)">
              {{ d }}
            </option>
          </select>
        </label>
      </div>

      <div class="pc-helper">
        <span class="pc-dot"></span>
        将保存为：{{ birthdayPreview }}
      </div>

      <div v-if="isButtonActive" class="pc-actions">
        <button type="button" class="pc-btn pc-btn--secondary" @click="cancel">
          取消
        </button>
        <button type="submit" class="pc-btn pc-btn--primary">保存生日</button>
      </div>
    </form>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref, watch } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";

const store = useStore(storeKey);
const emit = defineEmits(["close", "submit"]);

const isButtonActive = ref(false);
const year = ref("");
const month = ref("");
const day = ref("");
const years = ref<number[]>([]);
const months = ref([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]);
const days = ref<number[]>([]);

const currentYear = computed(() => new Date().getFullYear());

const birthdayPreview = computed(() => {
  const y = year.value || "----";
  const m = String(month.value || "--").padStart(2, "0");
  const d = String(day.value || "--").padStart(2, "0");
  return `${y}-${m}-${d}`;
});

watch(year, () => {
  updateDays();
});

watch(month, () => {
  updateDays();
});

function initYears() {
  years.value = [];
  for (let i = currentYear.value; i >= 1900; i--) {
    years.value.push(i);
  }
}

function getDaysInMonth(targetYear: number, targetMonth: number) {
  return new Date(targetYear, targetMonth, 0).getDate();
}

function updateDays() {
  days.value = [];
  const daysInMonth = getDaysInMonth(
    parseInt(year.value, 10) || currentYear.value,
    parseInt(month.value, 10) || 1
  );
  for (let i = 1; i <= daysInMonth; i++) {
    days.value.push(i);
  }

  if (day.value && Number(day.value) > daysInMonth) {
    day.value = String(daysInMonth);
  }
}

function parseDateString(dateString: string) {
  if (!dateString) return;

  const [y, m, d] = dateString.split("-");
  year.value = y || "";
  month.value = String(Number(m || 1));
  day.value = String(Number(d || 1));
}

function saveBirthday() {
  const y = year.value;
  const m = String(month.value).padStart(2, "0");
  const d = String(day.value).padStart(2, "0");

  emit("submit", {
    field: "userBirthday",
    birthday: `${y}-${m}-${d}`,
  });
  isButtonActive.value = false;
}

function cancel() {
  isButtonActive.value = false;
  close();
}

function close() {
  emit("close");
}

onMounted(() => {
  initYears();

  const savedBirthday = store.state.currentUser.userBirthday;
  if (savedBirthday) {
    parseDateString(savedBirthday);
  } else {
    year.value = String(currentYear.value);
    month.value = "1";
    day.value = "1";
  }

  updateDays();
});
</script>

<style scoped>
.bd-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
}
@media (max-width: 560px) {
  .bd-grid {
    grid-template-columns: 1fr;
  }
}
</style>
