<template>
  <section class="personal-page">
    <aside class="personal-sidebar">
      <div class="personal-sidebar__hero">
        <p>Personal Center</p>
        <h2>个人中心</h2>
        <span>管理头像、姓名、联系方式与家庭地址。</span>
      </div>

      <button
        class="personal-sidebar__item"
        :class="{ 'personal-sidebar__item--active': activeTab === 'personal' }"
        @click="switchTab('personal')"
      >
        个人资料
      </button>
      <button
        class="personal-sidebar__item"
        :class="{ 'personal-sidebar__item--active': activeTab === 'address' }"
        @click="switchTab('address')"
      >
        配送地址
      </button>
    </aside>

    <div class="personal-stage">
      <section class="personal-stage__header">
        <div>
          <p>Profile</p>
          <h3>{{ panelTitle }}</h3>
        </div>
        <span>{{ panelDescription }}</span>
      </section>

      <section class="personal-stage__content">
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
        <SetAddressView
          v-if="activeTab === 'address'"
          @close="close"
          @submit="submit"
        />
      </section>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";
import PersonalRightView from "./personalRightView.vue";
import SetHeadView from "./setPersonal-Iformation/setHeadView.vue";
import SetNameView from "./setPersonal-Iformation/setNameView.vue";
import SetBirthdayView from "./setPersonal-Iformation/setBirthdayView.vue";
import SetPhoneView from "./setPersonal-Iformation/setPhoneView.vue";
import SetEmailView from "./setPersonal-Iformation/setEmailView.vue";
import SetAddressView from "./setPersonal-Iformation/setAddressView.vue";

type PersonalTab =
  | "personal"
  | "address"
  | "head"
  | "name"
  | "birthday"
  | "phone"
  | "email";

interface SubmitData {
  field: string;
  name?: string;
  birthday?: string;
  phone?: string;
  address?: string;
}

const store = useStore(storeKey);
const activeTab = ref<PersonalTab>("personal");

const panelTitle = computed(() =>
  activeTab.value === "address" ? "地址与配送信息" : "用户资料总览"
);
const panelDescription = computed(() =>
  activeTab.value === "address"
    ? "编辑家庭地址、配送地址和收货相关信息。"
    : "点开任意卡片即可进入对应编辑项，不会影响其他资料。"
);

const switchTab = (tab: string) => {
  activeTab.value = tab as PersonalTab;
};

const close = () => {
  activeTab.value = "personal";
};

const submit = (data: SubmitData) => {
  activeTab.value = "personal";

  if (data.field === "userName") {
    store.dispatch("currentUser/updateUserField", {
      field: data.field,
      value: data.name,
    });
  }
  if (data.field === "userBirthday") {
    store.dispatch("currentUser/updateUserField", {
      field: data.field,
      value: data.birthday,
    });
  }
  if (data.field === "userPhone") {
    store.dispatch("currentUser/updateUserField", {
      field: data.field,
      value: data.phone,
    });
  }
  if (data.field === "userAddress") {
    store.dispatch("currentUser/updateUserField", {
      field: data.field,
      value: data.address,
    });
  }
};

const handleBeforeUnload = () => {
  if (store.state.auth.isLoggedIn) {
    const formData = {
      name: store.state.currentUser.userName,
      phone: store.state.currentUser.userPhone,
      email: store.state.currentUser.userEmail,
      birthday: store.state.currentUser.userBirthday,
      addressId: store.state.currentUser.userAddressId,
      address: store.state.currentUser.userAddress,
    };
    const blob = new Blob([JSON.stringify(formData)], {
      type: "application/json",
    });
    navigator.sendBeacon("/api/upload/form", blob);
  }
};

onMounted(() => {
  window.addEventListener("beforeunload", handleBeforeUnload);
  if (store.state.ui.personal) {
    store.commit("ui/closePersonal");
  }
});

onBeforeUnmount(() => {
  window.removeEventListener("beforeunload", handleBeforeUnload);
});
</script>

<style scoped lang="scss">
.personal-page {
  display: grid;
  grid-template-columns: 260px minmax(0, 1fr);
  gap: 20px;
  min-height: calc(100vh - 170px);
}

.personal-sidebar,
.personal-stage {
  border-radius: 30px;
  border: 1px solid rgba(21, 91, 92, 0.1);
  background: rgba(255, 250, 242, 0.82);
  box-shadow: 0 24px 55px rgba(25, 92, 93, 0.08);
}

.personal-sidebar {
  position: sticky;
  top: 128px;
  align-self: start;
  display: grid;
  gap: 10px;
  padding: 18px;
}

.personal-sidebar__hero {
  display: grid;
  gap: 6px;
  padding: 10px 4px 14px;
}

.personal-sidebar__hero p,
.personal-stage__header p {
  margin: 0;
  color: #1e8a88;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 12px;
  font-weight: 700;
}

.personal-sidebar__hero h2,
.personal-stage__header h3 {
  margin: 0;
  color: #143d40;
  font-size: 30px;
}

.personal-sidebar__hero span,
.personal-stage__header span {
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.personal-sidebar__item {
  padding: 14px 16px;
  border: 1px solid transparent;
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.58);
  color: #163f42;
  text-align: left;
  font-size: 15px;
  font-weight: 700;
  cursor: pointer;
}

.personal-sidebar__item--active {
  border-color: rgba(29, 134, 135, 0.18);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.28),
    rgba(243, 197, 155, 0.22)
  );
  box-shadow: 0 16px 30px rgba(28, 98, 99, 0.1);
}

.personal-stage {
  overflow: hidden;
}

.personal-stage__header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 16px;
  padding: 22px 24px 14px;
}

.personal-stage__content {
  padding: 0 24px 24px;
}

.personal-stage :deep(.personal-container-right-personal) {
  width: 100%;
}

@media (max-width: 1100px) {
  .personal-page {
    grid-template-columns: 1fr;
  }

  .personal-sidebar {
    position: static;
  }
}
</style>
