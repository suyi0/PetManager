<template>
  <div class="set-birthday">
    <!-- 关闭按钮 -->
    <div class="close-button" @click="close">
      <svg
        width="18"
        height="18"
        viewBox="0 0 18 18"
        fill="none"
        xmlns="http://www.w3.org/2000/svg"
      >
        <path
          d="M14 2L2 14M14 14L2 2"
          stroke="#333"
          stroke-width="2"
          stroke-linecap="round"
          stroke-linejoin="round"
        />
      </svg>
    </div>

    <!-- 页面标题 -->
    <div class="title-container">
      <div class="icon">
        <svg
          width="40"
          height="40"
          viewBox="0 0 40 40"
          fill="none"
          xmlns="http://www.w3.org/2000/svg"
        >
          <rect x="8" y="8" width="24" height="24" rx="4" fill="#409EFF" />
          <path d="M12 12H16V16H12V12Z" fill="white" />
          <path d="M12 20H16V24H12V20Z" fill="white" />
          <path d="M20 12H24V16H20V12Z" fill="white" />
          <path d="M20 20H24V24H20V20Z" fill="white" />
          <path d="M12 8H16V12H12V8Z" fill="white" />
          <path d="M20 8H24V12H20V8Z" fill="white" />
        </svg>
      </div>
      <h2>出生日期</h2>
      <p class="description">你的出生日期用于确定符合条件的服务。</p>
    </div>

    <!-- 日期选择器 -->
    <div class="date-selectors">
      <div class="selector-group">
        <label class="selector-label">年</label>
        <select
          v-model="year"
          class="selector-field"
          @change="isButtonActive = true"
        >
          <option v-for="y in years" :key="y" :value="y">{{ y }}</option>
          <!-- v-for="y in years" - 这是 Vue 的循环指令，会遍历 years 数组中的每个元素
                :key="y" - Vue 的特殊属性，用于优化渲染，这里使用年份值作为唯一标识
                :value="y" - 将选项的值设置为年份值
                {{ y }} - 在选项中显示年份文本 -->
        </select>
      </div>

      <div class="selector-group">
        <label class="selector-label">月</label>
        <select
          v-model="month"
          class="selector-field"
          @change="isButtonActive = true"
        >
          <option v-for="m in months" :key="m" :value="m">{{ m }}</option>
        </select>
      </div>

      <div class="selector-group">
        <label class="selector-label">日</label>
        <select
          v-model="day"
          class="selector-field"
          @change="isButtonActive = true"
        >
          <option v-for="d in days" :key="d" :value="d">{{ d }}</option>
        </select>
      </div>
    </div>
    <!-- 按钮组 -->
    <div v-show="isButtonActive" class="button-group">
      <button @click="cancel" class="cancel-button">取消</button>
      <button @click="saveBirthday" class="save-button">存储</button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from "vue";
import { useStore } from "vuex";
import { key } from "../../store";

const store = useStore(key);

// 定义 emits
const emit = defineEmits(["close"]);

// 响应式数据
const isButtonActive = ref(false);
const year = ref("");
const month = ref("");
const day = ref("");
const years = ref<number[]>([]);
const months = ref([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]);
const days = ref<number[]>([]);

// 计算属性
const currentYear = computed(() => {
  return new Date().getFullYear();
});

// 监听器
watch(year, () => {
  updateDays();
});

watch(month, () => {
  updateDays();
});

// 初始化年份选项
function initYears() {
  for (let i = currentYear.value; i >= 1900; i--) {
    years.value.push(i);
  }
}

// 更新天数选项
function updateDays() {
  days.value = [];
  const daysInMonth = getDaysInMonth(
    parseInt(year.value) || 0,
    parseInt(month.value) || 0
  );
  for (let i = 1; i <= daysInMonth; i++) {
    days.value.push(i);
  }
}

// 获取指定年月的天数
function getDaysInMonth(year: number, month: number) {
  return new Date(year, month, 0).getDate();
}

// 从日期字符串中提取年月日
function parseDateString(dateString: string) {
  if (!dateString) return;

  const [y, m, d] = dateString.split("-");
  year.value = y;
  month.value = m;
  day.value = d;
}

function saveBirthday() {
  // 确保月份和日期都有前导零
  const y = year.value;
  const m = month.value.toString().padStart(2, "0");
  const d = day.value.toString().padStart(2, "0");

  const birthday = `${y}-${m}-${d}`;

  store.dispatch("auth/updateUserField", {
    field: "userBirthday",
    value: birthday,
  });
  isButtonActive.value = false;
  close();
}

function cancel() {
  isButtonActive.value = false;
  close();
}

// 处理关闭事件
function close() {
  emit("close");
}

// 在 created 钩子中执行初始化逻辑
onMounted(() => {
  // 初始化年份选项
  initYears();

  // 如果有已保存的生日数据
  const savedBirthday = store.state.auth.userBirthday; // 假设格式为 "2003-9-20"
  if (savedBirthday) {
    parseDateString(savedBirthday);
  }

  // 初始化天数选项
  updateDays();
});
</script>

<style scoped lang="scss">
.set-birthday {
  width: 100%;
  max-width: 500px;
  padding: 24px;
  background-color: #ffffff;
  border-radius: 12px;
  box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
  position: relative;
  top: 45px;

  .close-button {
    position: absolute;
    top: 16px;
    right: 16px;
    cursor: pointer;
    z-index: 10;
    color: #333;
  }

  .title-container {
    text-align: center;
    margin-bottom: 32px;

    .icon {
      margin-bottom: 24px;
    }

    h2 {
      font-size: 24px;
      font-weight: 600;
      color: #333;
    }

    .description {
      font-size: 14px;
      color: #666;
      margin-top: 36px;
    }
  }

  .date-selectors {
    display: flex;
    gap: 16px;
    justify-content: center;
    margin-top: 50px;
  }

  .selector-group {
    flex: 1;
    text-align: center;
  }

  .selector-label {
    display: block;
    font-size: 12px;
    color: #666;
    margin-bottom: 8px;
  }

  .selector-field {
    width: 100%;
    padding: 12px 16px;
    border: 1px solid #dcdfe6;
    border-radius: 8px;
    font-size: 16px;
    appearance: none;
    background-image: url("data:image/svg+xml;charset=UTF-8,%3Csvg xmlns='http://www.w3.org/2000/svg' width='16' height='16' viewBox='0 0 16 16'%3E%3Cpath fill='%23666' d='M4 9l5 5 5-5z'/%3E%3C/svg%3E");
    background-repeat: no-repeat;
    background-position: right 12px center;
    background-size: 16px;
    transition: border-color 0.3s ease;
    cursor: pointer;

    &:focus {
      outline: none;
      border-color: #409eff;
      box-shadow: 0 0 0 2px rgba(64, 158, 255, 0.2);
    }
  }
  .button-group {
    display: flex;
    justify-content: space-between;
    margin-top: 62px;

    .cancel-button {
      padding: 12px 24px;
      border: 1px solid #409eff;
      border-radius: 8px;
      background-color: transparent;
      color: #409eff;
      font-size: 16px;
      cursor: pointer;
      transition: all 0.3s ease;
      width: 35%;

      &:hover {
        background-color: rgba(64, 158, 255, 0.1);
      }
    }

    .save-button {
      padding: 12px 24px;
      border: none;
      border-radius: 8px;
      background-color: #409eff;
      color: white;
      font-size: 16px;
      cursor: pointer;
      transition: all 0.3s ease;
      width: 35%;

      &:hover {
        background-color: #3a8ee6;
      }
    }
  }
}
</style>
