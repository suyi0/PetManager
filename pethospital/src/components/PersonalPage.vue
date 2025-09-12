<template>
  <div class="personal-container">
    <div class="personal-container-top">
      <span>个人中心</span>
    </div>
    <div class="personal-constainer-middle">
      <div class="personal-constainer-left">
        <div
          class="personal-left1"
          :class="{ ' click ': activeTab === 'personal' }"
          @click="switchTab('personal')"
        >
          <button class="personal-button1">个人资料</button>
        </div>
        <div
          class="personal-left2"
          :class="{ ' click ': activeTab === 'address' }"
          @click="switchTab('address')"
        >
          <button class="personal-button2">配送地址</button>
        </div>
      </div>
      <div class="personal-constainer-right">
        <PersonalRightView
          v-if="activeTab === 'personal'"
          @switchTab="switchTab"
        />
        <SetHeadView v-if="activeTab === 'head'" @close="close" />
        <SetNameView v-if="activeTab === 'name'" @close="close" />
        <SetBirthdayView v-if="activeTab === 'birthday'" @close="close" />
        <SetPhoneView
          v-if="activeTab === 'phone'"
          @close="close"
          @submit="submit"
        />
        <SetEmailView v-if="activeTab === 'email'" @close="close" />
        <SetAddressView v-if="activeTab === 'address'" />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from "vue";
import { useStore } from "vuex";
import { key } from "../store";
import PersonalRightView from "../views/Personal/personalRightView.vue";
import SetHeadView from "../views/Personal/setHeadView.vue";
import SetNameView from "../views/Personal/setNameView.vue";
import SetBirthdayView from "../views/Personal/setBirthdayView.vue";
import SetPhoneView from "../views/Personal/setPhoneView.vue";
import SetEmailView from "../views/Personal/setEmailView.vue";
import SetAddressView from "../views/Personal/setAddressView.vue";

// 使用 store
const store = useStore(key);

// 响应式数据
const activeTab = ref("personal");

// 计算属性
const userName = () => store.state.auth.userName;

// 方法
const switchTab = (tab: string) => {
  activeTab.value = tab;
};

const close = () => {
  activeTab.value = "personal";
};

interface SubmitData {
  field: string;
  name?: string;
  birthday?: string;
  phone?: string;
  address?: string;
}

const submit = (data: SubmitData) => {
  activeTab.value = "personal";
  if (data.field === "userName") {
    store.dispatch("auth/updateUserField", {
      field: data.field,
      value: data.name,
    });
  }
  if (data.field === "userBirthday") {
    store.dispatch("auth/updateUserField", {
      field: data.field,
      value: data.birthday,
    });
  }
  if (data.field === "userPhone") {
    store.dispatch("auth/updateUserField", {
      field: data.field,
      value: data.phone,
    });
  }
  if (data.field === "userAddress") {
    store.dispatch("auth/updateUserField", {
      field: data.field,
      value: data.address,
    });
  }
};

// 处理页面关闭前的保存操作
const handleBeforeUnload = () => {
  if (store.state.auth.isLoggedIn) {
    // 使用 sendBeacon 确保数据发送
    const formData = {
      name: store.state.auth.userName,
      phone: store.state.auth.userPhone,
      email: store.state.auth.userEmail,
      birthday: store.state.auth.userBirthday,
      addressId: store.state.auth.userAddressId,
      address: store.state.auth.userAddress,
    };
    const blob = new Blob([JSON.stringify(formData)], {
      type: "application/json",
    });
    navigator.sendBeacon("/api/user/form", blob);
  }
};

// 生命周期钩子
onMounted(() => {
  window.addEventListener("beforeunload", handleBeforeUnload);
});

onBeforeUnmount(() => {
  window.removeEventListener("beforeunload", handleBeforeUnload);
});
userName;
</script>

<style scoped lang="scss">
@import "../assets/styles/Personal.css";
</style>
