<template>
  <div class="profile-overview">
    <section class="profile-overview__hero">
      <button class="hero-avatar" @click="emit('switchTab', 'head')">
        <img v-if="avatar" :src="avatar" alt="头像" class="hero-avatar__img" />
        <span v-else>{{ userInitial }}</span>
      </button>
      <div class="hero-copy">
        <p>Profile Snapshot</p>
        <h3>{{ userName }}</h3>
        <span>点选下方卡片可单独编辑对应字段，修改后会自动同步保存。</span>
      </div>
    </section>

    <section class="profile-grid">
      <button class="profile-card" @click="emit('switchTab', 'name')">
        <p>姓名</p>
        <strong>{{ userName }}</strong>
        <span>维护用于预约与订单展示的用户姓名。</span>
      </button>

      <button class="profile-card" @click="emit('switchTab', 'birthday')">
        <p>出生日期</p>
        <strong>{{ userBirthday || "未设置" }}</strong>
        <span>用于资料完整度与必要时的年龄信息确认。</span>
      </button>

      <button class="profile-card" @click="emit('switchTab', 'phone')">
        <p>手机号</p>
        <strong>{{ userPhone || "未设置" }}</strong>
        <span>预约通知、诊疗提醒和客服联系都会优先使用。</span>
      </button>

      <button class="profile-card" @click="emit('switchTab', 'email')">
        <p>邮箱</p>
        <strong>{{ userEmail || "未设置" }}</strong>
        <span>适合接收回执、服务说明与资料备份。</span>
      </button>

      <button
        class="profile-card profile-card--wide"
        @click="emit('switchTab', 'address')"
      >
        <p>家庭地址</p>
        <strong>{{ userAddress || "未设置" }}</strong>
        <span>配送药品、护理用品或建立完整资料时会用到此地址。</span>
      </button>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";

const store = useStore(storeKey);
const emit = defineEmits(["switchTab"]);

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
</script>

<style scoped lang="scss">
.profile-overview {
  display: grid;
  gap: 18px;
}

.profile-overview__hero {
  display: grid;
  grid-template-columns: 120px minmax(0, 1fr);
  gap: 18px;
  padding: 24px;
  border-radius: 28px;
  background: linear-gradient(
    135deg,
    rgba(132, 214, 206, 0.2),
    rgba(243, 197, 155, 0.18)
  );
}

.hero-avatar {
  width: 120px;
  height: 120px;
  display: grid;
  place-items: center;
  border: none;
  border-radius: 32px;
  background: linear-gradient(135deg, #91ddd2, #f0c29b);
  color: #15474a;
  cursor: pointer;
  box-shadow: 0 20px 40px rgba(28, 98, 99, 0.14);
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 42px;
  font-weight: 700;
}

.hero-avatar__img {
  width: 100%;
  height: 100%;
  object-fit: cover;
  border-radius: 32px;
}

.hero-copy {
  display: grid;
  align-content: center;
  gap: 8px;
}

.hero-copy p {
  margin: 0;
  color: #1e8a88;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 12px;
  font-weight: 700;
}

.hero-copy h3 {
  margin: 0;
  font-size: clamp(30px, 3vw, 42px);
  color: #153f42;
}

.hero-copy span {
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.profile-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.profile-card {
  display: grid;
  justify-items: start;
  gap: 10px;
  padding: 22px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.7);
  color: #163f42;
  text-align: left;
  cursor: pointer;
  box-shadow: 0 18px 40px rgba(25, 92, 93, 0.06);
}

.profile-card p {
  margin: 0;
  color: #1d8b89;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.profile-card strong {
  font-size: 22px;
}

.profile-card span {
  color: #607975;
  line-height: 1.8;
  font-size: 14px;
}

.profile-card--wide {
  grid-column: 1 / -1;
}

@media (max-width: 900px) {
  .profile-overview__hero,
  .profile-grid {
    grid-template-columns: 1fr;
  }

  .hero-avatar {
    width: 96px;
    height: 96px;
  }
}
</style>
