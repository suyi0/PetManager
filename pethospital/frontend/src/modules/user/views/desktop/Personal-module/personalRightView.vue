<template>
  <div class="profile-overview">
    <section class="profile-overview__hero">
      <div class="hero-copy">
        <p>Profile Snapshot</p>
        <h3>{{ userName }}</h3>
        <span>
          当前资料会同步用于预约卡片、订单联系人与服务通知，建议把常用信息保持为最新。
        </span>

        <div class="hero-tags">
          <span>{{ userPhone || "未绑定手机" }}</span>
          <span>{{ userEmail || "未绑定邮箱" }}</span>
          <span>{{ userBirthday || "生日待补充" }}</span>
        </div>
      </div>

      <button class="hero-avatar" @click="emit('switchTab', 'head')">
        <img v-if="avatar" :src="avatar" alt="头像" class="hero-avatar__img" />
        <span v-else>{{ userInitial }}</span>
      </button>
    </section>

    <section class="profile-grid">
      <button
        class="profile-card profile-card--accent"
        @click="emit('switchTab', 'name')"
      >
        <div class="profile-card__eyebrow">Identity</div>
        <strong>{{ userName }}</strong>
        <span>维护用户姓名，预约单和订单展示会直接使用这里的名字。</span>
      </button>

      <button class="profile-card" @click="emit('switchTab', 'birthday')">
        <div class="profile-card__eyebrow">Birthday</div>
        <strong>{{ userBirthday || "未设置" }}</strong>
        <span>完善生日信息后，资料完整度和服务提醒会更准确。</span>
      </button>

      <button class="profile-card" @click="emit('switchTab', 'phone')">
        <div class="profile-card__eyebrow">Mobile</div>
        <strong>{{ userPhone || "未设置" }}</strong>
        <span>预约通知、医生回访与紧急联系优先使用当前手机号。</span>
      </button>

      <button class="profile-card" @click="emit('switchTab', 'email')">
        <div class="profile-card__eyebrow">Mail</div>
        <strong>{{ userEmail || "未设置" }}</strong>
        <span>适合接收账单回执、护理说明与系统通知。</span>
      </button>

      <button
        class="profile-card profile-card--wide"
        @click="emit('switchTab', 'address')"
      >
        <div class="profile-card__eyebrow">Address</div>
        <strong>{{ userAddress || "未设置" }}</strong>
        <span>配送药品、寄送护理用品和确认服务范围时会直接复用此地址。</span>
      </button>
    </section>
  </div>
</template>

<script setup lang="ts">
import { computed } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";

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
  grid-template-columns: minmax(0, 1fr) 150px;
  gap: 18px;
  padding: 26px;
  border-radius: 16px;
  background: linear-gradient(
    135deg,
    rgba(132, 214, 206, 0.22),
    rgba(255, 217, 176, 0.18)
  );
  border: 1px solid rgba(47, 158, 143, 0.08);
}

.hero-copy {
  display: grid;
  gap: 10px;
  align-content: center;
}

.hero-copy p,
.profile-card__eyebrow {
  margin: 0;
  color: #2f9e8f;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  font-size: 11px;
  font-weight: 700;
}

.hero-copy h3 {
  margin: 0;
  font-size: clamp(32px, 3vw, 46px);
  line-height: 1.05;
  color: #1f3a36;
}

.hero-copy span {
  color: #6b7d77;
  line-height: 1.8;
  font-size: 14px;
}

.hero-tags {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
  margin-top: 4px;
}

.hero-tags span {
  padding: 8px 12px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.68);
  color: #1f3a36;
  font-size: 12px;
  line-height: 1;
}

.hero-avatar {
  width: 150px;
  height: 150px;
  justify-self: end;
  display: grid;
  place-items: center;
  border: none;
  border-radius: 40px;
  background: linear-gradient(135deg, #cfe7e1, #ffd9b0);
  color: #1f3a36;
  cursor: pointer;
  box-shadow: 0 20px 40px rgba(47, 158, 143, 0.14);
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 54px;
  font-weight: 700;
}

.hero-avatar__img {
  width: 100%;
  height: 100%;
  object-fit: cover;
  border-radius: 40px;
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
  border: 1px solid rgba(47, 158, 143, 0.08);
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.74);
  color: #1f3a36;
  text-align: left;
  cursor: pointer;
  box-shadow: 0 18px 40px rgba(47, 158, 143, 0.06);
  transition: transform 0.2s ease, box-shadow 0.2s ease, border-color 0.2s ease;
}

.profile-card:hover {
  transform: translateY(-2px);
  border-color: rgba(47, 158, 143, 0.12);
  box-shadow: 0 22px 44px rgba(47, 158, 143, 0.08);
}

.profile-card--accent {
  background: linear-gradient(
    135deg,
    rgba(255, 248, 237, 0.96),
    rgba(239, 251, 248, 0.96)
  );
}

.profile-card strong {
  font-size: 24px;
  line-height: 1.2;
}

.profile-card span {
  color: #6b7d77;
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
    width: 110px;
    height: 110px;
    justify-self: start;
    border-radius: 16px;
    font-size: 42px;
  }

  .hero-avatar__img {
    border-radius: 16px;
  }
}
</style>
