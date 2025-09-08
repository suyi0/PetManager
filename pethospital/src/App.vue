<template>
  <div class="app-container">
    <div class="app-navbar">
      <!-- 导航栏始终显示 -->
      <nav>
        <!-- 使用 router-link 组件来导航. -->
        <!-- 通过传入 `to` 属性指定链接. -->
        <!-- <router-link> 默认会被渲染成一个 `<a>` 标签 -->
        <router-link to="/" class="home"><span>首页</span></router-link>
        <router-link to="/about" class="about">
          <span>关于我们</span>
        </router-link>
        <button
          v-if="!isLoggedIn"
          class="login"
          :class="{ 'login-active2': isLoginButtonActive }"
          @click="handleLoginClick"
        >
          <span>Login</span>
        </button>
        <div v-else class="onLine">
          <div class="onLine-line" @click="openPersonal">
            <div class="onLine-line-header">
              <img
                class="onLine-img"
                src="@/assets/photo/head.jpeg"
                alt="store.state.auth.userName"
              />
            </div>
            <div class="onLine-line-name">
              {{ store.getters["auth/formattedUserName"] }}
            </div>
            <div class="onLine-pulldown">
              <svg
                xmlns="http://www.w3.org/2000/svg"
                width="6"
                height="6"
                fill="currentColor"
              >
                <g clip-path="url(#caret_down_6_svg__a)">
                  <path
                    d="M5.272 1a.5.5 0 0 1 .405.793L3.405 4.939a.5.5 0 0 1-.81 0L.323 1.793A.5.5 0 0 1 .728 1z"
                  ></path>
                </g>
                <defs>
                  <clipPath id="caret_down_6_svg__a">
                    <path fill="#fff" d="M0 0h6v6H0z"></path>
                  </clipPath>
                </defs>
              </svg>
            </div>
          </div>
          <div v-show="personal" class="onLine-vertical" ref="personalMenu">
            <div class="onLine-vertical-container">
              <div class="onLine-vertical-container-top">
                <img
                  class="onLine-vertical-container-top-img"
                  src="@/assets/photo/head.jpeg"
                  alt="store.state.auth.userName"
                />
                <span class="onLine-vertical-container-top-name">{{
                  store.state.auth.userEmail
                }}</span>
              </div>
              <div class="onLine-vertical-container-main">
                <router-link to="/user/personal">个人中心</router-link>
                <router-link to="/user/order">我的订单</router-link>
                <router-link to="/user/collect">我的收藏</router-link>
                <router-link to="/user/coupon">帮助中心</router-link>
                <router-link to="/user/coupon">账号安全</router-link>
              </div>
              <div class="onLine-vertical-container-bottom">
                <button class="switch-account-button" @click="logout">
                  切换账号
                </button>
                <button class="logout-button" @click="logout">退出登录</button>
              </div>
            </div>
          </div>
        </div>
      </nav>
    </div>

    <!-- 内容区域根据路由变化 -->
    <main class="app-content">
      <router-view />
    </main>

    <div v-if="showLogin">
      <Login />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from "vue";
import { useStore } from "vuex";
import { key } from "@/store";
import { useRoute } from "vue-router";
import Login from "@/components/LoginPage.vue"; // 引入Login组件

const store = useStore(key);
const route = useRoute();
const isLoggedIn = computed(() => store.state.auth.isLoggedIn);
const showLogin = computed(() => store.state.auth.showLogin);
const personal = computed(() => store.state.auth.personal);
//const logout = () => store.dispatch("auth/logout");

// 页面加载时检查登录状态
onMounted(() => {
  // 从 localStorage 获取 token
  const token = localStorage.getItem("auth_token");
  if (token) {
    // 如果有 token，可以验证其有效性
    // 这里可以调用一个验证 token 的 API
    // 或者直接设置登录状态
    store.commit("auth/setLoginStatus", true);
  }
  //对于单一用途性能好

  //  // 添加点击外部关闭菜单的逻辑
  // const handleClickOutside = (event: MouseEvent) => {
  //   if (personalMenu.value && !personalMenu.value.contains(event.target as Node)) {
  //     closePersonal();
  //   }
  // };

  // document.addEventListener("click", handleClickOutside);
  // return () => {
  //   document.removeEventListener("click", handleClickOutside);
  // };
});

// 监听路由变化，当进入个人中心页面时关闭登录界面
watch(
  () => route.name,
  (newRouteName) => {
    if (newRouteName === "userPersonal") {
      // 关闭登录界面
      if (store.state.auth.showLogin) {
        store.commit("auth/closeLogin");
      }
      // 关闭个人下拉菜单
      if (store.state.auth.personal) {
        store.commit("auth/closePersonal");
      }
    }
  }
);
const openLogin = () => store.commit("auth/openLogin");
//因为 openLogin 在 store 中是一个 mutation，
// 而不是 action。在 Vuex 中，
// mutations 通过 commit 调用，
// actions 通过 dispatch 调用。

const logout = () => store.dispatch("auth/logout");

// 添加按钮状态管理
const isLoginButtonActive = computed(
  () => store.state.auth.isLoginButtonActive
);
const setIsLoginButtonActive = () => {
  store.commit("auth/upDataLoginButtonActive", {
    isLoginButtonActive: true,
  });
  store.commit("auth/upDataLoginButtonActive", {
    isinitLoginActive: true,
  });
};
const setLoginGradeActive = () => {
  store.commit("auth/setLoginGradeActive", 1);
};
// 合并两个点击处理函数
const handleLoginClick = () => {
  openLogin();
  setIsLoginButtonActive();
  setLoginGradeActive();
};
// 点击个人头像按钮时，打开个人中心
const openPersonal = (event: Event) => {
  // 阻止事件冒泡，避免触发 handleClickOutside
  event.stopPropagation();
  store.commit("auth/openPersonal");
};
const closePersonal = () => {
  store.commit("auth/closePersonal");
};
// 创建一个 ref 来引用个人菜单元素
const personalMenu = ref<HTMLElement | null>(null);

// 添加 useClickOutside 自定义 Hook
const useClickOutside = (elementRef: any, callback: () => void) => {
  //elementRef: any - 传入的DOM元素引用
  //callback: () => void - 当点击该元素外部时执行的回调函数

  const handleClickOutside = (event: MouseEvent) => {
    // 确保 elementRef.value 存在并且点击的元素不在该元素内部
    if (elementRef.value && !elementRef.value.contains(event.target)) {
      callback();
    }
  };

  onMounted(() => {
    document.addEventListener("click", handleClickOutside);
  });
  // 清理事件监听器
  const stopClickOutside = () => {
    document.removeEventListener("click", handleClickOutside);
  };

  return {
    stopClickOutside,
  };
};

// 使用 useClickOutside Hook
useClickOutside(personalMenu, closePersonal);
</script>

<style>
@import "@/assets/styles/Home.css";
@import "@/assets/styles/Login.css";
@import "@/assets/styles/About.css";
@import "@/assets/styles/App.css";
</style>
