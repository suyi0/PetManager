<template>
  <div class="usersHome-container">
    <div class="usersHome-navbar">
      <!-- 导航栏始终显示 -->
      <nav class="usersHome-nav">
        <!-- 使用 router-link 组件来导航. -->
        <!-- 通过传入 `to` 属性指定链接. -->
        <!-- <router-link> 默认会被渲染成一个 `<a>` 标签 -->
        <router-link
          to="/"
          class="nav-home"
          @mouseenter="handleMouseEnter('home')"
          @mouseleave="handleMouseLeave('home')"
          :class="{ 'nav-home-hover': hoveredLink === 'home' }"
          ><span>首页</span></router-link
        >
        <router-link
          to="/about"
          class="nav-about"
          @mouseenter="handleMouseEnter('about')"
          @mouseleave="handleMouseLeave('about')"
          :class="{ 'nav-about-hover': hoveredLink === 'about' }"
        >
          <span>关于</span>
        </router-link>
        <router-link
          to="/services"
          class="nav-services"
          @mouseenter="handleMouseEnter('services')"
          @mouseleave="handleMouseLeave('services')"
          :class="{ 'nav-services-hover': hoveredLink === 'services' }"
        >
          <span>服务</span>
        </router-link>
        <div class="onLine">
          <div class="onLine-line" @click="openPersonal">
            <div class="onLine-line-header">
              <img
                :src="store.state.auth.userHeadImage"
                alt="头像"
                class="onLine-img"
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
                  :src="store.state.auth.userHeadImage"
                  class="onLine-vertical-container-top-img"
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
    <main class="usersHome-content">
      <router-view />
    </main>

    <!-- 测试按钮，仅在开发环境中显示 -->
    <div
      v-if="!isLoggedIn"
      style="position: fixed; bottom: 20px; right: 20px; z-index: 1000"
    >
      <button
        @click="simulateLogin"
        style="
          background: #42b983;
          color: white;
          border: none;
          padding: 10px 20px;
          border-radius: 4px;
        "
      >
        模拟登录
      </button>
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

const hoveredLink = ref("");

const handleMouseEnter = (link: string) => {
  hoveredLink.value = link;
};

const handleMouseLeave = (link: string) => {
  if (hoveredLink.value === link) {
    hoveredLink.value = "";
  }
};

const simulateLogin = () => {
  store.commit("auth/frontSetUser", {
    userName: "测试用户",
    userPhone: "1234567890",
    userEmail: "test@example.com",
    userBirthday: "1990-01-01",
    userAddress: "北京市",
    userHeadImage: "",
    token: "test_token",
  });
};

// 添加对 store 是否存在的检查
if (!store) {
  console.error("Store is not properly initialized");
}

const isLoggedIn = computed(() => store.state.auth.isLoggedIn);
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
      // 关闭个人下拉菜单
      if (store.state.auth.personal) {
        store.commit("auth/closePersonal");
      }
    }
  }
);

// 在 Vuex 中，
// mutations 通过 commit 调用，
// actions 通过 dispatch 调用。

const logout = () => store.dispatch("auth/logout");
const setLoginGradeActive = () => {
  store.commit("auth/setLoginGradeActive", 1);
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

<style scoped lang="scss">
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}
html,
body {
  font-family: "Helvetica Neue", Arial, sans-serif;
  color: #2c3e50;
  line-height: 1.6;
  overflow-x: hidden;
  background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}
.app-container {
  background-color: rgb(255, 255, 255);
  background-size: cover;
  background-position: center;
  width: 100%;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}
.app-navbar {
  flex-shrink: 0; /* 保持导航栏在顶部，不随内容滚动 */
  width: 100%;
  height: 45px;
  background-color: rgba(62, 76, 86, 0.3);
  border-radius: 12px;
  position: fixed; /* 添加此属性 */
  top: 0; /* 固定在顶部 */
  z-index: 1000; /* 确保在其他元素之上 */
}
.app-nav {
  display: flex;
  gap: 18px;
  padding: 7px;
  font-size: 22px;
}
.app-content {
  flex: 1; /* 内容区域占据剩余空间 */
  width: 100%;
  height: 100%;
  box-sizing: border-box; /* 确保padding不会增加元素总宽度 */
  padding: 45px 0 0 0; /* 为固定导航栏留出空间 */
}
a.nav-home {
  margin-left: 20px;
  border-radius: 12px;
}
a.nav-home,
a.nav-about,
a.nav-services {
  width: 100px;
  border-radius: 12px;
  transform: scale(1);
  transition: all 1.5s ease;
  display: flex;
  align-items: center;
  justify-content: center;
}
.nav-home-hover,
.nav-about-hover,
.nav-services-hover {
  width: 100px;
  border-radius: 12px;
  /* 可以添加其他样式 */
  transform: scale(1.1);
  transition: all 1.5s ease;
  background-color: rgba(255, 255, 255, 0.6);
}

.login,
.onLine {
  color: rgb(0, 0, 0);
  font-size: 34px;
  font-weight: bold;
  text-align: center;
  width: 100px;
  height: 30px;
  text-decoration: none;
  margin-right: 10px;
  margin-bottom: 10px;
  position: absolute;
  right: 0px;
  background-color: rgba(255, 255, 255, 0);
  border: none; /* 去除边框 */
  cursor: pointer; /* 鼠标悬停时变为手型指针 */
}
.login-active2 {
  color: #42b983;
}
.login {
  padding: 0;
}
.login-text {
  font-size: 22px;
  display: flex;
  align-items: center;
  justify-content: center;
  line-height: 1.6;
}

#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: justify;
  color: #2c3e50;
  width: 100%;
}

nav {
  padding: 0px;
  display: flex;
}

.router-link-active,
a {
  text-decoration: none;
  height: 30px;
}

nav a {
  font-weight: bold;
}
nav a.router-link-active span {
  color: #42b983;
}

.onLine {
  width: 200px;
  height: 40px;
}
.onLine-line {
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  font-size: 16px;
}
.onLine-img {
  width: 30px;
  height: 30px;
  border-radius: 90px;
}
.onLine-pulldown {
  display: flex;
}
.onLine-vertical {
  width: 200px;
  height: 300px;
  display: flex;
  flex-direction: column;
  cursor: default;
}
.onLine-vertical-container {
  width: 200px;
  height: 300px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: rgb(255, 255, 255);
  /* box-shadow: h-offset v-offset blur spread color inset; */
  /*  h-offset（必需）：水平偏移量，正值向右，负值向左
        v-offset（必需）：垂直偏移量，正值向下，负值向上
        blur（可选）：模糊距离，值越大阴影越模糊
        spread（可选）：阴影扩展半径，正值扩大阴影，负值缩小阴影
        color（可选）：阴影颜色
        inset（可选）：内部阴影关键字 */
  box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
  border-radius: 4px; /* 添加圆角 */
  position: relative;
}
/* 上标箭头 */
/* 添加了伪元素 ::before 来创建上标箭头
使用CSS三角形技巧（border技术）创建箭头形状 */
.onLine-vertical-container::before {
  content: "";
  position: absolute;
  top: -6px;
  left: 50%;
  transform: translateX(-50%);
  width: 0;
  height: 0;
  border-left: 6px solid transparent;
  border-right: 6px solid transparent;
  border-bottom: 6px solid rgb(255, 255, 255);
  z-index: 1;
  font-size: 15px;
}
.onLine-vertical-container-top {
  width: 90%;
  height: 100px;
  position: relative;
  font-size: 16px;
  display: flex;
  flex-direction: column;
  align-items: center;
}
.onLine-vertical-container-top-img {
  width: 40px;
  height: 40px;
  border-radius: 90px;
  margin-top: 16px;
  margin-bottom: 16px;
}
.onLine-vertical-container-top-name {
  width: 100%;
  height: 40px;
}
.onLine-line-name {
  width: 165px;
  height: 40px;
  display: flex;
  align-items: center;
  justify-content: center;
}
.onLine-vertical-container-top::after,
.onLine-vertical-container-main::after {
  content: "";
  display: inline-block;
  width: 100%;
  height: 1px;
  position: absolute;
  bottom: 1px;
  left: 0px;
  background-color: rgba(81, 77, 77, 0.5);
}
.onLine-vertical-container-main {
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  width: 90%;
  height: 170px;
  font-size: 16px;
  position: relative;
}
.onLine-vertical-container-bottom {
  display: flex;
  flex-direction: row;
  justify-content: space-between;
  width: 100%;
  height: 30px;
  position: relative;
  background-color: rgba(167, 184, 187, 0.3);
}
.switch-account-button,
.logout-button {
  width: auto;
  font-size: 12px;
}
.switch-account-button:active,
.logout-button:active {
  color: rgb(212, 21, 21);
  transform: scale(0.98); /*点击时轻微缩小 */
  box-shadow: inset 0 0 5px rgba(255, 255, 255, 0.2); /* 内阴影效果 */
}
</style>
