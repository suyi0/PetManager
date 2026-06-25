<template>
  <div class="pc-card">
    <div class="pc-profile">
      <button
        class="pc-avatar"
        type="button"
        title="更换头像"
        @click="emit('switchTab', 'head')"
      >
        <img v-if="avatar" :src="avatar" alt="头像" />
        <span v-else>{{ userInitial }}</span>
      </button>
      <div class="pc-profile__id">
        <div class="pc-profile__name">
          {{ userName }}
          <span v-if="roleBadge" class="pc-role-badge">{{ roleBadge }}</span>
        </div>
        <div class="pc-profile__meta">
          <span
            >资料完整度 <b>{{ completeness }}%</b></span
          >
          <span
            >手机 <b>{{ userPhone ? "已绑定" : "未绑定" }}</b></span
          >
          <span
            >邮箱 <b>{{ userEmail ? "已绑定" : "未绑定" }}</b></span
          >
        </div>
      </div>
      <button
        type="button"
        class="pc-btn pc-btn--ghost"
        style="margin-left: auto"
        @click="emit('switchTab', 'head')"
      >
        更换头像
      </button>
    </div>

    <div class="pc-rows">
      <button class="pc-row" type="button" @click="emit('switchTab', 'name')">
        <span class="pc-row__k">姓名</span>
        <span class="pc-row__v">{{ userName }}</span>
        <span class="pc-row__act">编辑 ›</span>
      </button>

      <button
        class="pc-row"
        type="button"
        @click="emit('switchTab', 'birthday')"
      >
        <span class="pc-row__k">生日</span>
        <span class="pc-row__v" :class="{ 'pc-row__v--empty': !userBirthday }">
          {{ userBirthday || "未设置" }}
        </span>
        <span class="pc-row__act">{{ userBirthday ? "编辑" : "补充" }} ›</span>
      </button>

      <button class="pc-row" type="button" @click="emit('switchTab', 'phone')">
        <span class="pc-row__k">手机号</span>
        <span class="pc-row__v" :class="{ 'pc-row__v--empty': !userPhone }">
          {{ userPhone || "未设置" }}
          <span v-if="userPhone" class="pc-pill pc-pill--ok">已绑定</span>
        </span>
        <span class="pc-row__act">{{ userPhone ? "修改" : "绑定" }} ›</span>
      </button>

      <button class="pc-row" type="button" @click="emit('switchTab', 'email')">
        <span class="pc-row__k">邮箱</span>
        <span class="pc-row__v" :class="{ 'pc-row__v--empty': !userEmail }">
          {{ userEmail || "未设置" }}
          <span v-if="!userEmail" class="pc-pill pc-pill--warn">建议补充</span>
        </span>
        <span class="pc-row__act">{{ userEmail ? "修改" : "绑定" }} ›</span>
      </button>

      <button
        class="pc-row"
        type="button"
        @click="emit('switchTab', 'address')"
      >
        <span class="pc-row__k">配送地址</span>
        <span class="pc-row__v" :class="{ 'pc-row__v--empty': !userAddress }">
          {{ userAddress || "未设置" }}
        </span>
        <span class="pc-row__act">{{ userAddress ? "编辑" : "添加" }} ›</span>
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";

const store = useStore(storeKey);
const emit = defineEmits(["switchTab"]);

defineProps<{ roleBadge?: string }>();

const userName = computed(
  () => store.getters["auth/formattedUserName"] || "未设置姓名"
);
const userBirthday = computed(() => store.state.currentUser.userBirthday || "");
const userPhone = computed(() => store.state.currentUser.userPhone || "");
const userEmail = computed(() => store.state.currentUser.userEmail || "");
const userAddress = computed(() => store.state.currentUser.userAddress || "");
const avatar = computed(() => store.state.currentUser.userHeadImage || "");
const userInitial = computed(() =>
  String(userName.value || "U")
    .trim()
    .charAt(0)
    .toUpperCase()
);

const completeness = computed(() => {
  const fields = [
    store.getters["auth/formattedUserName"],
    userBirthday.value,
    userPhone.value,
    userEmail.value,
    userAddress.value,
  ];
  const filled = fields.filter((v) => String(v || "").trim()).length;
  return Math.round((filled / fields.length) * 100);
});
</script>
