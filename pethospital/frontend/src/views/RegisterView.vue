<template>
  <div v-show="showRegister" class="Register-mask">
    <div class="RegisterView">
      <button @click="back" class="floating-action back-button">
        <img src="@/assets/photo/上一步.svg" alt="上一步" />
      </button>
      <button @click="closeRegister" class="floating-action close-button">
        <img src="@/assets/photo/叉.svg" alt="关闭" />
      </button>

      <div class="Register-hero">
        <div class="Register-hero-badge">Pet Manager</div>
        <h1 class="Register-hero-title">创建你的宠物健康档案</h1>
        <p class="Register-hero-text">
          通过邮箱快速注册，完成后即可进入系统查看预约、诊疗与订单信息。
        </p>
        <div class="Register-hero-points">
          <div class="Register-hero-point">
            <span class="Register-hero-point-index">01</span>
            <span>填写邮箱并获取验证码</span>
          </div>
          <div class="Register-hero-point">
            <span class="Register-hero-point-index">02</span>
            <span>设置安全密码并确认</span>
          </div>
          <div class="Register-hero-point">
            <span class="Register-hero-point-index">03</span>
            <span>注册成功后自动登录</span>
          </div>
        </div>
      </div>

      <div class="Register-form-container">
        <div class="Register-form-header">
          <span class="Register-form-title">账号注册</span>
          <span class="Register-form-subtitle">请使用常用邮箱完成验证</span>
        </div>
        <form @submit.prevent="Verify" class="Register-form">
          <div class="Register-form-div">
            <div class="Register-form-minContainser">
              <div class="Register-form-minContainser-first">
                <div class="Register-form-minContainser-first-left-icon">
                  <span class="rq">*</span>
                  <label for="Email">邮箱地址</label>
                </div>
                <div class="Register-input-shell">
                  <input
                    id="Email"
                    v-model="Email"
                    placeholder="请输入常用邮箱"
                    type="text"
                    class="Register-form-minContainser-first-input"
                    autocomplete="off"
                    @input="CheckEmail"
                  />
                  <div
                    v-show="isEmailValid && EmailEffect"
                    class="Register-form-minContainser-first-right"
                  >
                    <img
                      src="@/assets/photo/勾.png"
                      alt="正确"
                      class="Register-form-minContainser-first-right-img"
                    />
                  </div>
                </div>
              </div>
              <div
                v-show="isCheckEmail && (!isEmailValid || !EmailEffect)"
                class="Register-form-minContainser-second"
              >
                <span
                  v-if="!isEmailValid"
                  class="Register-form-minContainser-second-span"
                  >请输入正确的邮箱地址</span
                >
                <span
                  v-else-if="!EmailEffect"
                  class="Register-form-minContainser-second-span"
                  >此邮箱已经被注册，请更换后重试</span
                >
              </div>
            </div>

            <div class="Register-form-minContainser">
              <div class="Register-form-minContainser-first">
                <div class="Register-form-minContainser-first-left-icon">
                  <span class="rq">*</span>
                  <label for="Password1">登录密码</label>
                </div>
                <div class="Register-input-shell Register-input-shell-password">
                  <input
                    id="Password1"
                    :type="inputType"
                    v-model="Password1"
                    placeholder="8-16 位，包含两种以上字符类型"
                    class="Register-form-minContainser-first-input"
                    @input="CheckPassword"
                    @focus="CheckSpaceIn()"
                    @blur="CheckSpaceOut()"
                  />
                  <div
                    v-show="isPasswordValid"
                    class="Register-form-minContainser-first-right"
                  >
                    <img
                      src="@/assets/photo/勾.png"
                      alt="正确"
                      class="Register-form-minContainser-first-right-img"
                    />
                  </div>
                </div>
              </div>
              <div
                v-show="PasswordPrompt"
                class="Register-form-minContainser-second"
              >
                <ul class="Register-form-minContainser-second-ul">
                  <li
                    v-show="!PasswordPromptNoSpace && !PasswordActive"
                    class="Register-form-minContainser-second-ul-li"
                    :class="{ nospace: nospace }"
                  >
                    不能包含空格
                  </li>
                  <li
                    v-show="!PasswordPromptspecial && !PasswordActive"
                    class="Register-form-minContainser-second-ul-li"
                    :class="{ specialChar: specialChar }"
                  >
                    必须包含字母、数字、符号中至少 2 种
                  </li>
                  <li
                    v-show="!PasswordPromptLength && !PasswordActive"
                    class="Register-form-minContainser-second-ul-li"
                    :class="{ passwordLength: passwordLength }"
                  >
                    长度为 8-16 个字符
                  </li>
                </ul>
                <span
                  v-show="PasswordActive"
                  class="Register-form-minContainser-second-span"
                >
                  密码不能为空
                </span>
              </div>
            </div>

            <div class="Register-form-minContainser">
              <div class="Register-form-minContainser-first">
                <div class="Register-form-minContainser-first-left-icon">
                  <span class="rq">*</span>
                  <label for="Password2">确认密码</label>
                </div>
                <div class="Register-input-shell Register-input-shell-password">
                  <input
                    id="Password2"
                    :type="inputType"
                    v-model="Password2"
                    placeholder="请再次输入密码"
                    class="Register-form-minContainser-first-input"
                    @input="checkPasswordConfirm"
                    @blur="checkPasswordConfirm"
                  />
                  <div class="Register-form-Password-eye">
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
                  <div
                    v-show="isPasswordConfirmValid"
                    class="Register-form-minContainser-first-right-password2"
                  >
                    <img
                      src="@/assets/photo/勾.png"
                      alt="正确"
                      class="Register-form-minContainser-first-right-password2-img"
                    />
                  </div>
                </div>
              </div>
              <div
                v-show="!isPasswordConfirmValid && Password1"
                class="Register-form-minContainser-second"
              >
                <span
                  v-show="Password2 && !isPasswordConfirmValid"
                  class="Register-form-minContainser-second-span"
                  >两次输入的密码不一致</span
                >
              </div>
            </div>

            <div class="Register-form-minContainser">
              <div class="Register-form-minContainser-first">
                <div class="Register-form-minContainser-first-left-icon">
                  <span class="rq">*</span>
                  <label for="VerificationCode">邮箱验证码</label>
                </div>
                <div class="Register-code-row">
                  <div class="Register-input-shell Register-code-input-shell">
                    <input
                      id="VerificationCode"
                      v-model="VerificationCode"
                      placeholder="请输入邮箱中的验证码"
                      type="text"
                      class="Register-form-minContainser-first-input"
                      autocomplete="off"
                    />
                  </div>
                  <div
                    class="Register-form-minContainser-first-right-button-wrap"
                  >
                    <button
                      type="button"
                      class="Register-form-minContainser-first-right-button"
                      :class="{ after: isgetVerificationCode }"
                      :disabled="!canRequestVerification"
                      @click="getVerificationCode"
                    >
                      {{
                        isgetVerificationCode
                          ? `${count}秒后重新获取`
                          : "获取验证码"
                      }}
                    </button>
                  </div>
                </div>
              </div>
            </div>

            <div class="Register-form-submit">
              <button
                type="submit"
                class="Register-form-submit-button"
                :disabled="!canSubmit"
              >
                立即注册
              </button>
              <span class="Register-form-submit-tip"
                >提交后会自动登录并进入对应首页</span
              >
            </div>
          </div>
        </form>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onUnmounted } from "vue";
import { useStore } from "vuex";
import { useRouter, useRoute } from "vue-router";
import { getHomeRouteByUserType } from "@/core/auth/utils/authRedirect";
import { isEmail } from "@/core/auth/utils/authValidators";
import { defineComponent } from "vue";

defineComponent({
  name: "userRegisterAccount",
});

const store = useStore();
const route = useRoute();
const router = useRouter();

const UserName = ref("");
const Password1 = ref("");
const Password2 = ref("");
const Email = ref("");
const VerificationCode = ref("");
const count = ref(0); // 倒计时的计时器
const passwordCheckTimeout = ref<number | null>(null); // 用于保存密码检查的定时器
const showPassword = ref(false); // 控制密码可见性
const isgetVerificationCode = ref(false); // 添加验证码按钮状态-灰色(禁用)
const showVerificationGreen = ref(false); // 添加验证码按钮状态-绿色
const showVerificationRed = ref(false); // 添加验证码按钮状态-红色
const isEmailValid = ref(false); // 邮箱是否有效
const isPasswordValid = ref(false); // 密码是否有效
const isPasswordConfirmValid = ref(false); // 确认密码是否有效
const PasswordActive = ref(false); // 密码输入框状态
const PasswordPrompt = ref(false);
const PasswordPromptNoSpace = ref(false); // 密码提示框状态
const PasswordPromptspecial = ref(false); // 密码提示框状态
const PasswordPromptLength = ref(false); // 密码提示框状态
const nospace = ref(false); // 不能输入空格
const specialChar = ref(false); // 特殊字符
const passwordLength = ref(false); // 密码长度
const number = ref(0);

const isCheckEmail = ref(false);
const EmailEffect = ref(true);

let requestInProgress = false;
let registerInProgress = false;
let checkEmailInProgress = false;

// 添加密码强度检查
/*const strongPassword =
  /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[@$!%*?&])[A-Za-z\d@$!%*?&]{8,}$/;*/ // 强密码：至少8个字符，包含大小写字母、数字和特殊字符
const passwordRegex =
  /^(?=.*[\S])[a-zA-Z\d!@#$%^&*()_+\-=[\]{};':"\\|,.<>?]{8,16}$/; // 简单密码验证，至少8个字符，不能有空格

const showRegister = computed(() => store.state.ui.showRegister);
const inputType = computed(() => (showPassword.value ? "text" : "password"));
const canRequestVerification = computed(
  () =>
    !isgetVerificationCode.value &&
    isEmailValid.value &&
    EmailEffect.value &&
    !requestInProgress
);
const canSubmit = computed(
  () =>
    isEmailValid.value &&
    EmailEffect.value &&
    isPasswordValid.value &&
    isPasswordConfirmValid.value &&
    !!VerificationCode.value &&
    !registerInProgress
);

// 当注册窗口关闭时，重置所有表单数据
watch(showRegister, (newVal) => {
  if (!newVal) {
    resetForm();
  }
});

// 重置表单数据的方法
function resetForm() {
  UserName.value = "";
  Password1.value = "";
  Password2.value = "";
  Email.value = "";
  VerificationCode.value = "";
  count.value = 0;
  if (passwordCheckTimeout.value) {
    clearTimeout(passwordCheckTimeout.value);
  }
  showPassword.value = false;
  isgetVerificationCode.value = false;
  showVerificationGreen.value = false;
  showVerificationRed.value = false;
  isEmailValid.value = false;
  isPasswordValid.value = false;
  isPasswordConfirmValid.value = false;
  PasswordActive.value = false;
  nospace.value = false;
  specialChar.value = false;
  passwordLength.value = false;
  PasswordPrompt.value = false;
  PasswordPromptNoSpace.value = false;
  PasswordPromptspecial.value = false;
  PasswordPromptLength.value = false;
  isCheckEmail.value = false;
  EmailEffect.value = true;
  number.value = 0;
}

function back() {
  // 实现返回功能
  if (route.name === "userRegisterAccount") {
    router.back();
    // 返回上一级登录界面
    store.commit("ui/upDataLoginButtonActive", {
      isinitLoginActive: false,
      isAcountLoginButtonActive: true,
      isWeChatLoginButtonActive: false,
      isPhoneLoginButtonActive: false,
      showRegister: false,
      choiceActive: true,
    });
    // 返回登录等级
    store.commit("ui/setLoginGradeActive", 2);
  }
}

function handleCloseLoginClick() {
  store.commit("ui/closeLogin");
  store.commit("ui/upDataLoginButtonActive", {
    isLoginButtonActive: false,
    isinitLoginActive: false,
    isAcountLoginButtonActive: false,
    isWeChatLoginButtonActive: false,
    isPhoneLoginButtonActive: false,
  });
  // 清空登录等级
  store.commit("ui/clearLoginGrade");
}

function closeRegister() {
  store.commit("ui/upDataLoginButtonActive", {
    showRegister: false,
  });
  handleCloseLoginClick();
}

function togglePasswordVisibility() {
  showPassword.value = !showPassword.value;
}

function CheckSpaceIn() {
  PasswordPrompt.value = true;
  PasswordPromptNoSpace.value = false;
  PasswordPromptspecial.value = false;
  PasswordPromptLength.value = false;
  PasswordActive.value = false;
}

function CheckSpaceOut() {
  if (Password1.value) {
    PasswordActive.value = false;
    if (nospace.value === true) {
      PasswordPromptNoSpace.value = true;
    } else {
      PasswordPromptNoSpace.value = false;
    }
    if (specialChar.value === true) {
      PasswordPromptspecial.value = true;
    } else {
      PasswordPromptspecial.value = false;
    }
    if (passwordLength.value === true) {
      PasswordPromptLength.value = true;
    } else {
      PasswordPromptLength.value = false;
    }
  } else {
    PasswordActive.value = true;
  }
}

function CheckEmail() {
  // 使用正确的ID选择器，并使用正则表达式的test方法
  if (passwordCheckTimeout.value) {
    clearTimeout(passwordCheckTimeout.value);
  }
  passwordCheckTimeout.value = setTimeout(() => {
    if (number.value <= 50) {
      number.value += 1;

      // 首先进行邮箱格式验证
      if (!isEmail(Email.value)) {
        isEmailValid.value = false;
        return;
      }

      isEmailValid.value = true;

      // 检查邮箱是否可用
      if (checkEmailInProgress) {
        return;
      }

      checkEmailInProgress = true;
      store
        .dispatch("auth/checkEmail", { email: Email.value })
        .then((response) => {
          if (response.status === 200) {
            // 其他状态码表示邮箱未被注册，可以使用
            EmailEffect.value = true;
          } else if (response.status === 400) {
            // 邮箱已被注册，不能使用
            EmailEffect.value = false;
          }
        })
        .catch((error) => {
          if (error.response.status === 400) {
            // 后端返回400错误，表示邮箱已被注册
            EmailEffect.value = false;
          }
        })
        .finally(() => {
          checkEmailInProgress = false;
          isCheckEmail.value = true;
        });
    }
  }, 1000); // 延迟请求频率到1秒
}

// 检查第一遍密码
function CheckPassword() {
  if (passwordCheckTimeout.value) {
    clearTimeout(passwordCheckTimeout.value);
  }
  passwordCheckTimeout.value = setTimeout(() => {
    if (Password1.value) {
      isPasswordValid.value = passwordRegex.test(Password1.value);
      // 当第一个密码改变时，也检查确认密码
      checkPasswordConfirm();
    } else if (!Password1.value) {
      isPasswordValid.value = false;
      isPasswordConfirmValid.value = false;
    }
    noSpace();
    Char();
    Length();
  }, 500); // 延迟500毫秒
}

// 检查确认密码
function checkPasswordConfirm() {
  // 只有当两个密码都不为空时才检查是否相同
  if (Password1.value && Password2.value) {
    isPasswordConfirmValid.value = Password1.value === Password2.value;
  } else {
    isPasswordConfirmValid.value = false;
  }
}

/*function CheckUsername() {
  // 使用正确的ID选择器，并使用正则表达式的test方法
  coonst userNameElement = document.getElementById("Username");
  if(userNameElement)
  {
    const usernameInput = userNameElement.value;
    isUsernameValid.value = usernameRegex.test(usernameInput);
  }
}*/
// 检查密码中是否有空格
function noSpace() {
  if (Password1.value) {
    if (/^\S*$/.test(Password1.value)) {
      nospace.value = true;
    } else {
      nospace.value = false;
    }
  } else {
    nospace.value = false;
  }
}
// 检查密码中是否有特殊字符
function Char() {
  if (Password1.value) {
    // 检查是否包含字母
    const hasLetter = /[a-zA-Z]/.test(Password1.value);
    // 检查是否包含数字
    const hasDigit = /\d/.test(Password1.value);
    // 检查是否包含特殊符号
    const hasSpecialChar = /[!@#$%^&*()_+\-=[\]{};':"\\|,.<>/?]/.test(
      Password1.value
    );

    // 计算满足条件的类型数量
    const typeCount =
      (hasLetter ? 1 : 0) + (hasDigit ? 1 : 0) + (hasSpecialChar ? 1 : 0);

    specialChar.value = typeCount >= 2;
  } else {
    specialChar.value = false;
  }
  console.log(specialChar.value);
}
// 检查密码长度
function Length() {
  if (/^.{8,}$/.test(Password1.value)) {
    passwordLength.value = true;
  } else {
    passwordLength.value = false;
  }
}

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
      .dispatch("auth/sendVerificationCodeRegister", {
        email: Email.value,
        scene: "register",
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
            console.log(error.response.status);
            alert("服务器错误: " + errorMessage);
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

function Verify() {
  if (registerInProgress) {
    alert("请等待注册完成");
    return;
  }

  // 验证必要参数
  if (!Email.value || !VerificationCode.value || !Password1.value) {
    alert("请填写完整的注册信息");
    return;
  }

  // 设置状态锁，防止重复提交
  registerInProgress = true;

  // 在注册组件中
  store
    .dispatch("auth/verify", {
      email: Email.value,
      verificationCode: VerificationCode.value,
      scene: "register",
    })
    .then((registerResponse) => {
      if (registerResponse.status === 200) {
        // 注册成功，进行保存用户数据
        // return store.dispatch("auth/registerSetUser",
        // 会返回一个Promise对象重新链接到主链上
        return store.dispatch("auth/registerSetUser", {
          email: Email.value,
          password: Password1.value,
        });
      } else {
        // 如果注册失败，抛出错误以进入catch块
        throw new Error(`注册失败: ${registerResponse.status}`);
      }
    })
    // 这个链式调用是通过return store.dispatch("auth/registerSetUser")返回的Promise对象，
    // 允许我们进行后续处理，比如显示注册成功提示。
    .then((registerSetUserResponse) => {
      if (registerSetUserResponse.status === 200) {
        alert("注册成功");
        // 登录操作
        return store.dispatch("auth/login", {
          identifier: Email.value,
          password: Password1.value,
        });
      } else {
        throw new Error(`保存用户数据失败: ${registerSetUserResponse.status}`);
      }
    })
    .then((loginResponse) => {
      const loginData = loginResponse.data?.data ?? loginResponse.data;
      const user = loginData?.user;

      if (loginResponse.status === 200) {
        if (!user) {
          throw new Error("注册后登录返回数据缺少 user 字段");
        }

        router.push(getHomeRouteByUserType(user.type_id, user.type_name));
      } else {
        throw new Error(`登录失败: ${loginResponse.status}`);
      }
    })
    .catch((error) => {
      if (error.response && error.response.status === 401) {
        alert("注册失败（检查邮箱地址和验证码是否错误）");
      } else if (error.response && error.response.status === 400) {
        alert("请输入邮箱或者验证码");
      } else {
        // 网络错误或其他客户端错误
        alert("网络错误或请求失败，请稍后重试");
      }
    })
    .finally(() => {
      registerInProgress = false; // 释放锁
      EmailEffect.value = false; // 重置邮箱可用标志
    });
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

// 清理定时器
onUnmounted(() => {
  if (passwordCheckTimeout.value) {
    clearTimeout(passwordCheckTimeout.value);
  }
});
</script>

<style scoped>
button {
  border: none;
}

.Register-mask {
  position: fixed;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  padding: 28px;
  top: 0;
  left: 0;
  z-index: 1000;
  box-sizing: border-box;
  background: linear-gradient(155deg, #cbd9f0 0%, #dde7f3 52%, #cfe0e2 100%);
  font-family: "PingFang SC", "Noto Sans SC", "Segoe UI", system-ui, sans-serif;
}

.RegisterView {
  position: relative;
  display: grid;
  grid-template-columns: 0.9fr 1.1fr;
  width: min(960px, 100%);
  min-height: 620px;
  border-radius: 18px;
  overflow: hidden;
  background: #ffffff;
  border: 1px solid #d4ddd9;
  box-shadow: 0 22px 48px rgba(28, 50, 80, 0.16);
}

.floating-action {
  position: absolute;
  top: 16px;
  z-index: 2;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 38px;
  height: 38px;
  border-radius: 8px;
  border: 1px solid #dfe7df;
  background: #ffffff;
  cursor: pointer;
  transition: background-color 0.15s ease, border-color 0.15s ease;
}

.floating-action:hover {
  background: #f4f7f4;
  border-color: #cdd8cd;
}

.back-button {
  left: 18px;
}

.close-button {
  right: 18px;
}

.Register-hero {
  position: relative;
  display: flex;
  flex-direction: column;
  justify-content: center;
  padding: 56px 40px;
  color: #1d3429;
  background: linear-gradient(180deg, #f3f7ff 0%, #eef6f3 100%);
  border-right: 1px solid #e7eef0;
}

.Register-hero-badge {
  display: inline-flex;
  align-items: center;
  width: fit-content;
  padding: 6px 12px;
  border-radius: 999px;
  background: #e7eeff;
  color: #1e5eff;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.Register-hero-title {
  margin: 20px 0 12px;
  font-size: 28px;
  line-height: 1.25;
  font-weight: 700;
  color: #103b73;
}

.Register-hero-text {
  margin: 0;
  max-width: 320px;
  color: #6d7b72;
  font-size: 14px;
  line-height: 1.7;
}

.Register-hero-points {
  display: grid;
  gap: 10px;
  margin-top: 32px;
}

.Register-hero-point {
  display: flex;
  gap: 12px;
  align-items: center;
  padding: 12px 14px;
  border-radius: 10px;
  border: 1px solid #e1e9ec;
  background: #ffffff;
  color: #1d3429;
  font-size: 13px;
}

.Register-hero-point-index {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 30px;
  height: 30px;
  border-radius: 8px;
  background: #e7eeff;
  color: #1e5eff;
  font-size: 13px;
  font-weight: 700;
}

.Register-form-container {
  display: flex;
  flex-direction: column;
  justify-content: center;
  padding: 48px 44px;
  background: #ffffff;
}

.Register-form-header {
  margin-bottom: 24px;
}

.Register-form-title {
  display: block;
  color: #18373a;
  font-size: 22px;
  font-weight: 700;
}

.Register-form-subtitle {
  display: inline-block;
  margin-top: 6px;
  color: #6d7b72;
  font-size: 13px;
}

.Register-form {
  flex: 1;
}

.Register-form-div {
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.Register-form-minContainser {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.Register-form-minContainser-first {
  display: grid;
  grid-template-columns: 72px minmax(0, 1fr);
  gap: 12px;
  align-items: center;
}

.Register-form-minContainser-first-left-icon {
  display: flex;
  align-items: center;
  gap: 4px;
  color: #6d7b72;
  font-size: 12px;
  font-weight: 600;
}

.rq {
  color: #b04455;
}

.Register-input-shell {
  position: relative;
  display: flex;
  align-items: center;
  min-height: 44px;
  padding: 0 12px;
  border: 1px solid #dfe7df;
  border-radius: 8px;
  background: #ffffff;
  transition: border-color 0.15s ease, box-shadow 0.15s ease;
}

.Register-input-shell:focus-within {
  border-color: #1e5eff;
  box-shadow: 0 0 0 3px rgba(30, 94, 255, 0.12);
}

.Register-input-shell-password {
  padding-right: 72px;
}

.Register-form-minContainser-first-input {
  width: 100%;
  border: none;
  outline: none;
  background: transparent;
  color: #1d3429;
  font-size: 13px;
  font-family: "PingFang SC", "Noto Sans SC", "Segoe UI", system-ui, sans-serif;
}

.Register-form-minContainser-first-input::placeholder {
  color: #93a1b5;
}

.Register-form-minContainser-first-right,
.Register-form-minContainser-first-right-password2 {
  position: absolute;
  right: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.Register-form-minContainser-first-right-img,
.Register-form-minContainser-first-right-password2-img {
  width: 18px;
  height: 18px;
}

.Register-form-Password-eye {
  position: absolute;
  right: 40px;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 22px;
  height: 22px;
  cursor: pointer;
}

.Register-form-minContainser-second {
  margin-left: 84px;
  color: #b04455;
  font-size: 12px;
}

.Register-form-minContainser-second-span {
  display: inline-block;
  line-height: 1.6;
}

.Register-form-minContainser-second-ul {
  display: grid;
  gap: 6px;
  margin: 0;
  padding: 0;
  list-style: none;
}

.Register-form-minContainser-second-ul-li {
  display: inline-flex;
  align-items: center;
  width: fit-content;
  padding: 5px 10px;
  border-radius: 8px;
  background: #fdeef0;
  color: #b04455;
  font-size: 12px;
  line-height: 1.4;
}

.Register-form-minContainser-second-ul-li.nospace,
.Register-form-minContainser-second-ul-li.specialChar,
.Register-form-minContainser-second-ul-li.passwordLength {
  background: #e7f5ef;
  color: #247b62;
}

.Register-code-row {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 120px;
  gap: 10px;
  align-items: center;
}

.Register-code-input-shell {
  min-width: 0;
}

.Register-form-minContainser-first-right-button-wrap {
  width: 120px;
}

.Register-form-minContainser-first-right-button {
  width: 100%;
  min-height: 44px;
  padding: 0 12px;
  border: 1px solid #dfe7df;
  border-radius: 8px;
  background: #f4f7f4;
  color: #1e5eff;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  transition: background 0.15s ease, opacity 0.15s ease;
}

.Register-form-minContainser-first-right-button.after {
  font-size: 12px;
  color: #6d7b72;
}

.Register-form-minContainser-first-right-button:hover:not(:disabled) {
  background: #eef2ee;
}

.Register-form-minContainser-first-right-button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.Register-form-submit {
  display: flex;
  flex-direction: column;
  align-items: stretch;
  margin-top: 6px;
  margin-left: 84px;
  gap: 8px;
}

.Register-form-submit-button {
  min-height: 44px;
  border: 0;
  border-radius: 8px;
  background: #1e5eff;
  color: #ffffff;
  font-size: 15px;
  font-weight: 700;
  cursor: pointer;
  transition: background 0.15s ease, box-shadow 0.15s ease, opacity 0.15s ease;
}

.Register-form-submit-button:hover:not(:disabled) {
  background: #1850e0;
  box-shadow: 0 10px 20px rgba(30, 94, 255, 0.22);
}

.Register-form-submit-button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
  box-shadow: none;
}

.Register-form-submit-tip {
  color: #93a1b5;
  font-size: 12px;
}

@media (max-width: 960px) {
  .RegisterView {
    grid-template-columns: 1fr;
    width: min(680px, 100%);
    min-height: auto;
  }

  .Register-hero {
    padding: 86px 28px 28px;
  }

  .Register-form-container {
    padding: 30px 28px 32px;
  }
}

@media (max-width: 640px) {
  .Register-mask {
    padding: 12px;
  }

  .RegisterView {
    border-radius: 24px;
  }

  .Register-hero {
    padding: 80px 22px 24px;
  }

  .Register-hero-title {
    font-size: 30px;
  }

  .Register-form-container {
    padding: 24px 18px 20px;
  }

  .Register-form-minContainser-first {
    grid-template-columns: 1fr;
    gap: 10px;
  }

  .Register-form-minContainser-second,
  .Register-form-submit {
    margin-left: 0;
  }

  .Register-code-row {
    grid-template-columns: 1fr;
  }

  .Register-form-minContainser-first-right-button-wrap {
    width: 100%;
  }
}
</style>
