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

        <input
          v-model="newUserPhone"
          type="tel"
          placeholder="电话号码"
          class="phone-input"
          @keyup.enter="changePhone"
          @input="checkPhone"
        />
        <div v-show="!newPhoneEffect && ischeckPhone" class="phone-effect">
          电话号码已存在不可用
        </div>

        <div class="verification-container">
          <input
            id="VerificationCode"
            type="text"
            class="verification-code"
            v-model="VerificationCode"
            placeholder="VerificationCode"
          />
          <button
            class="verification-button"
            :class="{ ' after  ': isgetVerificationCode }"
            :disabled="
              isgetVerificationCode || !isPhoneValid || !newPhoneEffect
            "
            @click="getVerificationCode"
          >
            {{ isgetVerificationCode ? `${count}秒后重新获取` : "获取验证码" }}
          </button>
        </div>

        <div v-show="isButtonActive" class="button-group">
          <button class="cancel-button" @click="cancel">取消</button>
          <button
            class="continue-button"
            :disabled="!isPhoneValid"
            @click="changePhone"
          >
            继续
          </button>
        </div>
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

const showChangePhoneModal = ref(false);
const userPhone = computed(() => store.state.auth.userPhone);
const VerificationCode = ref("");
const phoneRegex = /^1[3-9]\d{9}$/; // 简单的手机号验证（以中国大陆为例）
const isgetVerificationCode = ref(false);
const isButtonActive = ref(false);
const ischeckPhone = ref(false);
let count = ref(60);

// 定时器
const phoneCheckTimeout = ref<any>(null);
const phoneChangeTimeout = ref<any>(null);

let checkPhoneInProgress = false;
let requestInProgress = false;
let changePhoneInProgress = false;

// 新电话号码
const countryCode = ref("+86");
const newUserPhone = ref("");

// 验证状态
const isPhoneValid = ref(false);
const newPhoneEffect = ref(false);

// 方法
function checkPhone() {
  if (phoneCheckTimeout.value) {
    clearTimeout(phoneCheckTimeout.value);
  }

  phoneCheckTimeout.value = setTimeout(() => {
    if (!phoneRegex.test(newUserPhone.value)) {
      isPhoneValid.value = false;
      return;
    }

    isPhoneValid.value = true;

    if (checkPhoneInProgress) {
      return;
    }
    checkPhoneInProgress = true;

    store
      .dispatch("auth/checkPhone", {
        phone: newUserPhone.value,
      })
      .then((response) => {
        if (response.status === 200) {
          // 手机号未被注册，可以使用
          newPhoneEffect.value = true;
        } else if (response.status === 400) {
          // 手机号已被注册，不能使用
          newPhoneEffect.value = false;
          ischeckPhone.value = true;
        }
      })
      .catch((error) => {
        if (error.response.status === 400) {
          // 后端返回400错误，表示手机号已被注册
          newPhoneEffect.value = false;
          ischeckPhone.value = true;
        }
      })
      .finally(() => {
        checkPhoneInProgress = false;
      });
  }, 1000);

  isButtonActive.value = true;
}

async function getVerificationCode() {
  if (!isgetVerificationCode.value && isPhoneValid.value) {
    if (requestInProgress || !isPhoneValid.value) {
      return;
    }

    requestInProgress = true;
    isgetVerificationCode.value = true;
    count.value = 60;
    startCountdown();
    store
      .dispatch("auth/sendVerificationCode", {
        phone: `${countryCode.value}${newUserPhone.value}`,
      })
      .then((response) => {
        if (response.status === 200) {
          alert("验证码已发送");
        }
      })
      .catch((error) => {
        if (error.response) {
          if (error.response.status === 400) {
            alert("错误：" + error.response.data.error);
          } else {
            console.log(error.response.status);
            alert("服务器错误: " + error.response.data.error);
          }
        } else if (error.request) {
          // 请求已发出但没有收到响应
          alert("网络错误，请检查网络连接");
        } else {
          // 其他错误
          alert("请求错误: " + error.message);
        }
      })
      .finally(() => {
        requestInProgress = false;
      });
  }
}

// 修改电话号码
function changePhone() {
  if (phoneChangeTimeout.value) {
    clearTimeout(phoneChangeTimeout.value);
  }

  if (changePhoneInProgress) {
    alert("请勿重复提交");
    return;
  }
  changePhoneInProgress = true;

  phoneChangeTimeout.value = setTimeout(() => {
    if (newPhoneEffect.value) {
      store
        .dispatch("auth/verify", {
          phone: `${countryCode.value}${newUserPhone.value}`,
          verificationCode: VerificationCode.value,
        })
        .then((response) => {
          if (response.status === 200) {
            if (newUserPhone.value) {
              emit("submit", {
                field: "userPhone",
                value: newUserPhone.value,
              });

              // 验证码验证成功，更新用户数据
              return store.dispatch("auth/updateUserData");
            }
          }
        })
        .then((updateResponse) => {
          if (updateResponse.status === 200) {
            alert("修改成功");
            resetForm();
          }
        })
        .catch((error) => {
          if (error.response.status === 400) {
            alert("修改操作失败");
          }
        })
        .finally(() => {
          checkPhoneInProgress = false;
          newPhoneEffect.value = false; // 重置手机号可用标志
        });
    }
  }, 1000);
}

// 修改电话号码页面
function changePhoneModal() {
  showChangePhoneModal.value = true;
}

function startCountdown() {
  // 每秒减少倒计时
  const timer = setInterval(() => {
    count.value--;
    if (count.value <= 0) {
      // 倒计时结束，清除定时器并重置按钮状态
      clearInterval(timer);
      isgetVerificationCode.value = false;
    }
  }, 1000);
}

function resetForm() {
  showChangePhoneModal.value = false;
  countryCode.value = "+86";
  newUserPhone.value = "";
  VerificationCode.value = "";
  isPhoneValid.value = false;
  newPhoneEffect.value = false;
  ischeckPhone.value = false;
  isButtonActive.value = false;
}

// 取消操作
function cancel() {
  resetForm();
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
  .country-code-select {
    width: 334px;
    padding: 12px 16px;
    border: 1px solid #dcdfe6;
    margin-bottom: 12px;
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
  .phone-effect {
    color: red;
    font-size: 16px;
  }

  .phone-input {
    width: 300px;
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

  .verification-container {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    justify-content: center;
    margin-top: 12px;

    .verification-code {
      width: 174px;
      padding: 12px 16px;
      border: 1px solid #dcdfe6;
      border-radius: 8px;
      font-size: 16px;
      transition: border-color 0.3s ease;
    }

    .verification-button,
    .verification-button.after {
      display: flex;
      align-items: center;
      justify-content: center;
      width: 114px;
      height: 40px;
      margin-bottom: 15px;
      font-size: 18px;
      font-weight: 520;
      border: #000 3px solid;
      border-radius: 10px;
      position: relative;
      top: 8px;
      padding-inline-start: 0px;
      padding-block-end: 0px;
    }
    .verification-button.after {
      font-size: 14px;
    }
    .verification-button:hover {
      background-color: rgb(148, 232, 130);
      border: rgb(148, 232, 130) 3px solid;
      color: rgb(255, 255, 255);
    }
    .verification-button:active {
      background-color: rgb(255, 102, 102);
      border: rgb(255, 102, 102) 3px solid;
    }
    .verification-button:disabled {
      background-color: light-dark(
        rgba(239, 239, 239, 0.3),
        rgba(19, 1, 1, 0.3)
      );
      color: light-dark(rgba(16, 16, 16, 0.3), rgba(255, 255, 255, 0.3));
      border-color: light-dark(
        rgba(118, 118, 118, 0.3),
        rgba(195, 195, 195, 0.3)
      );
      cursor: default;
    }
  }
}

.button-group {
  display: flex;
  gap: 16px;
  justify-content: center;
  margin-top: 28px;
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
