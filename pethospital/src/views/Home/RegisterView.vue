<template>
  <div v-show="showRegister && LoginGrade === 3" class="Register-mask">
    <div class="RegisterView">
      <div class="login-back">
        <button @click="back" class="login-back-button">
          <img src="@/assets/photo/上一步.svg" alt="上一步" />
        </button>
      </div>
      <div class="login-cha">
        <button @click="closeRegister" class="login-cha-button">
          <img src="@/assets/photo/叉.svg" alt="叉" />
        </button>
      </div>
      <span class="Register-form-title">注册</span>
      <div class="Register-form-container">
        <form @submit.prevent="" class="Register-form">
          <div class="Register-form-left">
            <div class="Register-form-Email">
              <div class="Register-form-Email-icon">
                <span class="rq">*</span>
                <label for="Email">邮箱：</label>
              </div>
              <input
                id="Email"
                v-model="Email"
                placeholder="Email"
                type="text"
                class="Register-form-Email-input"
                autocomplete="off"
                @input="CheckEmail"
                @focus="CheckSpaceIn('Email')"
                @blur="CheckSpaceOut('Email')"
              />
              <div class="Register-form-Email-right-img">
                <img
                  v-show="isEmailValid"
                  src="@/assets/photo/勾.png"
                  alt="正确"
                  class="Register-form-right-img"
                />
              </div>
            </div>
            <div v-show="!isEmailValid" class="Register-form-Email-right">
              <span v-show="Email" class="Register-form-Email-right-span"
                >请输入正确的邮箱
              </span>
              <span v-show="EmailActive" class="Register-form-Email-right-span">
                邮箱不能为空
              </span>
              <img
                v-show="isEmailValid"
                src="@/assets/photo/勾.png"
                alt="正确"
                class="Register-form-right-img"
              />
            </div>
            <div class="Register-form-Password">
              <div class="Register-form-Password1-icon">
                <span class="rq">*</span>
                <label for="Password1">密码：</label>
              </div>
              <input
                id="Password1"
                :type="inputType"
                v-model="Password1"
                placeholder="Password"
                class="Register-form-Password1-input"
                @input="CheckPassword"
                @focus="CheckSpaceIn('Password1')"
                @blur="CheckSpaceOut('Password1')"
              />
              <div class="Register-form-Password1-right-img">
                <img
                  v-show="isPasswordValid"
                  src="@/assets/photo/勾.png"
                  alt="正确"
                  class="Register-form-right-img"
                />
              </div>
            </div>
            <div class="Register-form-Password1-right">
              <ul
                v-show="PasswordPrompt"
                class="Register-form-Password1-right-ul"
              >
                <li
                  v-show="!PasswordPromptNoSpace && !PasswordActive"
                  class="Register-form-Password1-right-li"
                  :class="{ ' nospace  ': nospace }"
                >
                  不能包括空格
                  <img
                    v-show="nospace"
                    src="@/assets/photo/勾.png"
                    alt="正确"
                    class="Register-form-right-nospace-img"
                  />
                </li>
                <li
                  v-show="!PasswordPromptspecial && !PasswordActive"
                  class="Register-form-Password1-right-li"
                  :class="{ ' specialChar  ': specialChar }"
                >
                  必须包含字母、数字、符号中至少2种
                  <img
                    v-show="specialChar"
                    src="@/assets/photo/勾.png"
                    alt="正确"
                    class="Register-form-right-specialChar-img"
                  />
                </li>
                <li
                  v-show="!PasswordPromptLength && !PasswordActive"
                  class="Register-form-Password1-right-li"
                  :class="{ ' passwordLength  ': passwordLength }"
                >
                  长度为8-16个字符
                  <img
                    v-show="passwordLength"
                    src="@/assets/photo/勾.png"
                    alt="正确"
                    class="Register-form-right-passwordLength-img"
                  />
                </li>
              </ul>
              <span
                v-show="PasswordActive"
                class="Register-form-Email-right-span"
              >
                密码不能为空
              </span>
            </div>
            <div class="Register-form-Password">
              <div class="Register-form-Password2-icon">
                <span class="rq">*</span>
                <label for="Password2">确定密码：</label>
              </div>
              <input
                id="Password2"
                :type="inputType"
                v-model="Password2"
                placeholder="Password2"
                class="Register-form-Password2-input"
                @input="checkPasswordConfirm"
                @blur="checkPasswordConfirm"
              />
              <div class="Register-form-Password2-right-img">
                <img
                  v-show="isPasswordConfirmValid"
                  src="@/assets/photo/勾.png"
                  alt="正确"
                  class="Register-form-right-img"
                />
              </div>
            </div>
            <div
              v-show="!isPasswordConfirmValid"
              class="Register-form-Password2-right"
            >
              <span
                v-show="Password2 && PasswordConfirmActive"
                class="Register-form-Password2-right-span"
                >两次密码不相同
                <img
                  v-show="isPasswordConfirmValid"
                  src="@/assets/photo/勾.png"
                  alt="正确"
                  class="Register-form-right-img"
                />
              </span>
            </div>
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
            <div class="Register-form-left-Verification">
              <div class="Register-form-Verification-icon">
                <span class="rq">*</span>
                <label for="VerificationCode">验证码：</label>
              </div>
              <input
                id="VerificationCode"
                v-model="VerificationCode"
                placeholder="VerificationCode"
                type="text"
                class="Register-form-Verification-input"
                autocomplete="off"
                @input="CheckVerify"
                @focus="CheckVerify"
                @blur="CheckVerify"
              />
              <div class="Register-form-Verification-right">
                <button
                  aria-label="关闭"
                  class="Register-form-Verification-right-button"
                  :class="{ ' after  ': isgetVerificationCode }"
                  :disabled="isgetVerificationCode || !isEmailValid"
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
            <div v-show="VerifyPrompt" class="Register-form-left-VerifyPrompt">
              <span>注册失败（检查邮箱地址和验证码是否错误）</span>
            </div>
            <div class="Register-form-submit">
              <button @click="Verify" class="Register-form-submit-button">
                注册
              </button>
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
import { useRouter } from "vue-router";
import axios from "axios";

const store = useStore();
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
const EmailActive = ref(false); // 邮箱输入框状态
const PasswordActive = ref(false); // 密码输入框状态
const PasswordConfirmActive = ref(false); // 确认密码输入框状态
const PasswordPrompt = ref(false);
const PasswordPromptNoSpace = ref(false); // 密码提示框状态
const PasswordPromptspecial = ref(false); // 密码提示框状态
const PasswordPromptLength = ref(false); // 密码提示框状态
const nospace = ref(false); // 不能输入空格
const specialChar = ref(false); // 特殊字符
const passwordLength = ref(false); // 密码长度
const VerifyPrompt = ref(false); // 验证码提示框状态

// 添加密码强度检查
const emailRegex = /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/; // 简单的邮箱格式验证
// const phoneRegex = /^1[3-9]\d{9}$/; // 中国大陆手机号验证
// const usernameRegex = /^[a-zA-Z](?!.*_{2})[a-zA-Z0-9_]{3,15}$/; // 用户名验证，4-16位字母、数字、下划线，不能以下划线开头或结尾，不能有连续下划线
/*const strongPassword =
  /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[@$!%*?&])[A-Za-z\d@$!%*?&]{8,}$/;*/ // 强密码：至少8个字符，包含大小写字母、数字和特殊字符
const passwordRegex =
  /^(?=.*[\S])[a-zA-Z\d!@#$%^&*()_+\-=[\]{};':"\\|,.<>?]{8,16}$/; // 简单密码验证，至少8个字符，不能有空格

const showRegister = computed(() => store.state.auth.showRegister);
const inputType = computed(() => (showPassword.value ? "text" : "password"));
const LoginGrade = computed(() => store.state.auth.LoginGrade);

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
  EmailActive.value = false;
  PasswordActive.value = false;
  PasswordConfirmActive.value = false;
  nospace.value = false;
  specialChar.value = false;
  passwordLength.value = false;
  PasswordPrompt.value = false;
  PasswordPromptNoSpace.value = false;
  PasswordPromptspecial.value = false;
  PasswordPromptLength.value = false;
  VerifyPrompt.value = false;
}

//注册成功操作
function register() {
  // 发送表单数据到后端
  const formData = {
    username: UserName.value,
    password: Password1.value,
    email: Email.value,
    code: VerificationCode.value,
  };
  axios
    .post("/api/form", formData)
    .then((response) => {
      console.log("Success:", response.data);
    })
    .catch((error) => {
      console.error("Error:", error);
    });
}
register;

function back() {
  // 实现返回功能
  if (store.state.auth.LoginGrade === 3) {
    store.commit("auth/openLogin");
    // 返回上一级登录界面
    store.commit("auth/upDataLoginButtonActive", {
      isinitLoginActive: false,
      isAcountLoginButtonActive: true,
      isWeChatLoginButtonActive: false,
      isPhoneLoginButtonActive: false,
      showRegister: false,
      choiceActive: true,
    });
    // 返回登录等级
    store.commit("auth/setLoginGradeActive", 2);
  }
}

function handleCloseLoginClick() {
  store.commit("auth/closeLogin");
  store.commit("auth/upDataLoginButtonActive", {
    isLoginButtonActive: false,
    isinitLoginActive: false,
    isAcountLoginButtonActive: false,
    isWeChatLoginButtonActive: false,
    isPhoneLoginButtonActive: false,
  });
  // 清空登录等级
  store.commit("auth/clearLoginGrade");
}

function closeRegister() {
  store.state.auth.showRegister = false;
  handleCloseLoginClick();
}

function togglePasswordVisibility() {
  showPassword.value = !showPassword.value;
}

function CheckSpaceOut(it: "Email" | "Password1") {
  switch (it) {
    case "Email":
      if (Email.value) {
        EmailActive.value = false;
      } else {
        EmailActive.value = true;
      }
      break;
    case "Password1":
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
      break;
  }
}

function CheckSpaceIn(it: "Email" | "Password1") {
  switch (it) {
    case "Email":
      if (Email.value) {
        EmailActive.value = false;
      } else {
        EmailActive.value = true;
      }
      break;
    case "Password1":
      PasswordPrompt.value = true;
      PasswordPromptNoSpace.value = false;
      PasswordPromptspecial.value = false;
      PasswordPromptLength.value = false;
      PasswordActive.value = false;
      break;
  }
}

function CheckEmail() {
  if (Email.value) {
    EmailActive.value = false;
  } else {
    EmailActive.value = true;
  }
  // 使用正确的ID选择器，并使用正则表达式的test方法
  if (passwordCheckTimeout.value) {
    clearTimeout(passwordCheckTimeout.value);
  }
  passwordCheckTimeout.value = setTimeout(() => {
    if (!emailRegex.test(Email.value)) {
      isEmailValid.value = false;
      return;
    }

    isEmailValid.value = true;
  }, 500); // 500毫秒延迟
}

/*function CheckPhone() {
  // 使用正确的ID选择器，并使用正则表达式的test方法
  const phoneElement = document.getElementById("Phone");
  if(phoneElement)
  {
    const phoneInput = phoneElement.value;
    isPhoneValid.value = phoneRegex.test(phoneInput);
  }
}*/

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
      PasswordConfirmActive.value = false;
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
    PasswordConfirmActive.value = true;
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

function Length() {
  if (/^.{8,}$/.test(Password1.value)) {
    passwordLength.value = true;
  } else {
    passwordLength.value = false;
  }
}

function getVerificationCode() {
  if (!isgetVerificationCode.value && isEmailValid.value) {
    if (passwordCheckTimeout.value) {
      clearTimeout(passwordCheckTimeout.value);
    }
    isgetVerificationCode.value = true;
    count.value = 60; // 设置倒计时时间为60秒
    // 开始倒计时
    startCountdown();
    axios
      .post("/api/verification/ready", {
        email: Email.value,
      })
      .catch((error) => {
        if (error.response) {
          if (error.response.status === 400) {
            alert("邮箱地址错误");
          } else {
            alert("服务器错误: " + error.response.status);
          }
        } else if (error.request) {
          // 请求已发出但没有收到响应
          alert("网络错误，请检查网络连接");
        } else {
          // 其他错误
          alert("请求错误: " + error.message);
        }
      });
  }
}

function Verify() {
  // 在注册组件中
  store
    .dispatch("auth/register", {
      email: Email.value,
      code: VerificationCode.value,
    })
    .then((response) => {
      if (response.status === 200 && response.data.success) {
        store.dispatch("auth/registerSetUser", {
          email: Email.value,
          password: Password1.value,
        });
        alert("注册成功");
        // 登录
        store.commit("auth/login");
        // 注册成功并自动登录，可以跳转到主页
        router.push("/");
      }
    })
    .catch((error) => {
      if (error.response && error.response.status === 401) {
        VerifyPrompt.value = true;
      } else if (error.response && error.response.status === 400) {
        alert("请输入邮箱或者验证码");
      }
      // 处理注册错误
      console.error("注册失败:", error);
    });
}

function CheckVerify() {
  if (!VerificationCode.value) {
    VerifyPrompt.value = false;
  }
  if (VerificationCode.value) {
    VerifyPrompt.value = false;
  }
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

<style scoped lang="scss">
@import "@/assets/styles/register.css";
</style>
