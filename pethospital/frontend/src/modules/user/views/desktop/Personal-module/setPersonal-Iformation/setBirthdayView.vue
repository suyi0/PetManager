<template>
  <div class="birthday-editor">
    <div class="birthday-editor__head">
      <div>
        <p>Birthday Studio</p>
        <h3>更新出生日期</h3>
        <span>
          生日信息会出现在个人资料里，也能帮助系统在服务推荐和资料完整度上给出更准确的提示。
        </span>
      </div>
      <button class="birthday-editor__ghost" @click="close">关闭</button>
    </div>

    <section class="birthday-editor__hero">
      <div class="birthday-editor__calendar">
        <span>{{ monthLabel }}</span>
        <strong>{{ dayLabel }}</strong>
        <small>{{ yearLabel }}</small>
      </div>
      <div class="birthday-editor__summary">
        <small>当前日期预览</small>
        <strong>{{ birthdayPreview }}</strong>
        <span>建议按实际生日填写，保存后会自动同步回个人中心总览卡片。</span>
      </div>
    </section>

    <form class="birthday-editor__form" @submit.prevent="saveBirthday">
      <label class="editor-field">
        <span>年份</span>
        <select v-model="year" @change="isButtonActive = true">
          <option v-for="y in years" :key="y" :value="String(y)">
            {{ y }}
          </option>
        </select>
      </label>

      <label class="editor-field">
        <span>月份</span>
        <select v-model="month" @change="isButtonActive = true">
          <option v-for="m in months" :key="m" :value="String(m)">
            {{ m }}
          </option>
        </select>
      </label>

      <label class="editor-field">
        <span>日期</span>
        <select v-model="day" @change="isButtonActive = true">
          <option v-for="d in days" :key="d" :value="String(d)">
            {{ d }}
          </option>
        </select>
      </label>

      <div class="birthday-editor__tips">
        <article>
          <small>ISO 格式</small>
          <strong>{{ birthdayPreview }}</strong>
        </article>
        <article>
          <small>备注</small>
          <span>月份和日期会自动补零，提交后会以 `YYYY-MM-DD` 形式存储。</span>
        </article>
      </div>

      <div v-if="isButtonActive" class="birthday-editor__actions">
        <button type="button" class="birthday-editor__ghost" @click="cancel">
          取消
        </button>
        <button type="submit" class="birthday-editor__primary">保存生日</button>
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

const monthLabel = computed(() => {
  return `${String(month.value || "--").padStart(2, "0")} 月`;
});
const dayLabel = computed(() => String(day.value || "--").padStart(2, "0"));
const yearLabel = computed(() => `${year.value || "----"} 年`);
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

<style scoped lang="scss">
.birthday-editor {
  display: grid;
  gap: 18px;
  padding: 24px;
  border-radius: 30px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: rgba(255, 255, 255, 0.76);
  box-shadow: 0 18px 44px rgba(24, 90, 91, 0.06);
}

.birthday-editor__head {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 16px;
}

.birthday-editor__head p,
.birthday-editor__summary small,
.birthday-editor__tips small {
  margin: 0;
  color: #1f8e89;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.birthday-editor__head h3 {
  margin: 6px 0 0;
  color: #133f42;
  font-size: 32px;
}

.birthday-editor__head span,
.birthday-editor__summary span,
.birthday-editor__tips span {
  display: block;
  margin-top: 10px;
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.birthday-editor__hero {
  display: grid;
  grid-template-columns: 130px minmax(0, 1fr);
  gap: 18px;
  padding: 22px;
  border-radius: 28px;
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.24),
    rgba(243, 197, 155, 0.18)
  );
}

.birthday-editor__calendar {
  display: grid;
  place-items: center;
  align-content: center;
  gap: 6px;
  padding: 16px;
  border-radius: 28px;
  background: linear-gradient(135deg, #91ddd2, #f0c29b);
  color: #15474a;
  box-shadow: 0 18px 34px rgba(28, 98, 99, 0.14);
}

.birthday-editor__calendar span,
.birthday-editor__calendar small {
  margin: 0;
  color: #15474a;
  line-height: 1;
}

.birthday-editor__calendar span {
  font-size: 13px;
  font-weight: 700;
}

.birthday-editor__calendar strong {
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 54px;
  line-height: 1;
}

.birthday-editor__calendar small {
  font-size: 12px;
  opacity: 0.9;
}

.birthday-editor__summary {
  display: grid;
  align-content: center;
  gap: 6px;
}

.birthday-editor__summary strong,
.birthday-editor__tips strong {
  color: #143f42;
  font-size: 26px;
}

.birthday-editor__form {
  display: grid;
  gap: 14px;
}

.editor-field {
  display: grid;
  gap: 8px;
}

.editor-field span {
  color: #24484b;
  font-size: 13px;
  font-weight: 700;
}

.editor-field select {
  width: 100%;
  padding: 13px 14px;
  border: 1px solid rgba(20, 82, 84, 0.12);
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.94);
  color: #173f42;
  font-size: 14px;
}

.editor-field select:focus {
  outline: none;
  border-color: rgba(24, 128, 127, 0.5);
  box-shadow: 0 0 0 4px rgba(141, 218, 210, 0.18);
}

.birthday-editor__tips {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.birthday-editor__tips article {
  padding: 16px 18px;
  border-radius: 18px;
  background: rgba(255, 249, 242, 0.95);
}

.birthday-editor__actions {
  display: flex;
  gap: 10px;
  justify-content: flex-end;
  flex-wrap: wrap;
}

.birthday-editor__ghost,
.birthday-editor__primary {
  border: none;
  border-radius: 999px;
  padding: 12px 16px;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.birthday-editor__ghost {
  background: rgba(20, 82, 84, 0.08);
  color: #154144;
}

.birthday-editor__primary {
  background: linear-gradient(135deg, #167f80, #2ca7a4);
  color: #fff;
  box-shadow: 0 16px 30px rgba(23, 104, 105, 0.22);
}

@media (max-width: 900px) {
  .birthday-editor__head,
  .birthday-editor__hero,
  .birthday-editor__tips {
    grid-template-columns: 1fr;
    display: grid;
  }
}
</style>
