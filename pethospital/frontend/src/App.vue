<template>
  <div class="app-container">
    <div v-if="!isLoggedIn">
      <button @click="logout">登出</button>
      <div v-if="!showRegister" class="initial_container">
        <div class="app-backgroundImage"></div>
        <Login />
      </div>
      <!-- 注册路由 -->
      <div v-else-if="showRegister" class="router-view">
        <div class="router-view">
          <router-view></router-view>
        </div>
      </div>
    </div>
    <div v-if="isLoggedIn" class="logined-container">
      <div>
        <router-view></router-view>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted } from "vue";
import { useStore } from "vuex";
import { key } from "@/store/userStore";
import Login from "./components/LoginPage.vue";
import router from "./router";

const store = useStore(key);

const isLoggedIn = computed(() => store.state.auth.isLoggedIn);
const showRegister = computed(() => store.state.auth.showRegister);
const logout = () => store.dispatch("auth/logout");

onMounted(() => {
  if (isLoggedIn.value) {
    router.push("/user/home");
  }
});
</script>

<style>
.app-container {
  position: relative;
  width: 100vw;
  height: 100vh;
  overflow: hidden;
  background-color: rgb(255, 255, 255);

  router-view {
    width: 100%;
    height: 100%;
  }

  .logined-container {
    width: 100vw;
    height: 100vh;
  }
}
</style>
