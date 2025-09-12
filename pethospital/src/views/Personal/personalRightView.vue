<template>
  <div class="personal-container-right-personal">
    <div class="personal-container-personal-right-top">
      <div class="personal-container-personal-right-top-head">
        <button
          class="personal-container-personal-right-top-head-button"
          @click="emit('switchTab', 'head')"
        >
          <img
            class="personal-container-personal-right-top-head-img"
            src="@/assets/photo/head.jpeg"
            alt="store.state.auth.userName"
          />
        </button>
      </div>
    </div>
    <div class="personal-container-right-personal-middle">
      <div
        class="personal-container-right-personal-middle-name"
        @click="emit('switchTab', 'name')"
      >
        <button class="right-personal-button">
          <div class="personal-container-right-personal-middle-div">姓名:</div>
          <span class="personal-container-right-personal-middle-span">{{
            UserName
          }}</span>
        </button>
      </div>
      <div
        class="personal-container-right-personal-middle-brithday"
        @click="emit('switchTab', 'birthday')"
      >
        <button class="right-personal-button">
          <div class="personal-container-right-personal-middle-div">
            出生日期:
          </div>
          <span class="personal-container-right-personal-middle-span">{{
            UserBirthday
          }}</span>
        </button>
      </div>
      <div
        class="personal-container-right-personal-middle-phone"
        @click="emit('switchTab', 'phone')"
      >
        <button class="right-personal-button">
          <div class="personal-container-right-personal-middle-div">
            手机号:
          </div>
          <div class="personal-container-right-personal-middle-span">
            <span class="personal-container-right-personal-middle-span">{{
              UserPhone
            }}</span>
          </div>
        </button>
      </div>
      <div
        class="personal-container-right-personal-middle-email"
        @click="emit('switchTab', 'email')"
      >
        <button class="right-personal-button">
          <div class="personal-container-right-personal-middle-div">邮箱:</div>
          <div class="personal-container-right-personal-middle-span">
            <span class="personal-container-right-personal-middle-span">{{
              UserEmail
            }}</span>
          </div>
        </button>
      </div>
      <div
        class="personal-container-right-personal-middle-address"
        @click="emit('switchTab', 'address')"
      >
        <!-- 允许子组件通过$emit触发自定义事件，父组件可以监听这些事件并作出响应。 -->
        <button class="right-personal-button">
          <div class="personal-container-right-personal-middle-div">
            家庭地址:
          </div>
          <div class="personal-container-right-personal-middle-span">
            <span>{{ UserAddress }}</span>
          </div>
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from "vue";
import { useStore } from "vuex";
import { key } from "@/store";

// 使用store
const store = useStore(key);

const emit = defineEmits(["switchTab"]);

// computed 属性。它们是纯函数，只负责根据现有状态计算并返回新值，不修改任何状态。
const UserName = computed(() => {
  return store.getters["auth/formattedUserName"];
});

const UserBirthday = computed(() => {
  if (!store || !store.state) return "";
  return store.state.auth.userBirthday;
});

const UserPhone = computed(() => {
  if (!store || !store.state) return "";
  const phone = store.state.auth.userPhone;
  if (!phone) return "";
  return phone.replace(/^(\+\d{1,3})(\d{11})$/, "$1 $2");
});

const UserEmail = computed(() => {
  if (!store || !store.state) return "";
  return store.state.auth.userEmail;
});

const UserAddress = computed(() => {
  if (!store || !store.state) return "";
  return store.state.auth.userAddress;
});
</script>

<style scoped lang="scss">
@import "@/assets/styles/Personal.css";
</style>
