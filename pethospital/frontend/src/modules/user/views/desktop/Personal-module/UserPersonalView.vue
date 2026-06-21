<template>
  <section class="personal-page">
    <div class="pp-head">
      <h2>个人中心</h2>
      <div class="seg">
        <button
          v-for="item in navItems"
          :key="item.key"
          class="seg__btn"
          :class="{ 'seg__btn--active': segActive(item.key) }"
          @click="switchTab(item.key)"
        >
          {{ item.label }}
        </button>
      </div>
    </div>

    <section class="personal-content">
      <PersonalRightView
        v-if="activeTab === 'personal'"
        @switchTab="switchTab"
      />
      <PetProfilesView
        v-else-if="activeTab === 'pet'"
        @updateCount="petCount = $event"
      />
      <SetAddressView
        v-else-if="activeTab === 'address'"
        @close="close"
        @submit="submit"
      />
      <SetHeadView v-else-if="activeTab === 'head'" @close="close" />
      <SetNameView
        v-else-if="activeTab === 'name'"
        @close="close"
        @submit="submit"
      />
      <SetBirthdayView
        v-else-if="activeTab === 'birthday'"
        @close="close"
        @submit="submit"
      />
      <SetPhoneView
        v-else-if="activeTab === 'phone'"
        @close="close"
        @submit="submit"
      />
      <SetEmailView
        v-else-if="activeTab === 'email'"
        @close="close"
        @submit="submit"
      />
    </section>
  </section>
</template>

<script setup lang="ts">
import { onMounted, ref, watch } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { useRoute, useRouter } from "vue-router";
import PersonalRightView from "./personalRightView.vue";
import PetProfilesView from "./PetProfilesView.vue";
import SetHeadView from "./setPersonal-Iformation/setHeadView.vue";
import SetNameView from "./setPersonal-Iformation/setNameView.vue";
import SetBirthdayView from "./setPersonal-Iformation/setBirthdayView.vue";
import SetPhoneView from "./setPersonal-Iformation/setPhoneView.vue";
import SetEmailView from "./setPersonal-Iformation/setEmailView.vue";
import SetAddressView from "./setPersonal-Iformation/setAddressView.vue";

type PersonalTab =
  | "personal"
  | "pet"
  | "address"
  | "head"
  | "name"
  | "birthday"
  | "phone"
  | "email";

interface SubmitData {
  field: string;
  name?: string;
  lastName?: string;
  middleName?: string;
  firstName?: string;
  birthday?: string;
  phone?: string;
  email?: string;
  address?: string;
}

const store = useStore(storeKey);
const route = useRoute();
const router = useRouter();
const activeTab = ref<PersonalTab>("personal");
const petCount = ref(0);

const navItems: Array<{
  key: Extract<PersonalTab, "personal" | "pet" | "address">;
  eyebrow: string;
  label: string;
  copy: string;
}> = [
  {
    key: "personal",
    eyebrow: "Profile",
    label: "个人资料",
    copy: "头像、姓名、生日和联系方式集中维护。",
  },
  {
    key: "pet",
    eyebrow: "Pets",
    label: "宠物档案",
    copy: "记录每只宠物的基本资料、偏好和护理备注。",
  },
  {
    key: "address",
    eyebrow: "Address",
    label: "配送地址",
    copy: "药品配送、护理用品寄送和家庭联系信息。",
  },
];

// 顶部分段高亮：个人资料下还含头像/姓名/生日/手机/邮箱等编辑子项。
const segActive = (key: string) => {
  if (key === "pet") return activeTab.value === "pet";
  if (key === "address") return activeTab.value === "address";
  return ["personal", "head", "name", "birthday", "phone", "email"].includes(
    activeTab.value
  );
};

const switchTab = (tab: string) => {
  activeTab.value = tab as PersonalTab;
  router.replace({
    query:
      tab === "personal"
        ? {}
        : {
            ...route.query,
            tab,
          },
  });
};

const close = () => {
  switchTab("personal");
};

const submit = (data: SubmitData) => {
  switchTab("personal");

  if (data.field === "userNameParts") {
    store.dispatch("currentUser/updateUserNameParts", {
      userLastName: data.lastName || "",
      userMiddleName: data.middleName || "",
      userFirstName: data.firstName || "",
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
  if (data.field === "userEmail") {
    store.dispatch("currentUser/updateUserField", {
      field: data.field,
      value: data.email,
    });
  }
  if (data.field === "userAddress") {
    store.dispatch("currentUser/updateUserField", {
      field: data.field,
      value: data.address,
    });
  }
};

onMounted(() => {
  if (store.state.ui.personal) {
    void store.dispatch("ui/closePersonal");
  }
  void store.dispatch("currentUser/ensureProfile");

  const tab = route.query.tab;
  if (typeof tab === "string") {
    const allowedTabs: PersonalTab[] = [
      "personal",
      "pet",
      "address",
      "head",
      "name",
      "birthday",
      "phone",
      "email",
    ];
    if (allowedTabs.includes(tab as PersonalTab)) {
      activeTab.value = tab as PersonalTab;
    }
  }
});

watch(
  () => route.query.tab,
  (tab) => {
    if (typeof tab === "string") {
      activeTab.value = tab as PersonalTab;
    } else {
      activeTab.value = "personal";
    }
  }
);
</script>

<style scoped lang="scss">
.personal-page {
  display: grid;
  gap: 14px;
}

.pp-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;
}

.pp-head h2 {
  margin: 0;
  font-size: 20px;
  font-weight: 800;
  letter-spacing: -0.01em;
  color: #1f3a36;
}

.seg {
  display: inline-flex;
  padding: 4px;
  gap: 2px;
  background: #ffffff;
  border: 1px solid #efe7dc;
  border-radius: 12px;
}

.seg__btn {
  height: 36px;
  padding: 0 16px;
  border: 0;
  border-radius: 9px;
  background: transparent;
  color: #6b7d77;
  font-size: 14px;
  font-weight: 700;
  cursor: pointer;
}

.seg__btn--active {
  background: #e7f5f1;
  color: #1f7a6c;
}

.personal-content {
  min-width: 0;
}
</style>
