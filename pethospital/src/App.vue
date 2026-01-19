<template>
  <div class="app-container">
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
</template>

<script setup lang="ts">
import { computed } from "vue";
import { useStore } from "vuex";
import { key } from "@/store/userStore";
import Login from "./components/LoginPage.vue";

const store = useStore(key);

const showRegister = computed(() => store.state.auth.showRegister);
</script>

<style>
.app-container {
  position: relative;
  width: 100vw;
  height: 100vh;
  overflow: hidden;
  background-color: rgb(255, 255, 255);

  .router-view {
    width: 100%;
    height: 100%;
  }
}
</style>
