<template>
  <div class="phone-editor">
    <div class="phone-editor__head">
      <div>
        <p>Phone Studio</p>
        <h3>{{ showChangePhoneModal ? "更换手机号" : "维护登录手机号" }}</h3>
        <span>
          {{
            showChangePhoneModal
              ? "先校验新手机号是否可用，再通过短信验证码完成替换。"
              : "当前手机号会用于登录、预约通知和客服联系。"
          }}
        </span>
      </div>
      <button class="phone-editor__ghost" @click="close">关闭</button>
    </div>

    <section v-if="!showChangePhoneModal" class="phone-editor__hero">
      <div class="phone-editor__badge">
        <span>{{ phoneInitial }}</span>
      </div>
      <div class="phone-editor__summary">
        <small>当前绑定手机号</small>
        <strong>{{ userPhone || "暂未绑定" }}</strong>
        <span
          >如果你已经更换号码，建议立刻更新，避免收不到预约和服务提醒。</span
        >
      </div>
      <button class="phone-editor__primary" @click="changePhoneModal">
        更换手机号
      </button>
    </section>

    <section v-else class="phone-editor__panel">
      <div class="phone-editor__preview">
        <article>
          <small>当前号码</small>
          <strong>{{ userPhone || "暂未绑定" }}</strong>
        </article>
        <article>
          <small>目标号码</small>
          <strong>{{ fullPhonePreview }}</strong>
        </article>
      </div>

      <form class="phone-editor__form" @submit.prevent="changePhone">
        <label class="editor-field editor-field--compact">
          <span>国家 / 地区</span>
          <select v-model="countryCode">
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
        </label>

        <label class="editor-field">
          <span>新手机号</span>
          <input
            v-model.trim="newUserPhone"
            type="tel"
            placeholder="请输入新的手机号码"
            @keyup.enter="changePhone"
            @input="checkPhone"
          />
        </label>

        <p v-if="!newPhoneEffect && ischeckPhone" class="editor-error">
          该手机号已存在，当前无法继续使用。
        </p>

        <div class="verification-strip">
          <label class="editor-field">
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
            class="phone-editor__ghost"
            :disabled="
              isgetVerificationCode || !isPhoneValid || !newPhoneEffect
            "
            @click="getVerificationCode"
          >
            {{ isgetVerificationCode ? `${count} 秒后重发` : "获取验证码" }}
          </button>
        </div>

        <div class="phone-editor__tips">
          <article>
            <small>校验状态</small>
            <strong>{{
              !newUserPhone
                ? "等待输入"
                : isPhoneValid && newPhoneEffect
                ? "号码可用"
                : "请先完成校验"
            }}</strong>
          </article>
          <article>
            <small>提交流程</small>
            <span>发送验证码后输入收到的短信，再点击保存完成替换。</span>
          </article>
        </div>

        <div v-if="isButtonActive" class="phone-editor__actions">
          <button type="button" class="phone-editor__ghost" @click="cancel">
            取消
          </button>
          <button
            type="submit"
            class="phone-editor__primary"
            :disabled="!isPhoneValid || !newPhoneEffect"
          >
            保存手机号
          </button>
        </div>
      </form>
    </section>
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

const phoneInitial = computed(() => {
  const currentPhone = userPhone.value || "P";
  return currentPhone.slice(-2).toUpperCase();
});

const fullPhonePreview = computed(() => {
  if (!newUserPhone.value) {
    return "等待输入";
  }
  return `${countryCode.value} ${newUserPhone.value}`;
});

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

<style scoped lang="scss">
.phone-editor {
  display: grid;
  gap: 18px;
  padding: 24px;
  border-radius: 16px;
  border: 1px solid rgba(47, 158, 143, 0.08);
  background: rgba(255, 255, 255, 0.76);
  box-shadow: 0 18px 44px rgba(47, 158, 143, 0.06);
}

.phone-editor__head {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 16px;
}

.phone-editor__head p,
.phone-editor__summary small,
.phone-editor__tips small,
.phone-editor__preview small {
  margin: 0;
  color: #2f9e8f;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.phone-editor__head h3 {
  margin: 6px 0 0;
  color: #1f3a36;
  font-size: 32px;
}

.phone-editor__head span,
.phone-editor__summary span,
.phone-editor__tips span {
  display: block;
  margin-top: 10px;
  color: #6b7d77;
  line-height: 1.8;
  font-size: 14px;
}

.phone-editor__hero,
.phone-editor__panel {
  padding: 22px;
  border-radius: 16px;
  border: 1px solid rgba(47, 158, 143, 0.08);
  background: linear-gradient(
    135deg,
    rgba(56, 178, 163, 0.18),
    rgba(255, 217, 176, 0.14)
  );
}

.phone-editor__hero {
  display: grid;
  grid-template-columns: 104px minmax(0, 1fr) auto;
  gap: 18px;
  align-items: center;
}

.phone-editor__badge {
  width: 104px;
  height: 104px;
  display: grid;
  place-items: center;
  border-radius: 16px;
  background: linear-gradient(135deg, #cfe7e1, #ffd9b0);
  color: #1f3a36;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 34px;
  font-weight: 700;
  box-shadow: 0 18px 34px rgba(47, 158, 143, 0.14);
}

.phone-editor__summary {
  display: grid;
  gap: 6px;
}

.phone-editor__summary strong,
.phone-editor__preview strong,
.phone-editor__tips strong {
  color: #1f3a36;
  font-size: 24px;
}

.phone-editor__preview {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin-bottom: 16px;
}

.phone-editor__preview article,
.phone-editor__tips article {
  padding: 16px 18px;
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.74);
}

.phone-editor__form {
  display: grid;
  gap: 14px;
}

.editor-field {
  display: grid;
  gap: 8px;
}

.editor-field--compact {
  max-width: 240px;
}

.editor-field span {
  color: #1f3a36;
  font-size: 13px;
  font-weight: 700;
}

.editor-field input,
.editor-field select {
  width: 100%;
  padding: 13px 14px;
  border: 1px solid rgba(47, 158, 143, 0.12);
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.94);
  color: #1f3a36;
  font-size: 14px;
}

.editor-field input:focus,
.editor-field select:focus {
  outline: none;
  border-color: rgba(47, 158, 143, 0.5);
  box-shadow: 0 0 0 4px rgba(56, 178, 163, 0.18);
}

.editor-error {
  margin: -2px 0 0;
  color: #be4b5b;
  font-size: 13px;
}

.verification-strip {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 12px;
  align-items: end;
}

.phone-editor__tips {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.phone-editor__actions {
  display: flex;
  gap: 10px;
  justify-content: flex-end;
  flex-wrap: wrap;
}

.phone-editor__ghost,
.phone-editor__primary {
  border: none;
  border-radius: 999px;
  padding: 12px 16px;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.phone-editor__ghost {
  background: rgba(47, 158, 143, 0.08);
  color: #1f3a36;
}

.phone-editor__ghost:disabled,
.phone-editor__primary:disabled {
  cursor: not-allowed;
  opacity: 0.56;
}

.phone-editor__primary {
  background: linear-gradient(135deg, #1f7a6c, #2f9e8f);
  color: #fff;
  box-shadow: 0 16px 30px rgba(47, 158, 143, 0.22);
}

@media (max-width: 900px) {
  .phone-editor__head,
  .phone-editor__hero,
  .phone-editor__preview,
  .verification-strip,
  .phone-editor__tips {
    grid-template-columns: 1fr;
    display: grid;
  }

  .phone-editor__badge {
    width: 88px;
    height: 88px;
    border-radius: 16px;
  }
}
</style>
