<template>
  <div class="email-editor">
    <div class="email-editor__head">
      <div>
        <p>Email Studio</p>
        <h3>{{ showChangeEmailModal ? "更换邮箱地址" : "维护登录邮箱" }}</h3>
        <span>
          {{
            showChangeEmailModal
              ? "系统会先检查新邮箱是否可用，再通过验证码完成替换。"
              : "邮箱会用于登录、账单回执和服务通知，建议保持为常用地址。"
          }}
        </span>
      </div>
      <button class="email-editor__ghost" @click="close">关闭</button>
    </div>

    <section v-if="!showChangeEmailModal" class="email-editor__hero">
      <div class="email-editor__badge">@</div>
      <div class="email-editor__summary">
        <small>当前绑定邮箱</small>
        <strong>{{ userEmail || "暂未绑定" }}</strong>
        <span
          >如果你希望接收预约提醒、订单回执和资料通知，建议绑定一个长期使用的邮箱。</span
        >
      </div>
      <button class="email-editor__primary" @click="changeEmailModal">
        更换邮箱
      </button>
    </section>

    <section v-else class="email-editor__panel">
      <div class="email-editor__preview">
        <article>
          <small>当前邮箱</small>
          <strong>{{ userEmail || "暂未绑定" }}</strong>
        </article>
        <article>
          <small>目标邮箱</small>
          <strong>{{ newUserEmail || "等待输入" }}</strong>
        </article>
      </div>

      <form class="email-editor__form" @submit.prevent="changeEmail">
        <label class="editor-field">
          <span>新邮箱地址</span>
          <input
            v-model.trim="newUserEmail"
            type="email"
            placeholder="请输入新的电子邮箱地址"
            @keyup.enter="changeEmail"
            @input="checkEmail"
          />
        </label>

        <p v-if="!newEmailEffect && ischeckEmail" class="editor-error">
          该邮箱已存在，当前无法继续使用。
        </p>

        <div class="verification-strip">
          <label class="editor-field">
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
            class="email-editor__ghost"
            :disabled="
              isgetVerificationCode || !isEmailValid || !newEmailEffect
            "
            @click="getVerificationCode"
          >
            {{ isgetVerificationCode ? `${count} 秒后重发` : "获取验证码" }}
          </button>
        </div>

        <div class="email-editor__tips">
          <article>
            <small>校验状态</small>
            <strong>{{
              !newUserEmail
                ? "等待输入"
                : isEmailValid && newEmailEffect
                ? "邮箱可用"
                : "请先完成校验"
            }}</strong>
          </article>
          <article>
            <small>提交流程</small>
            <span>验证码会发送到新邮箱，输入正确后保存即可完成替换。</span>
          </article>
        </div>

        <div v-if="isButtonActive" class="email-editor__actions">
          <button type="button" class="email-editor__ghost" @click="cancel">
            取消
          </button>
          <button
            type="submit"
            class="email-editor__primary"
            :disabled="!newEmailEffect"
          >
            保存邮箱
          </button>
        </div>
      </form>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { isEmail } from "@/core/auth/utils/authValidators";
import { profileApi } from "@/modules/user/api/userApi";

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
            return profileApi.updateEmail({
              email: newUserEmail.value,
              ticket,
            });
          }

          throw new Error("邮箱验证凭证无效");
        })
        .then((updateResponse) => {
          if (updateResponse?.status === 200) {
            const token = updateResponse.data?.data?.token;
            if (token) {
              store.commit("auth/refreshToken", token);
            }
            store.commit("currentUser/updateUserField", {
              field: "userEmail",
              value: newUserEmail.value,
              userType: store.state.auth.userType,
              userRole: store.state.auth.userRole,
            });
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

<style scoped lang="scss">
.email-editor {
  display: grid;
  gap: 18px;
  padding: 24px;
  border-radius: 30px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: rgba(255, 255, 255, 0.76);
  box-shadow: 0 18px 44px rgba(24, 90, 91, 0.06);
}

.email-editor__head {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 16px;
}

.email-editor__head p,
.email-editor__summary small,
.email-editor__tips small,
.email-editor__preview small {
  margin: 0;
  color: #1f8e89;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.email-editor__head h3 {
  margin: 6px 0 0;
  color: #133f42;
  font-size: 32px;
}

.email-editor__head span,
.email-editor__summary span,
.email-editor__tips span {
  display: block;
  margin-top: 10px;
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.email-editor__hero,
.email-editor__panel {
  padding: 22px;
  border-radius: 28px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.18),
    rgba(243, 197, 155, 0.14)
  );
}

.email-editor__hero {
  display: grid;
  grid-template-columns: 104px minmax(0, 1fr) auto;
  gap: 18px;
  align-items: center;
}

.email-editor__badge {
  width: 104px;
  height: 104px;
  display: grid;
  place-items: center;
  border-radius: 30px;
  background: linear-gradient(135deg, #91ddd2, #f0c29b);
  color: #15474a;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 40px;
  font-weight: 700;
  box-shadow: 0 18px 34px rgba(28, 98, 99, 0.14);
}

.email-editor__summary {
  display: grid;
  gap: 6px;
}

.email-editor__summary strong,
.email-editor__preview strong,
.email-editor__tips strong {
  color: #143f42;
  font-size: 24px;
}

.email-editor__preview {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin-bottom: 16px;
}

.email-editor__preview article,
.email-editor__tips article {
  padding: 16px 18px;
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.74);
}

.email-editor__form {
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

.editor-field input {
  width: 100%;
  padding: 13px 14px;
  border: 1px solid rgba(20, 82, 84, 0.12);
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.94);
  color: #173f42;
  font-size: 14px;
}

.editor-field input:focus {
  outline: none;
  border-color: rgba(24, 128, 127, 0.5);
  box-shadow: 0 0 0 4px rgba(141, 218, 210, 0.18);
}

.editor-error {
  margin: -2px 0 0;
  color: #a94949;
  font-size: 13px;
}

.verification-strip {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 12px;
  align-items: end;
}

.email-editor__tips {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.email-editor__actions {
  display: flex;
  gap: 10px;
  justify-content: flex-end;
  flex-wrap: wrap;
}

.email-editor__ghost,
.email-editor__primary {
  border: none;
  border-radius: 999px;
  padding: 12px 16px;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.email-editor__ghost {
  background: rgba(20, 82, 84, 0.08);
  color: #154144;
}

.email-editor__ghost:disabled,
.email-editor__primary:disabled {
  cursor: not-allowed;
  opacity: 0.56;
}

.email-editor__primary {
  background: linear-gradient(135deg, #167f80, #2ca7a4);
  color: #fff;
  box-shadow: 0 16px 30px rgba(23, 104, 105, 0.22);
}

@media (max-width: 900px) {
  .email-editor__head,
  .email-editor__hero,
  .email-editor__preview,
  .verification-strip,
  .email-editor__tips {
    grid-template-columns: 1fr;
    display: grid;
  }

  .email-editor__badge {
    width: 88px;
    height: 88px;
    border-radius: 24px;
  }
}
</style>
