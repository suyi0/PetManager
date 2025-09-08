<template>
  <div class="PersonalView">
    <Personal />
  </div>
</template>

<script setup lang="ts">
import { onMounted } from "vue";
import { useStore } from "vuex";
import { key } from "@/store";
import Personal from "@/components/PersonalPage.vue";

const store = useStore(key);

onMounted(() => {
  // 确保进入个人中心页面时关闭登录界面和其他弹出界面
  if (store.state.auth.showLogin) {
    store.commit("auth/closeLogin");
  }
  if (store.state.auth.personal) {
    store.commit("auth/closePersonal");
  }
});
</script>

<style scoped lang="scss">
@import "@/assets/styles/Personal.css";
</style>
