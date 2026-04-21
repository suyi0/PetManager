<template>
  <section v-if="order" class="detail-page">
    <header class="detail-hero">
      <div>
        <p class="eyebrow">医生端 / 诊单详情</p>
        <h2>{{ order.petName }} · {{ order.id }}</h2>
        <p class="subcopy">查看本次接诊的基础信息、诊断结论和用药明细。</p>
      </div>
      <button class="back-link" type="button" @click="goBack">
        {{ backLabel }}
      </button>
    </header>

    <section class="detail-grid">
      <article
        class="detail-card detail-card--deep-green"
        :class="{
          'detail-card detail-card--deep-red': order.status === '已取消',
          'detail-card detail-card--deep-yellow': order.status === '待付款',
        }"
      >
        <small>本次接诊</small>
        <strong>¥{{ order.totalFee.toFixed(2) }}</strong>
        <span>{{ order.status }} · {{ order.createdAt }}</span>
      </article>

      <article class="detail-card">
        <small>主人</small>
        <strong>{{ order.ownerName }}</strong>
        <span>{{ profile?.phone || "未登记手机号" }}</span>
      </article>

      <article class="detail-card">
        <small>主治医生</small>
        <strong>{{ order.doctorName }}</strong>
        <span>{{ order.petName }} · {{ pet?.breed || "宠物档案" }}</span>
      </article>
    </section>

    <section class="content-grid">
      <article class="panel">
        <div class="panel-head">
          <h3>粗略信息</h3>
        </div>
        <div class="fact-list">
          <div>
            <span>主人</span>
            <strong>{{ order.ownerName }}</strong>
          </div>
          <div>
            <span>宠物名</span>
            <strong>{{ order.petName }}</strong>
          </div>
          <div>
            <span>诊单时间</span>
            <strong>{{ order.createdAt }}</strong>
          </div>
          <div>
            <span>诊单金额</span>
            <strong>¥{{ order.totalFee.toFixed(2) }}</strong>
          </div>
        </div>
      </article>

      <article class="panel">
        <div class="panel-head">
          <h3>诊断记录</h3>
        </div>
        <div class="text-block">
          <label>主诉</label>
          <p>{{ order.symptom }}</p>
        </div>
        <div class="text-block">
          <label>诊断</label>
          <p>{{ order.diagnosis }}</p>
        </div>
        <div class="text-block">
          <label>医嘱备注</label>
          <p>{{ order.remark }}</p>
        </div>
      </article>
    </section>

    <section class="panel">
      <div class="panel-head">
        <h3>用药清单</h3>
      </div>
      <div class="medicine-list">
        <div
          v-for="item in order.medicines"
          :key="item.id"
          class="medicine-card"
        >
          <strong>{{ item.name }}</strong>
          <span>{{ item.dosage }}</span>
          <em>x{{ item.quantity }}</em>
          <b>¥{{ item.price.toFixed(2) }}</b>
        </div>
      </div>
    </section>
  </section>

  <section v-else class="empty-page">
    <h2>未找到诊单详情</h2>
    <button class="back-link" type="button" @click="goBackToWorkbench">
      返回工作台
    </button>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted } from "vue";
import { useRoute, useRouter } from "vue-router";
import { useStore } from "vuex";
import { storeKey } from "@/store/appStore";

export default defineComponent({
  name: "DoctorOrderDetail",
  setup() {
    const store = useStore(storeKey);
    const route = useRoute();
    const router = useRouter();
    const basePath = computed(() =>
      route.path.startsWith("/preview/doctor") ? "/preview/doctor" : "/doctor"
    );
    const orderId = computed(() => String(route.params.orderId ?? ""));

    /**
     * 诊单详情依赖用户档案缓存定位具体订单。
     */
    const loadDoctorUserProfiles = async () => {
      await store.dispatch("doctor/ensureUserProfiles");
    };

    /**
     * 获取当前用户的档案
     */
    const profile = computed(() =>
      store.state.doctor.userProfiles.find((item) =>
        item.orders.some((currentOrder) => currentOrder.id === orderId.value)
      )
    );

    /**
     * 根据订单ID从当前用户档案中找到对应的订单详情数据，如果没有找到则返回undefined
     */
    const order = computed(() =>
      profile.value?.orders.find((item) => item.id === orderId.value)
    );

    /**
     * 根据订单ID从当前用户档案中找到对应的宠物档案数据，如果没有找到则返回undefined
     */
    const pet = computed(() =>
      profile.value?.pets.find((item) => item.id === order.value?.petId)
    );

    const fromRecords = computed(() => route.query.from === "records");
    const backLabel = computed(() =>
      fromRecords.value ? "返回订单记录" : "返回用户档案"
    );

    const goBackToWorkbench = () => {
      router.push(`${basePath.value}/home`);
    };

    const goBack = () => {
      if (fromRecords.value) {
        router.push(`${basePath.value}/order-records`);
        return;
      }

      if (profile.value) {
        router.push(`${basePath.value}/users/${profile.value.id}`);
        return;
      }

      goBackToWorkbench();
    };

    onMounted(() => {
      void loadDoctorUserProfiles();
    });

    return {
      profile,
      order,
      pet,
      backLabel,
      goBack,
      goBackToWorkbench,
    };
  },
});
</script>

<style scoped>
.detail-page {
  display: grid;
  gap: 20px;
}

.detail-hero,
.detail-card,
.panel,
.empty-page {
  border: 1px solid rgba(151, 184, 174, 0.24);
  border-radius: 28px;
  background: linear-gradient(180deg, rgba(255, 252, 246, 0.96), #f5fbf8);
  box-shadow: 0 20px 40px rgba(43, 78, 75, 0.07);
}

.detail-hero {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 14px;
  padding: 28px;
}

.eyebrow {
  margin: 0 0 8px;
  color: #7d958e;
  font-size: 12px;
  letter-spacing: 0.18em;
  text-transform: uppercase;
}

.detail-hero h2,
.detail-hero p,
.panel-head h3,
.empty-page h2 {
  margin: 0;
}

.detail-hero h2 {
  color: #173d42;
  font-size: 34px;
}

.subcopy {
  margin-top: 10px;
  color: #6d8681;
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

.detail-grid,
.content-grid {
  display: grid;
  gap: 18px;
}

.detail-grid {
  grid-template-columns: repeat(3, 1fr);
}

.content-grid {
  grid-template-columns: 0.95fr 1.05fr;
}

.detail-card,
.panel {
  padding: 22px;
}

.fact-list span,
.text-block label,
.medicine-card span {
  color: #6f8782;
}

.detail-card strong {
  display: block;
  margin: 10px 0 8px;
  color: #183d42;
  font-size: 24px;
}

.detail-card--deep-green {
  background: linear-gradient(135deg, #16ef7f, #83c683);
}
.detail-card--deep-yellow {
  background: linear-gradient(135deg, #e9b311, #f9e0a1);
}
.detail-card--deep-red {
  background: linear-gradient(135deg, #f81212, #bd7373);
}

.detail-card--deep small,
.detail-card--deep strong,
.detail-card--deep span {
  color: #f5fffb;
}

.panel-head {
  margin-bottom: 16px;
}

.panel-head h3 {
  color: #183d42;
}

.fact-list {
  display: grid;
  gap: 14px;
}

.fact-list div,
.text-block,
.medicine-card {
  display: grid;
  gap: 6px;
  padding: 16px;
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.72);
  border: 1px solid rgba(223, 236, 232, 0.95);
}

.fact-list strong,
.medicine-card strong {
  color: #1a4044;
}

.text-block p {
  margin: 0;
  color: #466762;
  line-height: 1.8;
}

.medicine-list {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 12px;
}

.medicine-card em,
.medicine-card b {
  font-style: normal;
}

.medicine-card b {
  color: #1f6159;
}

.empty-page {
  min-height: 360px;
  display: grid;
  gap: 16px;
  place-items: center;
}

@media (max-width: 980px) {
  .detail-hero,
  .detail-grid,
  .content-grid,
  .medicine-list {
    grid-template-columns: 1fr;
  }

  .detail-hero {
    flex-direction: column;
  }
}
</style>
