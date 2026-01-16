<template>
  <div class="app-container">
    <div class="app-backgroundImage"></div>
    <div class="Login-container">
      <Login />
    </div>
  </div>
</template>

<script setup lang="ts">
import Login from "./components/LoginPage.vue";
</script>

<style>
.app-container {
  position: relative;
  width: 100vw;
  height: 100vh;
  overflow: hidden;
  background-color: rgb(255, 255, 255);
}
</style>
