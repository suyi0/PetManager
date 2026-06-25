<template>
  <div class="pc-panel">
    <div class="pc-panel__head">
      <div>
        <h3>{{ showChangeEmailModal ? "更换邮箱地址" : "维护登录邮箱" }}</h3>
        <p>
          {{
            showChangeEmailModal
              ? "先检查新邮箱是否可用，再通过验证码完成替换。"
              : "邮箱用于登录、账单回执与服务通知，建议保持为常用地址。"
          }}
        </p>
      </div>
      <button type="button" class="pc-btn pc-btn--ghost" @click="close">
        关闭
      </button>
    </div>

    <div class="pc-current">
      <span class="pc-current__lbl">当前绑定</span>
      <span
        class="pc-current__val"
        :class="{ 'pc-current__val--empty': !userEmail }"
      >
        {{ userEmail || "暂未绑定" }}
      </span>
      <span
        v-if="userEmail"
        class="pc-pill pc-pill--ok"
        style="margin-left: auto"
        >已绑定</span
      >
    </div>

    <div
      v-if="!showChangeEmailModal"
      class="pc-actions"
      style="border-top: 0; padding-top: 0"
    >
      <button
        type="button"
        class="pc-btn pc-btn--primary"
        @click="changeEmailModal"
      >
        更换邮箱
      </button>
    </div>

    <form v-else class="pc-form" @submit.prevent="changeEmail">
      <label class="pc-field">
        <span>新邮箱地址</span>
        <input
          v-model.trim="newUserEmail"
          type="email"
          placeholder="请输入新的电子邮箱地址"
          @keyup.enter="changeEmail"
          @input="checkEmail"
        />
      </label>

      <p v-if="!newEmailEffect && ischeckEmail" class="pc-error">
        该邮箱已存在，当前无法继续使用。
      </p>

      <div class="pc-code-row">
        <label class="pc-field">
          <span>邮箱验证码</span>
          <input
            id="VerificationCode"
            v-model.trim="VerificationCode"
            type="text"
            placeholder="请输入验证码"
          />
        </label>
        <button
          type="button"
          class="pc-btn pc-btn--ghost"
          style="height: 40px"
          :disabled="isgetVerificationCode || !isEmailValid || !newEmailEffect"
          @click="getVerificationCode"
        >
          {{ isgetVerificationCode ? `${count} 秒后重发` : "获取验证码" }}
        </button>
      </div>

      <div
        class="pc-helper"
        :class="{ 'pc-helper--ok': isEmailValid && newEmailEffect }"
      >
        <span class="pc-dot"></span>
        {{
          !newUserEmail
            ? "请输入新邮箱，系统会自动校验是否可用"
            : isEmailValid && newEmailEffect
            ? "邮箱可用，可发送验证码完成替换"
            : "正在校验邮箱，请稍候…"
        }}
      </div>

      <div v-if="isButtonActive" class="pc-actions">
        <button type="button" class="pc-btn pc-btn--secondary" @click="cancel">
          取消
        </button>
        <button
          type="submit"
          class="pc-btn pc-btn--primary"
          :disabled="!newEmailEffect"
        >
          保存邮箱
        </button>
      </div>
    </form>
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { isEmail } from "@/core/auth/utils/authValidators";

const store = useStore(storeKey);
const emit = defineEmits(["close", "submit"]);

const showChangeEmailModal = ref(false);
const userEmail = computed(() => store.state.currentUser.userEmail);
const newUserEmail = ref("");
const VerificationCode = ref("");
const isEmailValid = ref(false);
const isButtonActive = ref(false);
const isgetVerificationCode = ref(false);
const ischeckEmail = ref(false);
const newEmailEffect = ref(false);
const number = ref(0);
const count = ref(60);

const emialCheckTimeout = ref<number | null>(null);
const emailChangeTimeout = ref<number | null>(null);
let checkEmailInProgress = false;
let emailChangeInProgress = false;
let requestInProgress = false;

const checkEmail = () => {
  if (emialCheckTimeout.value) {
    clearTimeout(emialCheckTimeout.value);
  }

  emialCheckTimeout.value = window.setTimeout(() => {
    if (number.value <= 50) {
      number.value++;

      if (!isEmail(newUserEmail.value)) {
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
            newEmailEffect.value = true;
            ischeckEmail.value = false;
          } else if (response.status === 400) {
            newEmailEffect.value = false;
            ischeckEmail.value = true;
          }
        })
        .catch((error) => {
          if (error.response?.status === 400) {
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
      .dispatch("auth/sendVerificationCode", {
        email: newUserEmail.value,
        scene: "change",
      })
      .then((response) => {
        if (response.status === 200) {
          alert("验证码已发送");
        }
      })
      .catch((error) => {
        if (error.response) {
          const errorMessage =
            error.response.data?.error?.details ||
            error.response.data?.message ||
            "请求失败";

          if (error.response.status === 400) {
            alert("错误：" + errorMessage);
          } else {
            alert("服务器错误: " + errorMessage);
          }
        } else if (error.request) {
          alert("网络错误，请检查网络连接");
        } else {
          alert("请求错误: " + error.message);
        }
      })
      .finally(() => {
        requestInProgress = false;
      });
  }
}

function changeEmailModal() {
  showChangeEmailModal.value = true;
  isButtonActive.value = true;
}

function changeEmail() {
  if (emailChangeTimeout.value) {
    clearTimeout(emailChangeTimeout.value);
  }

  if (emailChangeInProgress) {
    alert("请勿重复提交");
    return;
  }
  emailChangeInProgress = true;

  emailChangeTimeout.value = window.setTimeout(() => {
    if (newEmailEffect.value) {
      store
        .dispatch("auth/verify", {
          email: newUserEmail.value,
          verificationCode: VerificationCode.value,
          scene: "change",
        })
        .then((response) => {
          const ticket = response.data?.data?.ticket;
          if (response.status === 200 && ticket && newUserEmail.value) {
            return store.dispatch("currentUser/updateEmailWithTicket", {
              email: newUserEmail.value,
              ticket,
            });
          }

          throw new Error("邮箱验证凭证无效");
        })
        .then((updateResponse) => {
          if (updateResponse?.status === 200) {
            alert("修改成功");
            resetForm();
            emit("close");
          }
        })
        .catch((error) => {
          if (error.response?.status === 400) {
            alert("修改操作失败");
          }
        })
        .finally(() => {
          checkEmailInProgress = false;
          emailChangeInProgress = false;
          newEmailEffect.value = false;
        });
    } else {
      emailChangeInProgress = false;
    }
  }, 1000);
}

function startCountdown() {
  const timer = setInterval(() => {
    count.value--;
    if (count.value <= 0) {
      clearInterval(timer);
      isgetVerificationCode.value = false;
    }
  }, 1000);
}

onMounted(() => {
  // reserved for init
});

onBeforeUnmount(() => {
  if (emialCheckTimeout.value) {
    clearTimeout(emialCheckTimeout.value);
  }
  if (emailChangeTimeout.value) {
    clearTimeout(emailChangeTimeout.value);
  }
});

function resetForm() {
  showChangeEmailModal.value = false;
  newUserEmail.value = "";
  VerificationCode.value = "";
  isEmailValid.value = false;
  newEmailEffect.value = false;
  ischeckEmail.value = false;
  isButtonActive.value = false;
  emailChangeInProgress = false;
}

const cancel = () => {
  resetForm();
};

function close() {
  if (showChangeEmailModal.value) {
    cancel();
    return;
  }
  emit("close");
}
</script>
