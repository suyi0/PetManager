<template>
  <section class="personal-page">
    <aside class="personal-sidebar">
      <div class="personal-sidebar__hero">
        <p class="personal-sidebar__eyebrow">Pet Passport</p>
        <h2>个人资料与宠物档案</h2>
        <span>
          把主人资料、联络方式和宠物信息整理在同一套轻盈面板里，日常维护更直观。
        </span>
      </div>

      <div class="personal-sidebar__metrics">
        <article class="sidebar-metric">
          <strong>{{ completionScore }}%</strong>
          <span>资料完成度</span>
        </article>
        <article class="sidebar-metric">
          <strong>{{ petCount }}</strong>
          <span>宠物档案数</span>
        </article>
      </div>

      <div class="personal-sidebar__tabs">
        <button
          v-for="item in navItems"
          :key="item.key"
          class="personal-sidebar__item"
          :class="{
            'personal-sidebar__item--active': activeTab === item.key,
          }"
          @click="switchTab(item.key)"
        >
          <small>{{ item.eyebrow }}</small>
          <strong>{{ item.label }}</strong>
          <span>{{ item.copy }}</span>
        </button>
      </div>
    </aside>

    <div class="personal-stage">
      <section class="personal-stage__header">
        <div>
          <p>{{ panelEyebrow }}</p>
          <h3>{{ panelTitle }}</h3>
        </div>
        <span>{{ panelDescription }}</span>
      </section>

      <section class="personal-stage__content">
        <PersonalRightView
          v-if="activeTab === 'personal'"
          @switchTab="switchTab"
        />
        <PetProfilesView v-else-if="activeTab === 'pet'" />
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
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, onMounted, ref, watch } from "vue";
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
  birthday?: string;
  phone?: string;
  email?: string;
  address?: string;
}

const store = useStore(storeKey);
const route = useRoute();
const router = useRouter();
const activeTab = ref<PersonalTab>("personal");
const petCount = computed(() => store.state.userPortal.petProfiles.length);

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

const completionScore = computed(() => {
  const values = [
    store.state.currentUser.userName,
    store.state.currentUser.userPhone,
    store.state.currentUser.userEmail,
    store.state.currentUser.userBirthday,
    store.state.currentUser.userAddress,
    store.state.currentUser.userHeadImage,
  ];
  const completed = values.filter(
    (value) => typeof value === "string" && value.trim().length > 0
  ).length;

  return Math.round((completed / values.length) * 100);
});

const panelTitle = computed(() => {
  switch (activeTab.value) {
    case "pet":
      return "宠物档案面板";
    case "address":
      return "地址与配送信息";
    case "head":
      return "更新头像";
    case "name":
      return "更新姓名";
    case "birthday":
      return "更新生日";
    case "phone":
      return "更新手机号";
    case "email":
      return "更新邮箱";
    default:
      return "用户资料总览";
  }
});

const panelEyebrow = computed(() => {
  switch (activeTab.value) {
    case "pet":
      return "Pet Desk";
    case "address":
      return "Delivery";
    case "head":
    case "name":
    case "birthday":
    case "phone":
    case "email":
      return "Edit Field";
    default:
      return "Profile Studio";
  }
});

const panelDescription = computed(() => {
  switch (activeTab.value) {
    case "pet":
      return "先整理宠物基础资料，再把护理提醒和偏好记录进去，预约时会更顺手。";
    case "address":
      return "统一维护常用地址，后续配送与资料确认都可以直接复用。";
    case "head":
    case "name":
    case "birthday":
    case "phone":
    case "email":
      return "当前正在编辑单项资料，保存后会自动回到主面板。";
    default:
      return "信息被拆成更清晰的资料卡，点击任意卡片即可进入对应编辑项。";
  }
});

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
    store.commit("ui/closePersonal");
  }
  /**
   * 个人资料和宠物档案在进入个人中心时一起预热，页面切换时优先复用缓存。
   */
  void Promise.all([
    store.dispatch("currentUser/ensureProfile"),
    store.dispatch("userPortal/ensurePetProfiles"),
  ]);

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
  grid-template-columns: 320px minmax(0, 1fr);
  gap: 22px;
  min-height: calc(100vh - 170px);
}

.personal-sidebar,
.personal-stage {
  position: relative;
  overflow: hidden;
  border-radius: 34px;
  border: 1px solid rgba(21, 91, 92, 0.1);
  background: linear-gradient(
      180deg,
      rgba(255, 251, 245, 0.96),
      rgba(255, 255, 255, 0.9)
    ),
    rgba(255, 255, 255, 0.82);
  box-shadow: 0 28px 60px rgba(24, 90, 91, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 0.55);
}

.personal-sidebar::before,
.personal-stage::before {
  content: "";
  position: absolute;
  inset: 0;
  background: radial-gradient(
      circle at top left,
      rgba(147, 224, 214, 0.18),
      transparent 34%
    ),
    radial-gradient(
      circle at bottom right,
      rgba(241, 193, 149, 0.16),
      transparent 32%
    );
  pointer-events: none;
}

.personal-sidebar {
  position: sticky;
  top: 128px;
  align-self: start;
  display: grid;
  gap: 18px;
  padding: 24px;
}

.personal-sidebar__hero,
.personal-sidebar__metrics,
.personal-sidebar__tabs,
.personal-stage__header,
.personal-stage__content {
  position: relative;
  z-index: 1;
}

.personal-sidebar__hero {
  display: grid;
  gap: 10px;
}

.personal-sidebar__eyebrow,
.personal-stage__header p {
  margin: 0;
  color: #1f8e89;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.personal-sidebar__hero h2,
.personal-stage__header h3 {
  margin: 0;
  color: #133f42;
  font-size: clamp(28px, 2.7vw, 38px);
  line-height: 1.1;
}

.personal-sidebar__hero span,
.personal-stage__header span {
  color: #5d7974;
  font-size: 14px;
  line-height: 1.8;
}

.personal-sidebar__metrics {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.sidebar-metric {
  display: grid;
  gap: 6px;
  padding: 16px 18px;
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.72);
  border: 1px solid rgba(20, 82, 84, 0.08);
  box-shadow: 0 16px 36px rgba(20, 82, 84, 0.06);
}

.sidebar-metric strong {
  color: #144346;
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 32px;
  line-height: 1;
}

.sidebar-metric span {
  color: #6b8481;
  font-size: 13px;
}

.personal-sidebar__tabs {
  display: grid;
  gap: 12px;
}

.personal-sidebar__item {
  display: grid;
  gap: 4px;
  justify-items: start;
  padding: 16px 18px;
  border: 1px solid transparent;
  border-radius: 22px;
  background: rgba(255, 255, 255, 0.62);
  color: #163f42;
  text-align: left;
  cursor: pointer;
  transition: transform 0.2s ease, border-color 0.2s ease, box-shadow 0.2s ease;
}

.personal-sidebar__item:hover {
  transform: translateY(-1px);
  border-color: rgba(29, 134, 135, 0.12);
}

.personal-sidebar__item small {
  color: #1f8e89;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 10px;
  font-weight: 700;
}

.personal-sidebar__item strong {
  font-size: 18px;
}

.personal-sidebar__item span {
  color: #617975;
  font-size: 13px;
  line-height: 1.7;
}

.personal-sidebar__item--active {
  border-color: rgba(29, 134, 135, 0.16);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.3),
    rgba(243, 197, 155, 0.22)
  );
  box-shadow: 0 18px 34px rgba(28, 98, 99, 0.12);
}

.personal-stage {
  display: grid;
  gap: 18px;
  padding: 28px;
}

.personal-stage__header {
  display: flex;
  align-items: end;
  justify-content: space-between;
  gap: 18px;
  padding-bottom: 16px;
  border-bottom: 1px solid rgba(21, 91, 92, 0.08);
}

.personal-stage__content {
  min-height: 560px;
}

@media (max-width: 1080px) {
  .personal-page {
    grid-template-columns: 1fr;
  }

  .personal-sidebar {
    position: static;
  }

  .personal-stage__header {
    flex-direction: column;
    align-items: start;
  }
}

@media (max-width: 720px) {
  .personal-page {
    gap: 16px;
  }

  .personal-sidebar,
  .personal-stage {
    padding: 18px;
    border-radius: 26px;
  }

  .personal-sidebar__metrics {
    grid-template-columns: 1fr;
  }

  .personal-stage__content {
    min-height: auto;
  }
}
</style>
