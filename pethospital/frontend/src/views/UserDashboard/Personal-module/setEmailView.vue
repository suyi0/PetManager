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
    <div v-show="!showChangeEmailModal">
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
        <p class="description">这个电子邮件地址可用于登录</p>
      </div>

      <div class="email-list">
        <div class="email-item">
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
            <span class="email-text">{{ userEmail }}</span>
          </div>
          <button @click="changeEmailModal" class="change-button">
            <span>更改</span>
          </button>
        </div>
      </div>
    </div>
    <div v-show="showChangeEmailModal" class="add-email-modal">
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

      <h2 class="modal-title">更改邮箱</h2>

      <p class="modal-description">验证码将发送至此邮箱地址。</p>

      <!-- 添加电子邮件表单 -->
      <div v-if="showChangeEmailModal" class="changeEmail-input">
        <input
          v-model="newUserEmail"
          type="email"
          placeholder="输入电子邮件地址"
          class="email-input"
          @keyup.enter="changeEmail"
          @input="checkEmail"
        />
        <div v-show="!newEmailEffect && ischeckEmail" class="email-effect">
          邮箱已存在不可用
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
            aria-label="关闭"
            class="verification-button"
            :class="{ ' after  ': isgetVerificationCode }"
            :disabled="
              isgetVerificationCode || !isEmailValid || !newEmailEffect
            "
            @click="getVerificationCode"
          >
            {{ isgetVerificationCode ? `${count}秒后重新获取` : "获取验证码" }}
          </button>
        </div>
        <div v-show="isButtonActive" class="form-actions">
          <button @click="cancel" class="cancel-button">取消</button>
          <button
            @click="changeEmail"
            class="save-button"
            :disabled="!newEmailEffect"
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
import { key } from "@/store/userStore";
// import { useRouter, useRoute } from "vue-router"
// import type { PropType } from "vue"

const store = useStore(key);

// 响应式数据
const userEmail = computed(() => store.state.auth.userEmail);
const newUserEmail = ref("");
const VerificationCode = ref("");
const showChangeEmailModal = ref(false);
const isEmailValid = ref(false);
const isButtonActive = ref(false);
const isgetVerificationCode = ref(false); // 添加验证码按钮状态-灰色(禁用)
const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/; // 简单的邮箱格式验证正则表达式
const ischeckEmail = ref(false);
const newEmailEffect = ref(false); // 邮箱是否可用
const number = ref(0);
const count = ref(60);

const emialCheckTimeout = ref<number | null>(null); // 用于保存邮箱检查的定时器
const emailChangeTimeout = ref<number | null>(null);
let checkEmailInProgress = false;
let emailChangeInProgress = false;
let requestInProgress = false;

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
const emit = defineEmits(["close", "submit"]);

// 6. 方法定义
const checkEmail = () => {
  if (emialCheckTimeout.value) {
    clearTimeout(emialCheckTimeout.value);
  }

  emialCheckTimeout.value = setTimeout(() => {
    if (number.value <= 50) {
      number.value++;

      // 验证邮箱格式是否正确
      if (!emailRegex.test(newUserEmail.value)) {
        isEmailValid.value = false;
        return;
      }

      isEmailValid.value = true;

      if (checkEmailInProgress) {
        return;
      }

      checkEmailInProgress = true;
      store
        .dispatch("auth/checkEmail", { email: newUserEmail.value })
        .then((response) => {
          if (response.status === 200) {
            // 其他状态码表示邮箱未被注册，可以使用
            newEmailEffect.value = true;
          } else if (response.status === 400) {
            // 邮箱已被注册，不能使用
            newEmailEffect.value = false;
            ischeckEmail.value = true;
          }
        })
        .catch((error) => {
          if (error.response.status === 400) {
            // 后端返回400错误，表示邮箱已被注册
            newEmailEffect.value = false;
            ischeckEmail.value = true;
          }
        })
        .finally(() => {
          checkEmailInProgress = false;
        });
    }
  }, 1000);

  isButtonActive.value = true;
};

function getVerificationCode() {
  if (!isgetVerificationCode.value && isEmailValid.value) {
    if (requestInProgress || !isEmailValid.value) {
      return;
    }

    requestInProgress = true;
    isgetVerificationCode.value = true;
    count.value = 60;
    startCountdown();
    store
      .dispatch("user/getVerificationCode", {
        email: newUserEmail.value,
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
        requestInProgress = false; // 释放锁
      });
  }
}

function changeEmailModal() {
  showChangeEmailModal.value = true;
}
// 修改电子邮件地址
function changeEmail() {
  if (emailChangeTimeout.value) {
    clearTimeout(emailChangeTimeout.value);
  }

  if (emailChangeInProgress) {
    return;
  }
  emailChangeInProgress = true;

  emailChangeTimeout.value = setTimeout(() => {
    // 邮箱可以使用才提交
    if (newEmailEffect.value) {
      // 修改电子邮件地址前先验证验证码
      store
        .dispatch("auth/register", {
          email: newUserEmail.value,
          verificationCode: VerificationCode.value,
        })
        .then((response) => {
          if (response.status === 200) {
            if (newUserEmail.value) {
              emit("submit", {
                field: "userEmail",
                value: newUserEmail.value,
              });

              // 验证码验证成功，更新用户数据
              return store.dispatch("auth/updateUserData");
            }
          }
        })
        .then((updateResponse) => {
          if (updateResponse.status === 200) {
            alert("修改成功");
            newUserEmail.value = "";
          }
        })
        .catch((error) => {
          if (error.response.status === 400) {
            alert("修改操作失败");
          }
        })
        .finally(() => {
          checkEmailInProgress = false;
          newEmailEffect.value = false; // 重置邮箱可用标志
        });
    }
  }, 1000);
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

// 生命周期钩子
onMounted(() => {
  // 可以在这里执行初始化操作
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
  showChangeEmailModal.value = false;
  isButtonActive.value = false;
  newUserEmail.value = "";
};
// 处理关闭事件
function close() {
  if (showChangeEmailModal.value === true) {
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
  font-size: 20px;
  color: #333;
}

.change-button {
  width: 60px;
  background: none;
  border: none;
  color: #999;
  display: flex; // 启用 Flex 布局
  align-items: center; // 垂直居中
  justify-content: center; // 水平居中
  flex-shrink: 0; // 防止按钮缩小
  padding: 0px;
}

.change-button:hover {
  color: #333;
}

.add-email-form {
  margin-bottom: 24px;
}

.form-actions {
  display: flex;
  justify-content: center;
  gap: 16px;
  margin-top: 16px;
}

.email-effect {
  color: red;
  font-size: 16px;
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

.changeEmail-input {
  .email-input {
    width: 300px;
    padding: 12px 16px;
    border: 1px solid #dcdfe6;
    border-radius: 8px;
    font-size: 16px;
    transition: border-color 0.3s ease;
  }
  .email-input:focus {
    outline: none;
    border-color: #409eff;
    box-shadow: 0 0 0 2px rgba(64, 158, 255, 0.2);
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
</style>
