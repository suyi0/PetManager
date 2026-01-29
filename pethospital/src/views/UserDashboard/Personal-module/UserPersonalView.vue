<template>
  <div class="PersonalView">
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
            <button class="personal-button1"><span>个人资料</span></button>
          </div>
          <div
            class="personal-left2"
            :class="{ ' click ': activeTab === 'address' }"
            @click="switchTab('address')"
          >
            <button class="personal-button2"><span>配送地址</span></button>
          </div>
        </div>
        <div class="personal-constainer-right">
          <PersonalRightView
            v-if="activeTab === 'personal'"
            @switchTab="switchTab"
          />
          <SetHeadView v-if="activeTab === 'head'" @close="close" />
          <SetNameView
            v-if="activeTab === 'name'"
            @close="close"
            @submit="submit"
          />
          <SetBirthdayView
            v-if="activeTab === 'birthday'"
            @close="close"
            @submit="submit"
          />
          <SetPhoneView
            v-if="activeTab === 'phone'"
            @close="close"
            @submit="submit"
          />
          <SetEmailView
            v-if="activeTab === 'email'"
            @close="close"
            @submit="submit"
          />
          <SetAddressView v-if="activeTab === 'address'" />
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted, onBeforeUnmount, ref } from "vue";
import { useStore } from "vuex";
import { key } from "@/store/userStore";
import PersonalRightView from "./personalRightView.vue";
import SetHeadView from "./setHeadView.vue";
import SetNameView from "./setNameView.vue";
import SetBirthdayView from "./setBirthdayView.vue";
import SetPhoneView from "./setPhoneView.vue";
import SetEmailView from "./setEmailView.vue";
import SetAddressView from "./setAddressView.vue";

// 使用 store
const store = useStore(key);

// 响应式数据
const activeTab = ref("personal");

// 计算属性
const userName = () => store.state.auth.userName;
userName;
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
  // 确保进入个人中心页面时关闭登录界面和其他弹出界面
  if (store.state.auth.personal) {
    store.commit("auth/closePersonal");
  }
});

onBeforeUnmount(() => {
  window.removeEventListener("beforeunload", handleBeforeUnload);
});
</script>

<style scoped lang="scss">
@import "@/assets/styles/UserDashboard/UserPersonal.css";
</style>
