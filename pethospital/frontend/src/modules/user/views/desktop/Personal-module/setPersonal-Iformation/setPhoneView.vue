<template>
  <div class="pc-panel">
    <div class="pc-panel__head">
      <div>
        <h3>{{ showChangePhoneModal ? "更换手机号" : "维护登录手机号" }}</h3>
        <p>
          {{
            showChangePhoneModal
              ? "先校验新手机号是否可用，再通过短信验证码完成替换。"
              : "手机号用于登录、预约通知与客服联系。"
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
        :class="{ 'pc-current__val--empty': !userPhone }"
      >
        {{ userPhone || "暂未绑定" }}
      </span>
      <span
        v-if="userPhone"
        class="pc-pill pc-pill--ok"
        style="margin-left: auto"
        >已绑定</span
      >
    </div>

    <div
      v-if="!showChangePhoneModal"
      class="pc-actions"
      style="border-top: 0; padding-top: 0"
    >
      <button
        type="button"
        class="pc-btn pc-btn--primary"
        @click="changePhoneModal"
      >
        更换手机号
      </button>
    </div>

    <form v-else class="pc-form" @submit.prevent="changePhone">
      <div class="pc-grid-2">
        <label class="pc-field">
          <span>国家 / 地区</span>
          <select v-model="countryCode">
            <option value="+86">+86 中国大陆</option>
            <option value="+1">+1 美国</option>
            <option value="+44">+44 英国</option>
            <option value="+81">+81 日本</option>
            <option value="+82">+82 韩国</option>
            <option value="+49">+49 德国</option>
            <option value="+33">+33 法国</option>
            <option value="+39">+39 意大利</option>
            <option value="+34">+34 西班牙</option>
          </select>
        </label>

        <label class="pc-field">
          <span>新手机号</span>
          <input
            v-model.trim="newUserPhone"
            type="tel"
            placeholder="请输入新的手机号码"
            @keyup.enter="changePhone"
            @input="checkPhone"
          />
        </label>
      </div>

      <p v-if="!newPhoneEffect && ischeckPhone" class="pc-error">
        该手机号已存在，当前无法继续使用。
      </p>

      <div class="pc-code-row">
        <label class="pc-field">
          <span>短信验证码</span>
          <input
            id="VerificationCode"
            v-model.trim="VerificationCode"
            type="text"
            placeholder="请输入 6 位验证码"
          />
        </label>
        <button
          type="button"
          class="pc-btn pc-btn--ghost"
          style="height: 40px"
          :disabled="isgetVerificationCode || !isPhoneValid || !newPhoneEffect"
          @click="getVerificationCode"
        >
          {{ isgetVerificationCode ? `${count} 秒后重发` : "获取验证码" }}
        </button>
      </div>

      <div
        class="pc-helper"
        :class="{ 'pc-helper--ok': isPhoneValid && newPhoneEffect }"
      >
        <span class="pc-dot"></span>
        {{
          !newUserPhone
            ? "请输入新手机号，系统会自动校验是否可用"
            : isPhoneValid && newPhoneEffect
            ? "号码可用，可发送验证码完成替换"
            : "正在校验号码，请稍候…"
        }}
      </div>

      <div v-if="isButtonActive" class="pc-actions">
        <button type="button" class="pc-btn pc-btn--secondary" @click="cancel">
          取消
        </button>
        <button
          type="submit"
          class="pc-btn pc-btn--primary"
          :disabled="!isPhoneValid || !newPhoneEffect"
        >
          保存手机号
        </button>
      </div>
    </form>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { isPhone } from "@/core/auth/utils/authValidators";

const emit = defineEmits(["close", "submit"]);
const store = useStore(storeKey);

const showChangePhoneModal = ref(false);
const userPhone = computed(() => store.state.currentUser.userPhone);
const VerificationCode = ref("");
const isgetVerificationCode = ref(false);
const isButtonActive = ref(false);
const ischeckPhone = ref(false);
const count = ref(60);

const phoneCheckTimeout = ref<ReturnType<typeof setTimeout> | null>(null);
const phoneChangeTimeout = ref<ReturnType<typeof setTimeout> | null>(null);

let checkPhoneInProgress = false;
let requestInProgress = false;
let changePhoneInProgress = false;

const countryCode = ref("+86");
const newUserPhone = ref("");

const isPhoneValid = ref(false);
const newPhoneEffect = ref(false);

function checkPhone() {
  if (phoneCheckTimeout.value) {
    clearTimeout(phoneCheckTimeout.value);
  }

  phoneCheckTimeout.value = setTimeout(() => {
    if (!isPhone(newUserPhone.value)) {
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
          newPhoneEffect.value = true;
          ischeckPhone.value = false;
        } else if (response.status === 400) {
          newPhoneEffect.value = false;
          ischeckPhone.value = true;
        }
      })
      .catch((error) => {
        if (error.response?.status === 400) {
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

              return store.dispatch("currentUser/updateUserData");
            }
          }
        })
        .then((updateResponse) => {
          if (updateResponse?.status === 200) {
            alert("修改成功");
            resetForm();
          }
        })
        .catch((error) => {
          if (error.response?.status === 400) {
            alert("修改操作失败");
          }
        })
        .finally(() => {
          checkPhoneInProgress = false;
          changePhoneInProgress = false;
          newPhoneEffect.value = false;
        });
    } else {
      changePhoneInProgress = false;
    }
  }, 1000);
}

function changePhoneModal() {
  showChangePhoneModal.value = true;
  isButtonActive.value = true;
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

function resetForm() {
  showChangePhoneModal.value = false;
  countryCode.value = "+86";
  newUserPhone.value = "";
  VerificationCode.value = "";
  isPhoneValid.value = false;
  newPhoneEffect.value = false;
  ischeckPhone.value = false;
  isButtonActive.value = false;
  changePhoneInProgress = false;
}

function cancel() {
  resetForm();
}

function close() {
  if (showChangePhoneModal.value) {
    cancel();
    return;
  }
  emit("close");
}

onMounted(() => {
  // reserved for future init
});
</script>
