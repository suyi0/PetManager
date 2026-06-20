<template>
  <section v-if="profile" class="profile-page">
    <header class="hero-card">
      <div class="hero-copy">
        <p class="eyebrow">医生端 / 用户档案</p>
        <h2>{{ profile.name }}</h2>
        <p class="subcopy">
          聚合展示用户基础信息、宠物摘要与近期诊单记录，方便医生快速接诊。
        </p>
      </div>
      <div class="hero-side">
        <button class="back-link" type="button" @click="goBackToWorkbench">
          返回搜索
        </button>
        <div class="balance-card">
          <span>用户类型</span>
          <strong>{{ profile.type_name || "未设置" }}</strong>
          <em>等级 {{ profile.user_level || 0 }}</em>
        </div>
      </div>
    </header>

    <section class="summary-grid">
      <article class="info-card">
        <small>基础信息</small>
        <ul class="info-list">
          <li>
            <span>用户编号</span>
            <strong>{{ profile.id }}</strong>
          </li>
          <li>
            <span>手机号</span>
            <strong>{{ profile.phone || "未登记" }}</strong>
          </li>
          <li>
            <span>邮箱</span>
            <strong>{{ profile.email || "未登记" }}</strong>
          </li>
          <li>
            <span>生日</span>
            <strong>{{ profile.birthday || "未登记" }}</strong>
          </li>
          <li>
            <span>创建时间</span>
            <strong>{{ profile.created_at || "暂无" }}</strong>
          </li>
        </ul>
      </article>

      <article class="info-card info-card--note">
        <small>用户说明</small>
        <p>{{ profile.user_introduction || "暂无用户说明。" }}</p>
      </article>
    </section>

    <section class="pet-shell">
      <div class="section-head">
        <div>
          <h3>宠物列表</h3>
          <p>点击宠物切换下方诊单记录。</p>
        </div>
      </div>

      <div class="pet-tabs">
        <button
          v-for="pet in profile.pets"
          :key="pet.id"
          type="button"
          class="pet-tab"
          :class="{ 'pet-tab--active': pet.id === selectedPetId }"
          @click="selectedPetId = pet.id"
        >
          <strong>{{ pet.pet_name || "未命名宠物" }}</strong>
          <span>{{ formatPetBrief(pet) }}</span>
        </button>
      </div>

      <div v-if="selectedPet" class="pet-detail">
        <div class="pet-spotlight">
          <small>当前宠物</small>
          <h4>{{ selectedPet.pet_name || "未命名宠物" }}</h4>
          <p>{{ formatPetBrief(selectedPet) }}</p>
        </div>

        <div class="pet-meta">
          <div>
            <span>品类</span>
            <strong>{{ selectedPet.pet_type || "未登记" }}</strong>
          </div>
          <div>
            <span>品种</span>
            <strong>{{ selectedPet.pet_breed || "未登记" }}</strong>
          </div>
          <div>
            <span>诊单数</span>
            <strong>{{ selectedOrders.length }}</strong>
          </div>
        </div>
      </div>
    </section>

    <section class="order-shell">
      <div class="section-head">
        <div>
          <h3>诊单记录</h3>
          <p>先看粗略信息，点击任意记录进入详细页。</p>
        </div>
      </div>

      <div v-if="selectedPet" class="order-summary-banner">
        <strong>{{ selectedPet.pet_name || "未命名宠物" }}</strong>
        <span>当前共 {{ selectedOrders.length }} 条诊单记录</span>
      </div>

      <div class="order-list">
        <div v-if="selectedOrders.length" class="order-head">
          <span>宠物名</span>
          <span>医生</span>
          <span>诊单时间</span>
          <span>诊单金额</span>
        </div>
        <RouterLink
          v-for="item in selectedOrders"
          :key="item.id"
          :to="getOrderDetailPath(item.id)"
          class="order-row"
        >
          <div class="order-cell">
            <label>宠物名</label>
            <strong>{{ item.pet_name || "未登记" }}</strong>
          </div>
          <div class="order-cell">
            <label>医生</label>
            <span>{{ item.doctor_name || "未登记" }}</span>
          </div>
          <div class="order-cell">
            <label>诊单时间</label>
            <span>{{ item.order_data || "暂无" }}</span>
          </div>
          <div class="order-cell order-cell--price">
            <label>诊单金额</label>
            <em>¥{{ item.order_totalprice.toFixed(2) }}</em>
          </div>
        </RouterLink>
        <div v-if="!selectedOrders.length" class="empty-state">
          当前宠物还没有诊单记录。
        </div>
      </div>
    </section>
  </section>

  <section v-else-if="loading" class="empty-page">
    <h2>正在加载用户详情...</h2>
  </section>

  <section v-else class="empty-page">
    <h2>未找到对应用户</h2>
    <button class="back-link" type="button" @click="goBackToWorkbench">
      返回工作台
    </button>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref, watch } from "vue";
import { useRoute, useRouter } from "vue-router";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";
import { DoctorUserProfilePetSummary } from "@/modules/doctor/api/types";

export default defineComponent({
  name: "DoctorUserProfile",
  setup() {
    const store = useStore(storeKey);
    const route = useRoute();
    const router = useRouter();
    const basePath = computed(() => "/doctor");

    const userId = computed(() => Number(route.params.userId));
    const profile = computed(() =>
      Number(store.state.doctor.currentUserProfile?.id) === userId.value
        ? store.state.doctor.currentUserProfile
        : null
    );
    const loading = computed(
      () => store.state.doctor.currentUserProfileMeta.loading
    );
    const selectedPetId = ref<number | null>(null);

    onMounted(() => {
      if (Number.isFinite(userId.value) && userId.value > 0) {
        void store.dispatch("doctor/ensureUserProfile", userId.value);
      }
    });

    watch(
      () => profile.value?.id,
      () => {
        selectedPetId.value = profile.value?.pets[0]?.id ?? null;
      }
    );

    watch(
      () => profile.value?.pets,
      () => {
        if (profile.value?.pets.length && !selectedPetId.value) {
          selectedPetId.value = profile.value.pets[0].id;
        }
      },
      { immediate: true }
    );

    const selectedPet = computed(() => {
      if (!selectedPetId.value) {
        return profile.value?.pets[0] ?? null;
      }

      return (
        profile.value?.pets.find((pet) => pet.id === selectedPetId.value) ??
        null
      );
    });
    const selectedOrders = computed(() => {
      if (!profile.value) {
        return [];
      }

      if (!selectedPet.value) {
        return profile.value.orders;
      }

      return profile.value.orders.filter(
        (order) => order.pet_name === selectedPet.value?.pet_name
      );
    });

    const formatPetBrief = (pet: DoctorUserProfilePetSummary) =>
      [pet.pet_type, pet.pet_breed, pet.pet_sex, pet.pet_age]
        .filter(Boolean)
        .join(" · ") || "暂无宠物摘要";

    const goBackToWorkbench = () => {
      router.push(`${basePath.value}/home`);
    };

    const getOrderDetailPath = (orderId: number) =>
      `${basePath.value}/orders/${orderId}`;

    return {
      profile,
      loading,
      selectedPetId,
      selectedPet,
      selectedOrders,
      formatPetBrief,
      goBackToWorkbench,
      getOrderDetailPath,
    };
  },
});
</script>

<style scoped>
.profile-page {
  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr);
  gap: 20px;
  height: var(--doctor-page-card-height, 860px);
  min-width: 0;
  overflow: hidden;
}

.hero-card,
.info-card,
.pet-shell,
.order-shell,
.empty-page {
  border: 1px solid rgba(150, 183, 173, 0.24);
  border-radius: 30px;
  background: linear-gradient(180deg, rgba(255, 252, 246, 0.96), #f5fbf8);
  box-shadow: 0 20px 40px rgba(42, 78, 75, 0.07);
}

.hero-card {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 250px;
  gap: 18px;
  padding: 28px;
}

.eyebrow {
  margin: 0 0 8px;
  font-size: 12px;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: #7d958e;
}

.hero-copy h2,
.hero-copy p,
.section-head h3,
.section-head p,
.pet-spotlight h4,
.empty-page h2 {
  margin: 0;
}

.hero-copy h2 {
  font-size: 34px;
  color: #173d42;
}

.subcopy {
  margin-top: 10px;
  max-width: 560px;
  color: #67827c;
  line-height: 1.7;
}

.hero-side {
  display: grid;
  gap: 14px;
  justify-items: end;
}

.back-link {
  border: 1px solid rgba(128, 168, 156, 0.34);
  border-radius: 999px;
  padding: 10px 16px;
  background: linear-gradient(135deg, #f8fffc, #e8f3ee);
  color: #28555b;
  font-weight: 700;
  cursor: pointer;
}

.balance-card {
  width: 100%;
  display: grid;
  gap: 8px;
  padding: 18px;
  border-radius: 22px;
  background: linear-gradient(135deg, #214f57, #255d65 55%, #31756f);
  color: #f6fffb;
}

.balance-card span,
.balance-card em {
  font-style: normal;
  opacity: 0.82;
}

.balance-card strong {
  font-size: 32px;
}

.summary-grid {
  display: grid;
  grid-template-columns: 1.15fr 0.85fr;
  gap: 18px;
}

.info-card {
  padding: 22px;
}

.info-card small,
.pet-spotlight small {
  color: #7d958f;
  font-size: 12px;
}

.info-list {
  list-style: none;
  margin: 18px 0 0;
  padding: 0;
  display: grid;
  gap: 14px;
}

.info-list li {
  display: grid;
  gap: 5px;
  padding-bottom: 14px;
  border-bottom: 1px solid rgba(225, 236, 231, 0.92);
}

.info-list span,
.pet-meta span,
.order-row span {
  color: #718984;
  font-size: 12px;
}

.info-list strong,
.pet-meta strong {
  color: #1b4044;
}

.info-card--note p {
  margin: 16px 0 0;
  color: #476763;
  line-height: 1.8;
}

.pet-shell,
.order-shell,
.empty-page {
  padding: 24px;
  min-height: 0;
  overflow: visible;
}

.section-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 16px;
}

.section-head h3 {
  color: #183d42;
}

.section-head p {
  margin-top: 6px;
  color: #6f8782;
}

.pet-tabs {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
}

.pet-tab {
  min-width: 170px;
  display: grid;
  gap: 6px;
  padding: 14px 16px;
  border: 1px solid rgba(138, 175, 164, 0.22);
  border-radius: 20px;
  background: #fbfffd;
  text-align: left;
  cursor: pointer;
  transition: transform 0.2s ease, box-shadow 0.2s ease;
}

.pet-tab--active {
  background: linear-gradient(135deg, #eaf7f1, #d9ece5 70%, #fff9f0);
  box-shadow: 0 16px 28px rgba(59, 97, 92, 0.12);
  transform: translateY(-1px);
}

.pet-tab strong,
.order-row strong {
  color: #1a4044;
}

.pet-tab span {
  color: #6f8782;
  font-size: 12px;
}

.pet-detail {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 1fr;
  gap: 16px;
  margin-top: 18px;
}

.pet-spotlight {
  padding: 22px;
  border-radius: 24px;
  background: linear-gradient(135deg, #1f5057, #255d64);
  color: #f5fffb;
}

.pet-spotlight h4 {
  margin-top: 10px;
  font-size: 30px;
}

.pet-spotlight p {
  margin: 8px 0 0;
  color: rgba(255, 255, 255, 0.82);
}

.pet-meta {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
}

.pet-meta div {
  display: grid;
  gap: 8px;
  align-content: center;
  padding: 18px;
  border-radius: 22px;
  background: rgba(255, 255, 255, 0.72);
  border: 1px solid rgba(220, 234, 229, 0.95);
}

.order-list {
  display: grid;
  gap: 12px;
  min-width: 0;
}

.order-summary-banner {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 16px;
  padding: 16px 18px;
  border-radius: 20px;
  background: linear-gradient(135deg, #eff9f4, #dfeee7 72%, #fff8ef);
  border: 1px solid rgba(173, 203, 192, 0.36);
}

.order-summary-banner strong {
  color: #173d42;
  font-size: 18px;
}

.order-summary-banner span {
  color: #5f7a74;
  font-size: 13px;
}

.order-head,
.order-row {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  align-items: center;
  gap: 14px;
}

.order-head {
  padding: 0 20px;
}

.order-head span {
  color: #7c948e;
  font-size: 12px;
  font-weight: 700;
}

.order-row {
  width: 100%;
  padding: 18px 20px;
  border: 1px solid rgba(218, 232, 227, 0.92);
  border-radius: 22px;
  background: linear-gradient(180deg, #ffffff, #f4fbf8);
  text-align: left;
  cursor: pointer;
  text-decoration: none;
  transition: transform 0.2s ease, box-shadow 0.2s ease;
  box-sizing: border-box;
  overflow: hidden;
}

.order-row:hover {
  transform: translateY(-2px);
  box-shadow: 0 18px 28px rgba(57, 98, 93, 0.08);
}

.order-cell {
  min-width: 0;
  display: grid;
  gap: 6px;
}

.order-cell label {
  color: #7c948e;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.04em;
}

.order-cell span,
.order-cell strong,
.order-cell em {
  min-width: 0;
  overflow-wrap: anywhere;
}

.order-cell--price {
  justify-items: end;
}

.order-cell--price em {
  font-style: normal;
  color: #1f6159;
  font-weight: 700;
  font-size: 16px;
}

.empty-state,
.empty-page {
  color: #6f8782;
}

.empty-page {
  display: grid;
  gap: 16px;
  place-items: center;
  min-height: 360px;
}

@media (max-width: 980px) {
  .hero-card,
  .summary-grid,
  .pet-detail {
    grid-template-columns: 1fr;
  }

  .hero-side {
    justify-items: stretch;
  }

  .pet-meta {
    grid-template-columns: 1fr;
  }

  .order-summary-banner,
  .order-head,
  .order-row {
    grid-template-columns: 1fr;
  }

  .order-summary-banner {
    align-items: flex-start;
    justify-content: flex-start;
    flex-direction: column;
  }

  .order-head {
    display: none;
  }

  .order-cell--price {
    justify-items: start;
  }
}
</style>
