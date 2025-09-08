<template>
  <div v-show="!isLoggedIn">
    <div class="web-login-mask">
      <div class="login-container">
        <div v-show="!isinitLoginActive" class="login-back">
          <button @click="back" class="login-back-button">
            <img src="@/assets/photo/上一步.svg" alt="上一步" />
          </button>
        </div>
        <div class="login-cha">
          <button @click="handleCloseLoginClick" class="login-cha-button">
            <img src="@/assets/photo/叉.svg" alt="叉" />
          </button>
        </div>
        <div
          v-show="isinitLoginActive && LoginGrade === 1"
          class="login-init-container"
        >
          <span class="init-login-title">登录</span>
          <div class="init-login-Acount">
            <button
              class="init-login-Acount-button"
              @click="setAcountLoginClick"
            >
              <span>账号登录</span>
            </button>
          </div>
          <div class="init-login-WeChat">
            <button
              class="init-login-WeChat-button"
              @click="setWeChatLoginClick"
            >
              <span>微信登录</span>
            </button>
          </div>
          <div class="init-login-Phone">
            <button class="init-login-Phone-button" @click="setPhoneLoginClick">
              <span>手机登录</span>
            </button>
          </div>
          <div class="init-login-line-container">
            <div class="init-login-line">
              <div class="init-login-line-button-container">
                <button class="init-login-line-button" @click="setChoiceActive">
                  <img
                    v-if="choiceActive"
                    src="@/assets/photo/钩.svg"
                    alt="钩"
                  />
                  <img v-else src="@/assets/photo/未选中.svg" alt="未选中" />
                </button>
              </div>
              <span
                class="init-login-line-text"
                style="font-size: 14px; max-width: 380px"
                >同意
                <a
                  href="https://rule.tencent.com/rule/399ab3d0-4989-4f34-9d7b-99c579b4cbdf"
                  target="_blank"
                  >《服务协议》
                </a>
                、
                <a
                  href="https://privacy.qq.com/document/priview/3fab9c7fc1424ebda42c3ce488322c8a"
                  target="_blank"
                  >《隐私政策》
                </a>
                、
                <a
                  href="https://privacy.qq.com/mb/policy/kids-privacypolicy"
                  target="_blank"
                  >《儿童隐私保护声明》
                </a>
                和
                <a
                  href="https://privacy.qq.com/document/preview/ba4294dc9d4a45a89f3d682eb07a489b"
                  target="_blank"
                  >《第三方信息共享清单》
                </a>
              </span>
              <div v-show="isAllowButtonActive" class="login-line-tip-mask">
                <div class="login-line-tip">
                  <span
                    class="login-line-tip-text"
                    style="font-size: 21px; max-width: 380px"
                    >同意
                    <a
                      href="https://rule.tencent.com/rule/399ab3d0-4989-4f34-9d7b-99c579b4cbdf"
                      target="_blank"
                      >《服务协议》
                    </a>
                    、
                    <a
                      href="https://privacy.qq.com/document/priview/3fab9c7fc1424ebda42c3ce488322c8a"
                      target="_blank"
                      >《隐私政策》
                    </a>
                    、
                    <a
                      href="https://privacy.qq.com/mb/policy/kids-privacypolicy"
                      target="_blank"
                      >《儿童隐私保护声明》
                    </a>
                    和
                    <a
                      href="https://privacy.qq.com/document/preview/ba4294dc9d4a45a89f3d682eb07a489b"
                      target="_blank"
                      >《第三方信息共享清单》
                    </a>
                  </span>
                  <button class="login-line-tip-allow-button" @click="allow">
                    同意并继续
                  </button>
                  <button class="login-line-tip-cancel-button" @click="cancel">
                    取消
                  </button>
                </div>
              </div>
            </div>
          </div>
        </div>
        <div
          v-show="isAcountLoginButtonActive && LoginGrade === 2 && choiceActive"
          class="login-Acount-container"
        >
          <div class="login-acount">
            <div class="login-container-header">
              <span class="login-container-title">账号登录</span>
            </div>
            <div class="login-form">
              <div class="login-form-title-container">
                <div class="login-form-title">
                  <span>密码登录</span>
                </div>
                <div class="login-form-tips">
                  <span>推荐使用快捷登录，防止盗号。</span>
                </div>
              </div>
              <form @submit.prevent="handleLogin">
                <!-- 当表单提交时（用户点击提交按钮或按回车）
                首先阻止表单的默认提交行为（页面不会刷新或跳转）
                然后调用组件中的 handleLogin 方法来处理登录逻辑 -->
                <div class="login-form-UserName">
                  <input
                    placeholder="Email || Phone"
                    :name="'emailAddress_' + Math.random()"
                    type="text"
                    class="login-form-UserName-input"
                    autocomplete="new-password"
                    @change="handleInput"
                  />
                </div>
                <div class="login-form-Password">
                  <input
                    :type="inputType"
                    v-model="Password"
                    placeholder="Password"
                    class="login-form-Password-input"
                  />
                  <div class="login-form-Password-eye">
                    <svg
                      @click="togglePasswordVisibility"
                      data-v-1a96ced4=""
                      width="20"
                      height="20"
                      viewBox="0 0 20 20"
                      fill="none"
                      xmlns="http://www.w3.org/2000/svg"
                    >
                      <!-- 闭眼图标（隐藏状态） -->
                      <path
                        v-show="!showPassword"
                        id="eye-closed"
                        data-v-1a96ced4=""
                        fill-rule="evenodd"
                        clip-rule="evenodd"
                        d="M17.5753 6.85456C17.7122 6.71896 17.8939 6.63806 18.0866 6.63806C18.7321 6.63806 19.0436 7.42626 18.5748 7.87006C18.1144 8.30554 17.457 8.69885 16.6478 9.03168L18.1457 10.5296C18.2101 10.5941 18.2613 10.6706 18.2962 10.7548C18.331 10.839 18.349 10.9293 18.349 11.0204C18.349 11.1116 18.331 11.2019 18.2962 11.2861C18.2613 11.3703 18.2101 11.4468 18.1457 11.5113C18.0812 11.5757 18.0047 11.6269 17.9205 11.6618C17.8363 11.6967 17.746 11.7146 17.6548 11.7146C17.5637 11.7146 17.4734 11.6967 17.3892 11.6618C17.305 11.6269 17.2284 11.5757 17.164 11.5113L15.3409 9.68819C15.2898 9.63708 15.247 9.57838 15.2141 9.51428C14.4874 9.71293 13.6876 9.87122 12.8344 9.98119C12.8363 9.99011 12.8381 9.99908 12.8397 10.0081L13.2874 12.5472C13.315 12.7266 13.2713 12.9098 13.1656 13.0573C13.0598 13.2049 12.9005 13.3052 12.7217 13.3367C12.5429 13.3683 12.3589 13.3285 12.2091 13.2259C12.0592 13.1234 11.9555 12.9663 11.9202 12.7882L11.4725 10.2491C11.4645 10.2039 11.4611 10.1581 11.4621 10.1125C10.9858 10.1428 10.4976 10.1586 10.0002 10.1586C9.57059 10.1586 9.14778 10.1468 8.73362 10.1241C8.73477 10.1656 8.7322 10.2074 8.72578 10.249L8.27808 12.7881C8.24612 12.9694 8.14345 13.1306 7.99265 13.2362C7.84186 13.3418 7.65528 13.3831 7.47398 13.3512C7.29268 13.3192 7.1315 13.2166 7.0259 13.0658C6.9203 12.915 6.87892 12.7284 6.91088 12.5471L7.35858 10.008C7.35877 10.007 7.35896 10.0061 7.35915 10.0052C6.50085 9.90284 5.6941 9.75191 4.95838 9.56025C4.93012 9.60634 4.89634 9.64933 4.85748 9.68819L3.03438 11.5113C2.96992 11.5757 2.8934 11.6269 2.80918 11.6618C2.72496 11.6967 2.63469 11.7146 2.54353 11.7146C2.45237 11.7146 2.36211 11.6967 2.27789 11.6618C2.19367 11.6269 2.11714 11.5757 2.05268 11.5113C1.98822 11.4468 1.93709 11.3703 1.90221 11.2861C1.86732 11.2019 1.84937 11.1116 1.84937 11.0204C1.84937 10.9293 1.86732 10.839 1.90221 10.7548C1.93709 10.6706 1.98822 10.5941 2.05268 10.5296L3.49373 9.08855C2.6197 8.744 1.91247 8.33062 1.42559 7.87006C0.956591 7.42636 1.26799 6.63816 1.91359 6.63816C2.10629 6.63816 2.28789 6.71896 2.42489 6.85456C2.70009 7.12696 3.19529 7.45886 3.98459 7.77796C5.54429 8.40856 7.73699 8.77016 10.0001 8.77016C12.2632 8.77016 14.4558 8.40856 16.0156 7.77796C16.8049 7.45886 17.3001 7.12696 17.5753 6.85456Z"
                        fill="#9499A0"
                      ></path>
                      <!-- 睁眼图标（可见状态） -->
                      <path
                        v-show="showPassword"
                        id="eye-open"
                        data-v-1a96ced4=""
                        fill-rule="evenodd"
                        clip-rule="evenodd"
                        d="M2.11069 9.43732C3.21647 7.77542 5.87904 4.58331 9.89458 4.58331C13.8801 4.58331 16.6483 7.72502 17.8345 9.4049C18.0905 9.76747 18.0905 10.2325 17.8345 10.5951C16.6483 12.2749 13.8801 15.4166 9.89458 15.4166C5.87904 15.4166 3.21647 12.2245 2.11069 10.5626C1.88009 10.2161 1.88009 9.7839 2.11069 9.43732ZM9.89458 3.33331C5.19832 3.33331 2.20919 7.03277 1.07001 8.74489C0.560324 9.51091 0.560323 10.4891 1.07001 11.2551C2.20919 12.9672 5.19832 16.6666 9.89458 16.6666C14.5412 16.6666 17.6368 13.0422 18.8556 11.3161C19.4168 10.5213 19.4168 9.4787 18.8556 8.68391C17.6368 6.95774 14.5412 3.33331 9.89458 3.33331ZM7.29165 9.99998C7.29165 8.50421 8.50421 7.29165 9.99998 7.29165C11.4958 7.29165 12.7083 8.50421 12.7083 9.99998C12.7083 11.4958 11.4958 12.7083 9.99998 12.7083C8.50421 12.7083 7.29165 11.4958 7.29165 9.99998ZM9.99998 6.04165C7.81385 6.04165 6.04165 7.81385 6.04165 9.99998C6.04165 12.1861 7.81385 13.9583 9.99998 13.9583C12.1861 13.9583 13.9583 12.1861 13.9583 9.99998C13.9583 7.81385 12.1861 6.04165 9.99998 6.04165Z"
                        fill="#9499A0"
                      ></path>
                    </svg>
                  </div>
                </div>
                <div class="login-form-LoginButton">
                  <button type="submit" class="login-form-LoginButton-button">
                    登录
                  </button>
                </div>
              </form>
              <div>
                <router-link to="/find/password">
                  <span class="login-form-fpw">找回密码</span>
                </router-link>
                <router-link to="/register/account">
                  <span @click="changeRegister" class="login-form-register"
                    >注册账号
                  </span>
                </router-link>
                <router-link to="/feedback">
                  <span class="login-form-feedback">意见反馈</span>
                </router-link>
              </div>
            </div>
            <button class="login-form-line">
              <div class="login-form-line-img">
                <img src="@/assets/photo/刷新.svg" alt="刷新" />
              </div>
              <span class="login-form-line-text">刷新</span>
            </button>
          </div>
        </div>
        <div
          v-show="isWeChatLoginButtonActive && LoginGrade === 2 && choiceActive"
          class="login-WeChat-container"
        >
          <div class="login-WeChat-title-header">
            <span class="login-WeChat-title">微信登录</span>
          </div>
          <div class="login-WeChat-container-img">
            <div class="login-WeChat-container-img">
              <img src="" alt="微信登录二维码" />
            </div>
            <div class="login-WeChat-tips">
              <span>请使用微信扫一扫登录</span>
            </div>
          </div>
          <button class="login-form-line">
            <div class="login-form-line-img">
              <img src="@/assets/photo/刷新.svg" alt="刷新" />
            </div>
            <span class="login-form-line-text">刷新</span>
          </button>
        </div>
        <div
          v-show="isPhoneLoginButtonActive && LoginGrade === 2 && choiceActive"
          class="login-Phone-container"
        >
          <div class="login-Phone-title-header">
            <span class="login-Phone-title">手机登录</span>
          </div>
          <div class="login-Phone-container">
            <div class="login-Phone-container-input">
              <div
                class="login-Phone-container-input-number"
                :class="{ 'is-active': isPhoneInputActive }"
              >
                <!-- 动态绑定类 -->
                <button class="login-Phone-container-input-number-button">
                  <span class="login-Phone-container-input-number-button-text">
                    +86
                  </span>
                  <img src="@/assets/photo/下拉.svg" alt="下拉" />
                </button>
                <input
                  type="text"
                  placeholder="请输入手机号"
                  @focus="setPhoneInputActive"
                  @blur="isPhoneInputActive = false"
                  class="login-Phone-container-input-number-input"
                  pattern="[0-9]*"
                  inputmode="numeric"
                  style="color: rgb(128, 128, 128)"
                  maxlength="11"
                />
              </div>
              <div
                class="login-Phone-container-input-Verification"
                :class="{ 'is-active': isVerificationCodeValidActive }"
              >
                <input
                  type="text"
                  placeholder="请输入验证码"
                  @focus="setVerificationCodeValidActive"
                  @blur="isVerificationCodeValidActive = false"
                  class="login-Phone-container-input-number-input"
                  pattern="[0-9]*"
                  style="color: rgb(128, 128, 128)"
                  maxlength="6"
                  autocomplete="new-password"
                />
                <button class="login-Phone-container-input-Verification-button">
                  验证码
                </button>
              </div>
              <button class="login-Phone-container-input-button">登录</button>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from "vue";
import { useStore } from "vuex";
import { useRouter } from "vue-router";

// 使用 store 和 router
const store = useStore();
const router = useRouter();

// 响应式数据
const Email = ref("");
const Phone = ref("");
const inputTypeValue = ref(""); // 标记输入类型 ("email" 或 "phone")
const Password = ref("");
const showPassword = ref(false); // 控制密码可见性
const isPhoneInputActive = ref(false); // 添加手机输入框状态
const isPhoneNumberValid = ref(false); // 手机号码是否有效
const isVerificationCodeValid = ref(false); // 验证码是否有效
const isVerificationCodeValidActive = ref(false); // 验证码输入框状态
const isAllowButtonActive = ref(false); // 控制同意按钮状态

// 计算属性
const isLoggedIn = computed(() => store.state.isLoggedIn);
const inputType = computed(() => (showPassword.value ? "text" : "password"));
const isinitLoginActive = computed(() => store.state.auth.isinitLoginActive);
const isAcountLoginButtonActive = computed(
  () => store.state.auth.isAcountLoginButtonActive
);
const isWeChatLoginButtonActive = computed(
  () => store.state.auth.isWeChatLoginButtonActive
);
const isPhoneLoginButtonActive = computed(
  () => store.state.auth.isPhoneLoginButtonActive
);
const showRegister = computed(() => store.state.auth.showRegister);
const LoginGrade = computed(() => store.state.auth.LoginGrade);
const choiceActive = computed(() => store.state.auth.choiceActive);

isPhoneNumberValid;
isVerificationCodeValid;
showRegister;
// 方法
const handleInput = (event: Event) => {
  const value = (event.target as HTMLInputElement).value;
  //event.target 指向触发事件的元素（通常是输入框）
  //而 event.target.value 则是该元素当前的值

  // 判断输入的是邮箱还是电话
  if (isEmail(value)) {
    Email.value = value;
    Phone.value = "";
    inputTypeValue.value = "email";
  } else if (isPhone(value)) {
    Phone.value = "+86" + value.replace(/^\+86/, ""); // 确保前缀是 +86
    Email.value = "";
    inputTypeValue.value = "phone";
  } else {
    // 如果既不是邮箱也不是电话，重置值
    Email.value = "";
    Phone.value = "";
    inputTypeValue.value = "";
  }
};

const isEmail = (value: string) => {
  // 邮箱验证正则表达式
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  return emailRegex.test(value);
};

const isPhone = (value: string) => {
  // 支持中国手机号格式（11位数字）和国际格式
  const phoneRegex = /^(\+?86)?1[3-9]\d{9}$/;
  return phoneRegex.test(value);
};

const handleLogin = () => {
  if (inputTypeValue.value === "email" && Email.value) {
    // 登录逻辑
    store
      .dispatch("auth/login", {
        email: Email.value,
        password: Password.value,
      })
      .then((response) => {
        if (response.data.success && response.status === 200) {
          console.log("登录成功");
          console.log("Response data:", response.data);
          // 登录
          store.commit("auth/login");
          // 登录成功，可以跳转到主页
          router.push("/");
        }
      })
      .catch((error) => {
        if (error.response.status === 401) {
          // 服务器返回了错误状态码
          console.error("Error status:", error.response.status);
          console.error("Error data:", error.response.data);
          alert("登录失败: " + error.response.data.error);
        } else if (error.request) {
          // 请求已发出但没有收到响应
          console.error("No response received:", error.request);
          alert("无法连接到服务器，请检查网络连接");
        } else {
          // 其他错误
          console.error("Error message:", error.message);
          alert("登录请求出错: " + error.message);
        }
      });
  } else if (inputTypeValue.value === "phone" && Phone.value) {
    store
      .dispatch("auth/login", {
        phone: Phone.value,
        password: Password.value,
      })
      .then((response) => {
        if (response.data.success && response.status === 200) {
          console.log("登录成功");
          console.log("Response data:", response.data);
          // 登录
          store.commit("auth/login");
          // 登录成功，可以跳转到主页
          router.push("/");
        }
      })
      .catch((error) => {
        if (error.response.status === 401) {
          // 服务器返回了错误状态码
          console.error("Error status:", error.response.status);
          console.error("Error data:", error.response.data);
          alert("登录失败: " + error.response.data.error);
        } else if (error.request) {
          // 请求已发出但没有收到响应
          console.error("No response received:", error.request);
          alert("无法连接到服务器，请检查网络连接");
        } else {
          // 其他错误
          console.error("Error message:", error.message);
          alert("登录请求出错: " + error.message);
        }
      });
  }
};

const togglePasswordVisibility = () => {
  showPassword.value = !showPassword.value;
};

const back = () => {
  // 实现返回功能
  if (store.state.auth.LoginGrade === 2) {
    // 返回一级登录界面
    store.commit("auth/upDataLoginButtonActive", {
      isinitLoginActive: true,
      isAcountLoginButtonActive: false,
      isWeChatLoginButtonActive: false,
      isPhoneLoginButtonActive: false,
    });
    store.commit("auth/setLoginGradeActive", 1);
  }
};

const handleCloseLoginClick = () => {
  store.commit("auth/closeLogin");
  store.commit("auth/upDataLoginButtonActive", {
    isLoginButtonActive: false,
    isinitLoginActive: false,
    isAcountLoginButtonActive: false,
    isWeChatLoginButtonActive: false,
    isPhoneLoginButtonActive: false,
    choiceActive: false,
  });
  // 清空登录等级
  store.commit("auth/clearLoginGrade");
};

const setAcountLoginClick = () => {
  if (choiceActive.value == true) {
    store.commit("auth/upDataLoginButtonActive", {
      isinitLoginActive: false,
    });
    // 设置登录等级为2
    store.commit("auth/setLoginGradeActive", 2);
  } else {
    isAllowButtonActive.value = true; // 激活同意按钮状态
  }
  store.commit("auth/upDataLoginButtonActive", {
    isAcountLoginButtonActive: true,
  });
};

const setWeChatLoginClick = () => {
  if (choiceActive.value == true) {
    store.commit("auth/upDataLoginButtonActive", {
      isinitLoginActive: false,
    });
    // 设置登录等级为2
    store.commit("auth/setLoginGradeActive", 2);
  } else {
    isAllowButtonActive.value = true; // 激活同意按钮状态
  }
  store.commit("auth/upDataLoginButtonActive", {
    isWeChatLoginButtonActive: true,
  });
};

const setPhoneLoginClick = () => {
  if (choiceActive.value == true) {
    store.commit("auth/upDataLoginButtonActive", {
      isinitLoginActive: false,
    });
    // 设置登录等级为2
    store.commit("auth/setLoginGradeActive", 2);
  } else {
    isAllowButtonActive.value = true; // 激活同意按钮状态
  }
  store.commit("auth/upDataLoginButtonActive", {
    isPhoneLoginButtonActive: true,
  });
};

const setPhoneInputActive = () => {
  isPhoneInputActive.value = true; // 设置手机输入框为活动状态
};

const setVerificationCodeValidActive = () => {
  isVerificationCodeValidActive.value = true; // 设置手机输入框为活动状态
};

const setChoiceActive = () => {
  if (!choiceActive.value) {
    store.commit("auth/upDataLoginButtonActive", {
      choiceActive: true,
    });
  } else {
    store.commit("auth/upDataLoginButtonActive", {
      choiceActive: false,
    });
  }
};

const allow = () => {
  setChoiceActive();
  isAllowButtonActive.value = false;
  if (isAcountLoginButtonActive.value == true) {
    store.commit("auth/upDataLoginButtonActive", {
      isinitLoginActive: false,
    });
    // 设置登录等级为2
    store.commit("auth/setLoginGradeActive", 2);
  } else if (isWeChatLoginButtonActive.value == true) {
    store.commit("auth/upDataLoginButtonActive", {
      isinitLoginActive: false,
    });
    // 添加微信登录等级为2
    store.commit("auth/setLoginGradeActive", 2);
  } else if (isPhoneLoginButtonActive.value == true) {
    store.commit("auth/upDataLoginButtonActive", {
      isinitLoginActive: false,
    });
    // 添加手机登录等级为2
    store.commit("auth/setLoginGradeActive", 2);
  }
};

const cancel = () => {
  if (isAcountLoginButtonActive.value == true) {
    store.commit("auth/upDataLoginButtonActive", {
      isAcountLoginButtonActive: false,
    });
  } else if (isWeChatLoginButtonActive.value == true) {
    store.commit("auth/upDataLoginButtonActive", {
      isWeChatLoginButtonActive: false,
    });
  } else if (isPhoneLoginButtonActive.value == true) {
    store.commit("auth/upDataLoginButtonActive", {
      isPhoneLoginButtonActive: false,
    });
  }
  isAllowButtonActive.value = false;
};

const changeRegister = () => {
  if (choiceActive.value == true) {
    store.commit("auth/closeLogin");
    store.commit("auth/upDataLoginButtonActive", {
      isAcountLoginButtonActive: false,
    });
    // 设置登录等级为3
    store.commit("auth/setLoginGradeActive", 3);
  } else {
    isAllowButtonActive.value = true; // 激活同意按钮状态
  }
  store.commit("auth/upDataLoginButtonActive", {
    showRegister: true,
  });
};
</script>

<style lang="css">
@import "@/assets/styles/Login.css";
</style>
