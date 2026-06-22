<template>
  <!-- 仅渲染登录卡：左侧品牌与整页布局由 App.vue 的 guest-shell 提供 -->
  <div v-show="!isLoggedIn" class="login-card">
    <h2 class="login-card__title">登录</h2>

    <!-- 方式分段选择 -->
    <div class="login-methods" role="tablist" aria-label="登录方式">
      <button
        v-for="method in methods"
        :key="method.key"
        type="button"
        role="tab"
        class="login-methods__tab"
        :class="{
          'login-methods__tab--active': activeMethod === method.key,
        }"
        :aria-selected="activeMethod === method.key"
        @click="activeMethod = method.key"
      >
        {{ method.label }}
      </button>
    </div>

    <!-- 账号登录 -->
    <form
      v-if="activeMethod === 'account'"
      class="login-form"
      @submit.prevent="handleLogin"
    >
      <label class="login-field">
        <span class="login-field__label">账号</span>
        <input
          v-model="Account"
          :name="'emailAddress_' + randomSuffix"
          type="text"
          class="login-field__input"
          placeholder="邮箱 / 手机号"
          autocomplete="new-password"
        />
      </label>

      <label class="login-field">
        <span class="login-field__label">密码</span>
        <span class="login-field__control">
          <input
            v-model="Password"
            :type="inputType"
            class="login-field__input"
            placeholder="请输入密码"
          />
          <button
            type="button"
            class="login-field__eye"
            :aria-label="showPassword ? '隐藏密码' : '显示密码'"
            @click="togglePasswordVisibility"
          >
            <svg
              width="20"
              height="20"
              viewBox="0 0 20 20"
              fill="none"
              xmlns="http://www.w3.org/2000/svg"
            >
              <path
                v-show="!showPassword"
                fill-rule="evenodd"
                clip-rule="evenodd"
                d="M17.5753 6.85456C17.7122 6.71896 17.8939 6.63806 18.0866 6.63806C18.7321 6.63806 19.0436 7.42626 18.5748 7.87006C18.1144 8.30554 17.457 8.69885 16.6478 9.03168L18.1457 10.5296C18.2746 10.6585 18.349 10.8359 18.349 11.0204C18.349 11.2049 18.2746 11.3823 18.1457 11.5113C18.0167 11.6402 17.8393 11.7146 17.6548 11.7146C17.4703 11.7146 17.2929 11.6402 17.164 11.5113L15.3409 9.68819C15.2898 9.63708 15.247 9.57838 15.2141 9.51428C14.4874 9.71293 13.6876 9.87122 12.8344 9.98119L13.2874 12.5472C13.3427 12.9061 13.0966 13.2434 12.7217 13.3367C12.5429 13.3683 12.3589 13.3285 12.2091 13.2259C12.0592 13.1234 11.9555 12.9663 11.9202 12.7882L11.4725 10.2491C11.4645 10.2039 11.4611 10.1581 11.4621 10.1125C10.9858 10.1428 10.4976 10.1586 10.0002 10.1586C9.57059 10.1586 9.14778 10.1468 8.73362 10.1241L8.27808 12.7881C8.21416 13.1507 7.86864 13.3939 7.47398 13.3512C7.29268 13.3192 7.1315 13.2166 7.0259 13.0658C6.9203 12.915 6.87892 12.7284 6.91088 12.5471L7.35858 10.008C6.50085 9.90284 5.6941 9.75191 4.95838 9.56025C4.93012 9.60634 4.89634 9.64933 4.85748 9.68819L3.03438 11.5113C2.90543 11.6402 2.72803 11.7146 2.54353 11.7146C2.35903 11.7146 2.18163 11.6402 2.05268 11.5113C1.92373 11.3823 1.84937 11.2049 1.84937 11.0204C1.84937 10.8359 1.92373 10.6585 2.05268 10.5296L3.49373 9.08855C2.6197 8.744 1.91247 8.33062 1.42559 7.87006C0.956591 7.42636 1.26799 6.63816 1.91359 6.63816C2.10629 6.63816 2.28789 6.71896 2.42489 6.85456C2.70009 7.12696 3.19529 7.45886 3.98459 7.77796C5.54429 8.40856 7.73699 8.77016 10.0001 8.77016C12.2632 8.77016 14.4558 8.40856 16.0156 7.77796C16.8049 7.45886 17.3001 7.12696 17.5753 6.85456Z"
                fill="#9499A0"
              />
              <path
                v-show="showPassword"
                fill-rule="evenodd"
                clip-rule="evenodd"
                d="M2.11069 9.43732C3.21647 7.77542 5.87904 4.58331 9.89458 4.58331C13.8801 4.58331 16.6483 7.72502 17.8345 9.4049C18.0905 9.76747 18.0905 10.2325 17.8345 10.5951C16.6483 12.2749 13.8801 15.4166 9.89458 15.4166C5.87904 15.4166 3.21647 12.2245 2.11069 10.5626C1.88009 10.2161 1.88009 9.7839 2.11069 9.43732ZM9.89458 3.33331C5.19832 3.33331 2.20919 7.03277 1.07001 8.74489C0.560324 9.51091 0.560323 10.4891 1.07001 11.2551C2.20919 12.9672 5.19832 16.6666 9.89458 16.6666C14.5412 16.6666 17.6368 13.0422 18.8556 11.3161C19.4168 10.5213 19.4168 9.4787 18.8556 8.68391C17.6368 6.95774 14.5412 3.33331 9.89458 3.33331ZM7.29165 9.99998C7.29165 8.50421 8.50421 7.29165 9.99998 7.29165C11.4958 7.29165 12.7083 8.50421 12.7083 9.99998C12.7083 11.4958 11.4958 12.7083 9.99998 12.7083C8.50421 12.7083 7.29165 11.4958 7.29165 9.99998ZM9.99998 6.04165C7.81385 6.04165 6.04165 7.81385 6.04165 9.99998C6.04165 12.1861 7.81385 13.9583 9.99998 13.9583C12.1861 13.9583 13.9583 12.1861 13.9583 9.99998C13.9583 7.81385 12.1861 6.04165 9.99998 6.04165Z"
                fill="#9499A0"
              />
            </svg>
          </button>
        </span>
      </label>

      <button type="submit" class="login-submit">登录</button>
    </form>

    <!-- 微信登录 -->
    <div v-else-if="activeMethod === 'wechat'" class="login-wechat">
      <div class="login-wechat__qr">
        <span>二维码</span>
      </div>
      <p class="login-wechat__tip">请使用微信扫一扫登录</p>
      <button type="button" class="login-secondary">刷新二维码</button>
    </div>

    <!-- 手机登录 -->
    <div v-else class="login-form">
      <label class="login-field">
        <span class="login-field__label">手机号</span>
        <span class="login-field__control">
          <span class="login-field__prefix">+86</span>
          <input
            ref="phoneNumberInputRef"
            type="text"
            class="login-field__input"
            placeholder="请输入手机号"
            pattern="[0-9]*"
            inputmode="numeric"
            maxlength="11"
          />
        </span>
      </label>
      <label class="login-field">
        <span class="login-field__label">验证码</span>
        <span class="login-field__control">
          <input
            type="text"
            class="login-field__input"
            placeholder="请输入验证码"
            pattern="[0-9]*"
            maxlength="6"
            autocomplete="new-password"
          />
          <button type="button" class="login-field__code">获取验证码</button>
        </span>
      </label>
      <button type="button" class="login-submit">登录</button>
    </div>

    <!-- 协议同意 -->
    <label class="login-agree" :class="{ 'login-agree--error': agreeError }">
      <input v-model="agreed" type="checkbox" class="login-agree__box" />
      <span class="login-agree__text">
        同意
        <a :href="agreementLinks.service" target="_blank">《服务协议》</a>、
        <a :href="agreementLinks.privacy" target="_blank">《隐私政策》</a>、
        <a :href="agreementLinks.kids" target="_blank">《儿童隐私保护声明》</a>
        和
        <a :href="agreementLinks.thirdParty" target="_blank"
          >《第三方信息共享清单》</a
        >
      </span>
    </label>
    <p v-if="agreeError" class="login-agree__hint">请先阅读并同意相关协议</p>

    <!-- 辅助入口 -->
    <div class="login-links">
      <router-link to="/find/password">找回密码</router-link>
      <router-link to="/register/account" @click="goRegister"
        >注册账号</router-link
      >
      <router-link to="/feedback">意见反馈</router-link>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onUnmounted } from "vue";
import { useStore } from "vuex";
import { useRouter } from "vue-router";
import { AppHttpError } from "@/api/httpError";
import { getHomeRouteByUserType } from "@/core/auth/utils/authRedirect";
import { isEmail, isPhone } from "@/core/auth/utils/authValidators";

const store = useStore();
const router = useRouter();

type LoginMethod = "account" | "wechat" | "phone";

const methods: Array<{ key: LoginMethod; label: string }> = [
  { key: "account", label: "账号登录" },
  { key: "wechat", label: "微信登录" },
  { key: "phone", label: "手机登录" },
];

const agreementLinks = {
  service: "https://rule.tencent.com/rule/399ab3d0-4989-4f34-9d7b-99c579b4cbdf",
  privacy:
    "https://privacy.qq.com/document/priview/3fab9c7fc1424ebda42c3ce488322c8a",
  kids: "https://privacy.qq.com/mb/policy/kids-privacypolicy",
  thirdParty:
    "https://privacy.qq.com/document/preview/ba4294dc9d4a45a89f3d682eb07a489b",
};

// 本地表单状态（登录方式、协议等只属于本页，无需进 Vuex）
const activeMethod = ref<LoginMethod>("account");
const agreed = ref(false);
const agreeError = ref(false);
const Account = ref("");
const Password = ref("");
const showPassword = ref(false);
const phoneNumberInputRef = ref<HTMLInputElement | null>(null);
const randomSuffix = Math.random().toString(36).slice(2);

let loginInProgress = false;

const inputType = computed(() => (showPassword.value ? "text" : "password"));
const isLoggedIn = computed(() => store.state.auth.isLoggedIn);

const resolveLoginIdentifier = (rawValue: string) => {
  const value = rawValue.trim();

  if (isEmail(value)) {
    return { type: "email" as const, identifier: value };
  }

  if (isPhone(value)) {
    return {
      type: "phone" as const,
      identifier: value.startsWith("+86")
        ? value
        : `+86${value.replace(/^\+86/, "")}`,
    };
  }

  return null;
};

const togglePasswordVisibility = () => {
  showPassword.value = !showPassword.value;
};

const getLoginErrorMessage = (error: unknown) => {
  if (error instanceof AppHttpError) {
    switch (error.kind) {
      case "unauthorized":
      case "validation":
        return "账号或密码错误，请检查后重试";
      case "rate-limited":
        return "请求过于频繁，请稍等一分钟后再试";
      case "network":
      case "timeout":
        return "无法连接后端服务，请确认 PetManager 后端正在运行";
      case "server":
        return "服务器处理登录失败，请稍后重试";
      default:
        return error.message || "登录失败，请稍后重试";
    }
  }

  if (error instanceof Error && error.message.includes("缺少 user 字段")) {
    return "登录返回数据异常，请稍后重试";
  }

  return "登录请求出错，请稍后重试";
};

// 进入注册页：App.vue 的 guest-shell 依据 showRegister 切换到注册路由视图
const goRegister = () => {
  store.commit("ui/upDataLoginButtonActive", { showRegister: true });
};

const handleLogin = () => {
  if (loginInProgress) {
    return;
  }

  // 未同意协议时就近提示，不再用全屏弹层打断
  if (!agreed.value) {
    agreeError.value = true;
    return;
  }
  agreeError.value = false;

  const resolvedIdentifier = resolveLoginIdentifier(Account.value);
  if (!resolvedIdentifier) {
    alert("请输入正确的邮箱或手机号");
    return;
  }

  if (!Password.value.trim()) {
    alert("请输入密码");
    return;
  }

  Account.value = resolvedIdentifier.identifier;
  loginInProgress = true;

  store
    .dispatch("auth/login", {
      identifier: resolvedIdentifier.identifier,
      password: Password.value,
    })
    .then((response) => {
      const loginData = response.data?.data ?? response.data;
      const user = loginData?.user;

      if (response.status === 200) {
        if (!user) {
          throw new Error("登录返回数据缺少 user 字段");
        }

        router.push(getHomeRouteByUserType(user.type_id, user.type_name));
      }
    })
    .catch((error) => {
      console.error("Login request failed:", error);
      alert(getLoginErrorMessage(error));
    })
    .finally(() => {
      loginInProgress = false;
    });
};

onUnmounted(() => {
  Account.value = "";
  Password.value = "";
  showPassword.value = false;
  agreed.value = false;
  agreeError.value = false;
  loginInProgress = false;
});
</script>

<style scoped>
/* 登录卡：宽度交给 App.vue 的 .login-panel 容器居中，这里只管卡片本身 */
.login-card {
  width: 100%;
  max-width: 380px;
  background: #ffffff;
  border: 1px solid #d4ddd9;
  border-radius: 18px;
  box-shadow: 0 22px 48px rgba(28, 50, 80, 0.16);
  padding: 28px 26px 22px;
  box-sizing: border-box;
  font-family: "PingFang SC", "Noto Sans SC", "Segoe UI", system-ui, sans-serif;
}

.login-card__title {
  margin: 0 0 18px;
  font-size: 22px;
  font-weight: 700;
  color: #18373a;
}

/* 分段方式选择 */
.login-methods {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 6px;
  padding: 4px;
  border-radius: 12px;
  background: #f4f7f4;
  margin-bottom: 20px;
}

.login-methods__tab {
  height: 38px;
  border: 0;
  border-radius: 8px;
  background: transparent;
  color: #6d7b72;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  transition: background 0.15s ease, color 0.15s ease;
}

.login-methods__tab--active {
  background: #1e5eff;
  color: #ffffff;
  box-shadow: 0 6px 14px rgba(30, 94, 255, 0.24);
}

/* 表单 */
.login-form {
  display: grid;
  gap: 14px;
}

.login-field {
  display: grid;
  gap: 6px;
}

.login-field__label {
  font-size: 12px;
  font-weight: 600;
  color: #6d7b72;
}

.login-field__control {
  display: flex;
  align-items: center;
  border: 1px solid #dfe7df;
  border-radius: 8px;
  background: #ffffff;
  overflow: hidden;
}

.login-field__control:focus-within {
  border-color: #1e5eff;
  box-shadow: 0 0 0 3px rgba(30, 94, 255, 0.12);
}

.login-field__input {
  flex: 1;
  min-width: 0;
  height: 42px;
  border: 1px solid #dfe7df;
  border-radius: 8px;
  padding: 0 12px;
  font-size: 13px;
  color: #1d3429;
  background: #ffffff;
  box-sizing: border-box;
  outline: none;
}

.login-field__input:focus {
  border-color: #1e5eff;
  box-shadow: 0 0 0 3px rgba(30, 94, 255, 0.12);
}

/* 控件组合内的 input 去掉自身边框，由外层 control 统一描边 */
.login-field__control .login-field__input {
  border: 0;
  box-shadow: none;
}

.login-field__prefix {
  padding: 0 10px;
  font-size: 13px;
  color: #6d7b72;
  border-right: 1px solid #dfe7df;
}

.login-field__eye {
  display: grid;
  place-items: center;
  width: 42px;
  height: 42px;
  border: 0;
  background: transparent;
  cursor: pointer;
}

.login-field__code {
  flex: 0 0 auto;
  height: 30px;
  margin-right: 6px;
  padding: 0 12px;
  border: 1px solid #dfe7df;
  border-radius: 8px;
  background: #f4f7f4;
  color: #1e5eff;
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
}

.login-submit {
  margin-top: 4px;
  height: 44px;
  border: 0;
  border-radius: 8px;
  background: #1e5eff;
  color: #ffffff;
  font-size: 15px;
  font-weight: 700;
  cursor: pointer;
  transition: background 0.15s ease, box-shadow 0.15s ease;
}

.login-submit:hover {
  background: #1850e0;
  box-shadow: 0 10px 20px rgba(30, 94, 255, 0.22);
}

/* 微信 */
.login-wechat {
  display: grid;
  justify-items: center;
  gap: 12px;
  padding: 10px 0;
}

.login-wechat__qr {
  width: 168px;
  height: 168px;
  border-radius: 12px;
  border: 1px solid #dfe7df;
  background: #f4f7f4;
  display: grid;
  place-items: center;
  color: #6d7b72;
  font-size: 13px;
}

.login-wechat__tip {
  margin: 0;
  font-size: 13px;
  color: #6d7b72;
}

.login-secondary {
  height: 36px;
  padding: 0 16px;
  border: 1px solid #dfe7df;
  border-radius: 8px;
  background: #f4f7f4;
  color: #1e5eff;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
}

/* 协议 */
.login-agree {
  display: flex;
  align-items: flex-start;
  gap: 8px;
  margin-top: 18px;
  cursor: pointer;
}

.login-agree__box {
  margin-top: 2px;
  width: 16px;
  height: 16px;
  accent-color: #1e5eff;
  flex: 0 0 auto;
}

.login-agree__text {
  font-size: 12px;
  line-height: 1.6;
  color: #6d7b72;
}

.login-agree__text a {
  color: #1e5eff;
  text-decoration: none;
}

.login-agree__text a:hover {
  text-decoration: underline;
}

.login-agree--error .login-agree__text {
  color: #b04455;
}

.login-agree__hint {
  margin: 6px 0 0 24px;
  font-size: 12px;
  color: #b04455;
}

/* 辅助入口 */
.login-links {
  display: flex;
  margin-top: 14px;
  padding-top: 14px;
  border-top: 1px solid #eef1ee;
}

.login-links a {
  flex: 1;
  text-align: center;
  font-size: 13px;
  color: #6d7b72;
  text-decoration: none;
}

/* 相邻链接之间的竖线分隔符 */
.login-links a + a {
  border-left: 1px solid #e3e6e3;
}

.login-links a:hover {
  color: #1e5eff;
}
</style>
