<template>
  <div class="set-phone">
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
    <div v-show="!showChangePhoneModal">
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
      <h2 class="modal-title">电话号码</h2>
      <p class="modal-description">电话号码可用于登录</p>

      <div class="phone-list">
        <div class="phone-item">
          <div class="phone-content">
            <svg
              width="16"
              height="16"
              viewBox="0 0 16 16"
              fill="none"
              xmlns="http://www.w3.org/2000/svg"
            >
              <path
                d="M11 11L9 9M9 9L7 11M9 9L11 7M9 9L7 7M9 9L9 13M9 9L13 9M9 9L9 5M9 9L5 9"
                stroke="currentColor"
                stroke-width="2"
              />
            </svg>
            <span class="phone-text">{{ userPhone }}</span>
          </div>
          <button class="change-button" @click="changePhoneModal">
            <span>更改</span>
          </button>
        </div>
      </div>
    </div>

    <div v-show="showChangePhoneModal">
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

      <h2 class="modal-title">添加新电话号码</h2>
      <p class="modal-description">验证码将发送至此号码。</p>
      <p class="modal-warning">可能会产生短信和数据费用。</p>

      <div class="form-container-group">
        <div class="form-group">
          <select v-model="countryCode" class="country-code-select">
            <option value="+86">+86 (中国大陆)</option>
            <option value="+1">+1 (美国)</option>
            <option value="+44">+44 (英国)</option>
            <option value="+81">+81 (日本)</option>
            <option value="+82">+82 (韩国)</option>
            <option value="+49">+49 (德国)</option>
            <option value="+33">+33 (法国)</option>
            <option value="+39">+39 (意大利)</option>
            <option value="+34">+34 (西班牙)</option>
          </select>
        </div>

        <div class="form-group">
          <input
            v-model="newUserPhone"
            type="tel"
            placeholder="电话号码"
            class="phone-input"
            @input="validatePhoneNumber"
          />
        </div>
      </div>

      <div v-show="isButtonActive" class="button-group">
        <button class="cancel-button" @click="cancel">取消</button>
        <button
          class="continue-button"
          :disabled="!isValidPhoneNumber"
          @click="changePhone"
        >
          继续
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from "vue";
import { useStore } from "vuex";
import { key } from "@/store/userStore";

const emit = defineEmits(["close", "submit"]);

// 使用 store
const store = useStore(key);

const userPhone = computed(() => store.state.auth.userPhone);
// 新电话号码
const countryCode = ref("+86");
const newUserPhone = ref("");

// 模态框状态
const showChangePhoneModal = ref(false);

// 验证状态
const isValidPhoneNumber = ref(false);

const isButtonActive = ref(false);

// 方法
function validatePhoneNumber() {
  // 简单的手机号验证（以中国大陆为例）
  const phoneRegex = /^1[3-9]\d{9}$/;
  isValidPhoneNumber.value = phoneRegex.test(newUserPhone.value);
  isButtonActive.value = true;
}

// 修改电话号码
function changePhone() {
  if (isValidPhoneNumber.value && newUserPhone.value) {
    const field = "userPhone";
    // 发送带加号的纯数字格式
    const phone = `${countryCode.value}${newUserPhone.value}`;
    emit("submit", {
      field,
      phone,
    });
    // 更新用户电话号码
    store.dispatch("auth/updateUserData");
    newUserPhone.value = "";
  }
}

// 修改电话号码页面
function changePhoneModal() {
  showChangePhoneModal.value = true;
}

// 取消操作
function cancel() {
  showChangePhoneModal.value = false;
  isButtonActive.value = false;
  newUserPhone.value = "";
}
// 关闭模态框
function close() {
  if (showChangePhoneModal.value === true) {
    cancel();
    return;
  } else {
    emit("close");
  }
}

// 生命周期钩子
onMounted(() => {
  // 可以在这里执行初始化操作
});
</script>

<style scoped lang="scss">
.set-phone {
  width: 500px;
  max-width: 500px;
  background: white;
  border-radius: 12px;
  box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
  padding: 32px;
  text-align: center;
  position: relative;
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

.logo {
  margin-bottom: 24px;
  text-align: center;
  svg {
    fill: #007aff;
  }
}

.modal-title {
  font-size: 20px;
  font-weight: 600;
  color: #000;
  margin-bottom: 8px;
  text-align: center;
}

.modal-description {
  font-size: 14px;
  color: #666;
  margin-bottom: 32px;
  text-align: center;
  line-height: 1.5;
}

.phone-text {
  font-size: 20px;
  color: #333;
}

.change-button {
  width: 60px;
  border-radius: 50%;
  color: #999;
  border: none;
  display: flex; // 启用 Flex 布局
  align-items: center; // 垂直居中
  justify-content: center; // 水平居中
  flex-shrink: 0; // 防止按钮缩小
  padding: 0;
}
.change-button:hover {
  color: #333;
}

.phone-list {
  margin-bottom: 32px;
}

.phone-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 0;
  border-bottom: 1px solid #eaeaea;

  &:last-child {
    border-bottom: none;
  }
}

.phone-content {
  display: flex;
  align-items: center;
  gap: 16px;
}

.primary-label {
  font-size: 14px;
  color: #007aff;
  font-weight: 500;
}

.set-primary-button {
  font-size: 14px;
  color: #007aff;
  background: none;
  border: none;
  cursor: pointer;
  padding: 4px 8px;
  border-radius: 4px;
  transition: all 0.3s ease;

  &:hover {
    background: #e6f2ff;
  }
}

.modal-title {
  font-size: 20px;
  font-weight: 600;
  color: #000;
  margin-bottom: 16px;
}

.modal-description {
  font-size: 14px;
  color: #666;
  margin-bottom: 16px;
}

.modal-warning {
  font-size: 14px;
  color: #999;
  margin-bottom: 24px;
}

.form-container-group {
  display: flex;
  flex-direction: column;
  align-items: center;
}
.form-group {
  margin-bottom: 38px;
  width: 300px;

  .country-code-select {
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

    &:focus {
      outline: none;
      border-color: #409eff;
      box-shadow: 0 0 0 2px rgba(64, 158, 255, 0.2);
    }
  }
}

.phone-input {
  width: 100%;
  padding: 12px 16px;
  border: 1px solid #dcdfe6;
  border-radius: 8px;
  font-size: 16px;
  transition: border-color 0.3s ease;

  &:focus {
    outline: none;
    border-color: #409eff;
    box-shadow: 0 0 0 2px rgba(64, 158, 255, 0.2);
  }
}

.button-group {
  display: flex;
  gap: 16px;
  justify-content: center;
}

.cancel-button {
  padding: 12px 24px;
  border: 1px solid #007aff;
  border-radius: 8px;
  background: white;
  color: #007aff;
  font-size: 16px;
  cursor: pointer;
  transition: all 0.3s ease;

  &:hover {
    background: #f0f8ff;
  }
}

.continue-button {
  padding: 12px 24px;
  border: none;
  border-radius: 8px;
  background: #007aff;
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
</style>
