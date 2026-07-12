<template>
  <Teleport to="body">
    <Transition name="password-dialog">
      <div
        v-if="modelValue"
        class="password-dialog__backdrop"
        role="presentation"
        @mousedown.self="close"
      >
        <section
          class="password-dialog"
          role="dialog"
          aria-modal="true"
          aria-labelledby="password-dialog-title"
          @keydown.esc="close"
        >
          <header class="password-dialog__header">
            <div class="password-dialog__title-wrap">
              <span class="password-dialog__icon" aria-hidden="true">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="2"
                >
                  <rect x="4" y="10" width="16" height="10" rx="2" />
                  <path d="M8 10V7a4 4 0 0 1 8 0v3" />
                </svg>
              </span>
              <div>
                <h2 id="password-dialog-title">修改密码</h2>
                <p>修改成功后，所有设备需要重新登录</p>
              </div>
            </div>
            <button
              class="password-dialog__close"
              type="button"
              aria-label="关闭"
              @click="close"
            >
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
              >
                <path d="m6 6 12 12M18 6 6 18" />
              </svg>
            </button>
          </header>

          <form class="password-dialog__form" @submit.prevent="submit">
            <label class="password-field">
              <span>当前密码</span>
              <span class="password-field__control">
                <input
                  v-model="currentPassword"
                  :type="visible.current ? 'text' : 'password'"
                  autocomplete="current-password"
                  maxlength="128"
                  autofocus
                  placeholder="输入当前密码"
                />
                <button
                  type="button"
                  :aria-label="visible.current ? '隐藏密码' : '显示密码'"
                  @click="visible.current = !visible.current"
                >
                  <svg
                    viewBox="0 0 24 24"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="2"
                  >
                    <path
                      d="M2 12s3.5-6 10-6 10 6 10 6-3.5 6-10 6S2 12 2 12Z"
                    />
                    <circle cx="12" cy="12" r="2.5" />
                  </svg>
                </button>
              </span>
            </label>

            <label class="password-field">
              <span>新密码</span>
              <span class="password-field__control">
                <input
                  v-model="newPassword"
                  :type="visible.next ? 'text' : 'password'"
                  autocomplete="new-password"
                  maxlength="64"
                  placeholder="8 至 64 位，包含字母和数字"
                />
                <button
                  type="button"
                  :aria-label="visible.next ? '隐藏密码' : '显示密码'"
                  @click="visible.next = !visible.next"
                >
                  <svg
                    viewBox="0 0 24 24"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="2"
                  >
                    <path
                      d="M2 12s3.5-6 10-6 10 6 10 6-3.5 6-10 6S2 12 2 12Z"
                    />
                    <circle cx="12" cy="12" r="2.5" />
                  </svg>
                </button>
              </span>
            </label>

            <div class="password-rules" aria-live="polite">
              <span :class="{ 'is-valid': rules.length }">至少 8 位</span>
              <span :class="{ 'is-valid': rules.letter }">包含字母</span>
              <span :class="{ 'is-valid': rules.number }">包含数字</span>
            </div>

            <label class="password-field">
              <span>确认新密码</span>
              <span class="password-field__control">
                <input
                  v-model="confirmPassword"
                  :type="visible.confirm ? 'text' : 'password'"
                  autocomplete="new-password"
                  maxlength="64"
                  placeholder="再次输入新密码"
                />
                <button
                  type="button"
                  :aria-label="visible.confirm ? '隐藏密码' : '显示密码'"
                  @click="visible.confirm = !visible.confirm"
                >
                  <svg
                    viewBox="0 0 24 24"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="2"
                  >
                    <path
                      d="M2 12s3.5-6 10-6 10 6 10 6-3.5 6-10 6S2 12 2 12Z"
                    />
                    <circle cx="12" cy="12" r="2.5" />
                  </svg>
                </button>
              </span>
            </label>

            <p
              v-if="errorMessage"
              class="password-dialog__message password-dialog__message--error"
              role="alert"
            >
              {{ errorMessage }}
            </p>
            <p
              v-if="success"
              class="password-dialog__message password-dialog__message--success"
              role="status"
            >
              {{
                stayLoggedIn
                  ? "密码已修改，其他设备已退出登录"
                  : "密码已修改，正在退出当前账号…"
              }}
            </p>

            <footer class="password-dialog__actions">
              <button
                type="button"
                class="password-dialog__cancel"
                :disabled="submitting || success"
                @click="close"
              >
                取消
              </button>
              <button
                type="submit"
                class="password-dialog__submit"
                :disabled="!canSubmit || submitting || success"
              >
                {{ submitting ? "正在修改…" : "确认修改" }}
              </button>
            </footer>
          </form>
        </section>
      </div>
    </Transition>
  </Teleport>
</template>

<script setup lang="ts">
import { computed, reactive, ref, watch } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { authApi } from "@/core/auth/api/authApi";
import { getHttpErrorMessage } from "@/api/httpError";

const props = defineProps<{ modelValue: boolean }>();
const emit = defineEmits<{
  (_event: "update:modelValue", _value: boolean): void;
  (_event: "password-changed"): void;
}>();

const store = useStore(storeKey);

const currentPassword = ref("");
const newPassword = ref("");
const confirmPassword = ref("");
const submitting = ref(false);
const success = ref(false);
// 改密成功后当前设备是否保持登录：后端返回续签 token 即为 true，无需重新登录。
const stayLoggedIn = ref(false);
const errorMessage = ref("");
const visible = reactive({ current: false, next: false, confirm: false });

const rules = computed(() => ({
  length: newPassword.value.length >= 8 && newPassword.value.length <= 64,
  letter: /[A-Za-z]/.test(newPassword.value),
  number: /\d/.test(newPassword.value),
}));

const canSubmit = computed(
  () =>
    currentPassword.value.length > 0 &&
    rules.value.length &&
    rules.value.letter &&
    rules.value.number &&
    newPassword.value !== currentPassword.value &&
    confirmPassword.value === newPassword.value
);

const reset = () => {
  currentPassword.value = "";
  newPassword.value = "";
  confirmPassword.value = "";
  submitting.value = false;
  success.value = false;
  stayLoggedIn.value = false;
  errorMessage.value = "";
  visible.current = false;
  visible.next = false;
  visible.confirm = false;
};

const close = () => {
  if (submitting.value || success.value) return;
  emit("update:modelValue", false);
};

const submit = async () => {
  if (!canSubmit.value || submitting.value) return;
  submitting.value = true;
  errorMessage.value = "";

  try {
    const res = await authApi.changePassword({
      currentPassword: currentPassword.value,
      newPassword: newPassword.value,
    });
    success.value = true;

    const nextToken = res.data?.data?.token;
    if (nextToken) {
      // 后端已续签：换上新 token，当前设备无缝保持登录，不再强制登出重登。
      store.commit("auth/refreshToken", nextToken);
      stayLoggedIn.value = true;
      window.setTimeout(() => emit("update:modelValue", false), 1100);
    } else {
      // 兜底：未拿到续签 token（异常情况），沿用旧流程强制重新登录。
      window.setTimeout(() => emit("password-changed"), 900);
    }
  } catch (error) {
    errorMessage.value = getHttpErrorMessage(error, "密码修改失败，请稍后重试");
  } finally {
    submitting.value = false;
  }
};

watch(
  () => props.modelValue,
  (open) => {
    if (open) reset();
  }
);
</script>

<style scoped>
.password-dialog__backdrop {
  position: fixed;
  inset: 0;
  z-index: 1200;
  display: grid;
  place-items: center;
  padding: 20px;
  background: rgba(15, 23, 42, 0.42);
}

.password-dialog {
  width: min(440px, 100%);
  overflow: hidden;
  border: 1px solid #e5e7eb;
  border-radius: 8px;
  background: #fff;
  box-shadow: 0 20px 50px rgba(15, 23, 42, 0.2);
  color: #0f172a;
}

.password-dialog__header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  padding: 22px 24px 18px;
  border-bottom: 1px solid #eef0f3;
}

.password-dialog__title-wrap {
  display: flex;
  align-items: center;
  gap: 12px;
}

.password-dialog__icon {
  display: grid;
  width: 36px;
  height: 36px;
  flex: 0 0 auto;
  place-items: center;
  border-radius: 6px;
  background: #eef2ff;
  color: #4f46e5;
}

.password-dialog__icon svg,
.password-dialog__close svg,
.password-field__control button svg {
  width: 18px;
  height: 18px;
}

.password-dialog h2 {
  margin: 0;
  font-size: 18px;
  font-weight: 700;
  line-height: 24px;
  letter-spacing: 0;
}

.password-dialog__header p {
  margin: 3px 0 0;
  color: #64748b;
  font-size: 13px;
  line-height: 20px;
}

.password-dialog__close,
.password-field__control button {
  display: grid;
  place-items: center;
  border: 0;
  background: transparent;
  color: #64748b;
  cursor: pointer;
}

.password-dialog__close {
  width: 32px;
  height: 32px;
  border-radius: 6px;
}

.password-dialog__close:hover,
.password-field__control button:hover {
  background: #f1f5f9;
  color: #334155;
}

.password-dialog__form {
  padding: 20px 24px 22px;
}

.password-field {
  display: block;
  margin-bottom: 16px;
  color: #334155;
  font-size: 13px;
  font-weight: 600;
}

.password-field__control {
  display: flex;
  align-items: center;
  height: 42px;
  margin-top: 7px;
  border: 1px solid #d7dce3;
  border-radius: 6px;
  background: #fff;
}

.password-field__control:focus-within {
  border-color: #6366f1;
  box-shadow: 0 0 0 3px rgba(99, 102, 241, 0.12);
}

.password-field__control input {
  min-width: 0;
  flex: 1;
  height: 100%;
  padding: 0 12px;
  border: 0;
  outline: 0;
  background: transparent;
  color: #0f172a;
  font: inherit;
  font-weight: 500;
  letter-spacing: 0;
}

.password-field__control input::placeholder {
  color: #94a3b8;
}

.password-field__control button {
  width: 40px;
  height: 40px;
  flex: 0 0 auto;
}

.password-rules {
  display: flex;
  gap: 16px;
  margin: -6px 0 16px;
  color: #94a3b8;
  font-size: 12px;
}

.password-rules span::before {
  content: "";
  display: inline-block;
  width: 6px;
  height: 6px;
  margin-right: 6px;
  border-radius: 50%;
  background: #cbd5e1;
  vertical-align: 1px;
}

.password-rules .is-valid {
  color: #15803d;
}

.password-rules .is-valid::before {
  background: #22c55e;
}

.password-dialog__message {
  margin: 0 0 14px;
  padding: 9px 11px;
  border-radius: 6px;
  font-size: 13px;
  line-height: 20px;
}

.password-dialog__message--error {
  background: #fef2f2;
  color: #b91c1c;
}

.password-dialog__message--success {
  background: #f0fdf4;
  color: #166534;
}

.password-dialog__actions {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  padding-top: 4px;
}

.password-dialog__actions button {
  min-width: 92px;
  height: 38px;
  padding: 0 16px;
  border-radius: 6px;
  font-size: 13px;
  font-weight: 700;
  cursor: pointer;
}

.password-dialog__cancel {
  border: 1px solid #d7dce3;
  background: #fff;
  color: #334155;
}

.password-dialog__submit {
  border: 1px solid #4f46e5;
  background: #4f46e5;
  color: #fff;
}

.password-dialog__actions button:disabled {
  cursor: not-allowed;
  opacity: 0.5;
}

.password-dialog-enter-active,
.password-dialog-leave-active {
  transition: opacity 0.16s ease;
}

.password-dialog-enter-active .password-dialog,
.password-dialog-leave-active .password-dialog {
  transition: transform 0.16s ease, opacity 0.16s ease;
}

.password-dialog-enter-from,
.password-dialog-leave-to {
  opacity: 0;
}

.password-dialog-enter-from .password-dialog,
.password-dialog-leave-to .password-dialog {
  opacity: 0;
  transform: translateY(8px);
}

@media (max-width: 520px) {
  .password-dialog__backdrop {
    align-items: end;
    padding: 0;
  }

  .password-dialog {
    width: 100%;
    border-right: 0;
    border-bottom: 0;
    border-left: 0;
    border-radius: 8px 8px 0 0;
  }

  .password-dialog__header,
  .password-dialog__form {
    padding-right: 20px;
    padding-left: 20px;
  }

  .password-rules {
    gap: 10px;
  }
}

@media (prefers-reduced-motion: reduce) {
  .password-dialog-enter-active,
  .password-dialog-leave-active,
  .password-dialog-enter-active .password-dialog,
  .password-dialog-leave-active .password-dialog {
    transition: none;
  }
}
</style>
