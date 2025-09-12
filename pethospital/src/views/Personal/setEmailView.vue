<template>
  <div class="set-email">
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
    <div v-show="!showAddEmailModal">
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
        <h2>电子邮件地址</h2>
        <p class="description">这些电子邮件地址可用于登录</p>
      </div>

      <!-- Email section -->
      <div class="section">
        <div class="section-header">
          <h3>{{ emailCount }} 个邮箱</h3>
          <button class="add-button" @click="showAddEmailModal = true">
            <svg
              class="add-icon"
              width="32"
              height="32"
              viewBox="0 0 32 32"
              fill="none"
              xmlns="http://www.w3.org/2000/svg"
            >
              <circle cx="16" cy="16" r="15" fill="#007AFF" />
              <path
                d="M16 10V22M10 16H22"
                stroke="white"
                stroke-width="2"
                stroke-linecap="round"
              />
            </svg>
          </button>
        </div>
      </div>

      <!-- 电子邮件地址列表 -->
      <div class="email-list">
        <div v-for="(email, index) in emails" :key="index" class="email-item">
          <div class="email-content">
            <svg
              width="16"
              height="16"
              viewBox="0 0 16 16"
              fill="none"
              xmlns="http://www.w3.org/2000/svg"
            >
              <path
                d="M2 4H14C14.5523 4 15 4.44772 15 5V11C15 11.5523 14.5523 12 14 12H2C1.44772 12 1 11.5523 1 11V5C1 4.44772 1.44772 4 2 4Z"
                stroke="#409EFF"
                stroke-width="2"
              />
              <path
                d="M2 4L8 9L14 4"
                stroke="#409EFF"
                stroke-width="2"
                stroke-linecap="round"
              />
            </svg>
            <span class="email-text">{{ email.email }}</span>
          </div>
          <button @click="removeEmail(index)" class="remove-button">
            <svg
              width="16"
              height="16"
              viewBox="0 0 16 16"
              fill="none"
              xmlns="http://www.w3.org/2000/svg"
            >
              <!-- 外圈 -->
              <circle
                cx="8"
                cy="8"
                r="7"
                stroke="currentColor"
                stroke-width="2"
              />
              <!-- 红色减号 -->
              <path
                d="M5 8H11"
                stroke="red"
                stroke-width="2"
                stroke-linecap="round"
              />
            </svg>
          </button>
        </div>
      </div>
    </div>
    <div v-show="showAddEmailModal" class="add-email-modal">
      <div class="logo">
        <svg
          width="60"
          height="60"
          viewBox="0 0 60 60"
          fill="none"
          xmlns="http://www.w3.org/2000/svg"
        >
          <circle
            cx="30"
            cy="30"
            r="28"
            stroke="currentColor"
            stroke-width="2"
          />
          <path
            d="M30 12C35.5228 12 40 16.4772 40 22C40 27.5228 35.5228 32 30 32C24.4772 32 20 27.5228 20 22C20 16.4772 24.4772 12 30 12Z"
            fill="currentColor"
          />
          <path
            d="M30 32C35.5228 32 40 27.5228 40 22C40 16.4772 35.5228 12 30 12C24.4772 12 20 16.4772 20 22C20 27.5228 24.4772 32 30 32Z"
            fill="currentColor"
          />
          <path
            d="M30 32C35.5228 32 40 27.5228 40 22C40 16.4772 35.5228 12 30 12C24.4772 12 20 16.4772 20 22C20 27.5228 24.4772 32 30 32Z"
            fill="currentColor"
          />
        </svg>
      </div>

      <h2 class="modal-title">添加新邮箱</h2>

      <p class="modal-description">验证码将发送至此邮箱地址。</p>

      <!-- 添加电子邮件表单 -->
      <div v-if="showAddEmailModal">
        <input
          v-model="newEmail"
          type="email"
          placeholder="输入电子邮件地址"
          class="email-input"
          @keyup.enter="addEmail"
          @input="validateEmailNumber"
        />
        <div v-show="isButtonActive" class="form-actions">
          <button @click="cancel" class="cancel-button">取消</button>
          <button
            @click="addEmail"
            :disabled="!isValidEmailNumber"
            class="save-button"
          >
            继续
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// 1. 导入部分
import { ref, onMounted, onBeforeUnmount, computed } from "vue";
import { useStore } from "vuex";
import { key } from "@/store";
// import { useRouter, useRoute } from "vue-router"
// import type { PropType } from "vue"

const store = useStore(key);

// 响应式数据
const emails = ref<Array<{ email: string }>>([]);
const newEmail = ref("");
const showAddEmailModal = ref(false);
const isValidEmailNumber = ref(false);
const isButtonActive = ref(false);

// 计算属性
const emailCount = computed(() => emails.value.length);

// 2. Props定义 (如果需要接收父组件传递的数据)
// const props = defineProps({
//   title: {
//     type: String,
//     default: ''
//   },
//   dataList: {
//     type: Array as PropType<any[]>,
//     default: () => []
//   }
// })

//3. Emit事件定义 (如果需要向父组件传递事件)
const emit = defineEmits(["close"]);

// 6. 方法定义
const validateEmailNumber = () => {
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  isValidEmailNumber.value = emailRegex.test(newEmail.value);
  isButtonActive.value = true;
};
// 添加新的电子邮件地址
function addEmail() {
  if (newEmail.value && !emails.value.some((e) => e.email === newEmail.value)) {
    emails.value.push({ email: newEmail.value });
    newEmail.value = "";
    showAddEmailModal.value = false;
  }
}

// 移除指定的电子邮件地址
function removeEmail(index: number) {
  if (emails.value.length > 1) {
    emails.value.splice(index, 1);
  }
}

// 生命周期钩子
onMounted(() => {
  // 如果用户已经有电话号码，则添加到 phones 数组中
  if (store.state.auth.userEmail) {
    emails.value.push({ email: store.state.auth.userEmail });
  }
});

onBeforeUnmount(() => {
  // 可以在这里执行清理操作
});

// 8. 监听器 (如需要)
// import { watch, watchEffect } from "vue"
// watch(() => formData.name, (newVal, oldVal) => {
//   console.log("Name changed:", newVal)
// });

// 9. 路由相关 (如需要)
// const router = useRouter()
// const route = useRoute()
// const goToDetail = (id: number) => {
//   router.push(`/detail/id`)
// };

// 取消操作
const cancel = () => {
  showAddEmailModal.value = false;
  isButtonActive.value = false;
};
// 处理关闭事件
function close() {
  if (showAddEmailModal.value === true) {
    cancel();
    return;
  } else {
    emit("close");
  }
}
</script>

<style scoped lang="scss">
.set-email {
  width: 500px;
  max-width: 500px;
  padding: 32px;
  background-color: #ffffff;
  border-radius: 12px;
  box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
  position: relative;
  text-align: center;
  top: 45px;
}

.close-button {
  position: absolute;
  top: 12px;
  right: 16px;
  font-size: 24px;
  cursor: pointer;
  background: none;
  border: none;
  color: #000;
  font-weight: bold;
  padding: 4px;
  border-radius: 50%;
}

.title-container {
  text-align: center;
  margin-bottom: 32px;
}

.title-container .icon {
  margin-bottom: 24px;
}

.title-container h2 {
  font-size: 24px;
  font-weight: 600;
  color: #333;
}

.title-container .description {
  font-size: 14px;
  color: #666;
  margin-top: 36px;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.section-header .add-button {
  width: 32px;
  height: 32px;
  border-radius: 50%;
  color: white;
  border: none;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  padding: 0;
}

.email-list {
  margin-bottom: 24px;
}

.email-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 0px;
  border-bottom: 1px solid #ebeef5;
}

.email-item:last-child {
  border-bottom: none;
}

.email-content {
  display: flex;
  align-items: center;
  gap: 16px;
}

.email-content .email-text {
  font-size: 16px;
  color: #333;
}

.remove-button {
  width: 32px;
  height: 32px;
  background: none;
  border: none;
  cursor: pointer;
  color: #999;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0px;
}

.remove-button:hover {
  color: #333;
}

.add-email-form {
  margin-bottom: 24px;
}

.email-input {
  width: 300px;
  padding: 12px 16px;
  border: 1px solid #dcdfe6;
  border-radius: 8px;
  font-size: 16px;
  transition: border-color 0.3s ease;
  margin-bottom: 38px;
}

.email-input:focus {
  outline: none;
  border-color: #409eff;
  box-shadow: 0 0 0 2px rgba(64, 158, 255, 0.2);
}

.form-actions {
  display: flex;
  justify-content: center;
  gap: 16px;
  margin-top: 16px;
}

.cancel-button {
  padding: 12px 24px;
  border: 1px solid #dcdfe6;
  border-radius: 8px;
  background-color: transparent;
  color: #666;
  font-size: 16px;
  cursor: pointer;
  transition: all 0.3s ease;

  &:hover {
    background-color: #f0f8ff;
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

  &:disabled {
    background: #cccccc;
    cursor: not-allowed;
  }

  &:hover:not(:disabled) {
    background: #0066cc;
  }
}

.button-group {
  display: flex;
  justify-content: flex-end;
}

.button-group .cancel-button {
  padding: 12px 24px;
  border: 1px solid #dcdfe6;
  border-radius: 8px;
  background-color: transparent;
  color: #666;
  font-size: 16px;
  cursor: pointer;
  transition: all 0.3s ease;
}

.button-group .cancel-button:hover {
  background-color: rgba(0, 0, 0, 0.05);
}

.logo {
  margin-bottom: 24px;
  text-align: center;
}

.logo svg {
  fill: #007aff;
}

.modal-title {
  font-size: 20px;
  font-weight: 600;
  color: #000;
  margin-bottom: 24px;
}

.modal-description {
  font-size: 14px;
  color: #666;
  margin-bottom: 24px;
}
</style>
