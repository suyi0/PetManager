<template>
  <div class="boss-shell">
    <aside class="boss-sidebar">
      <div class="boss-sidebar__tag">Boss Portal</div>
      <h1 class="boss-sidebar__title">股份中枢</h1>
      <p class="boss-sidebar__subtitle">
        从总份额、剩余份额到股东分布，一屏看清当前股权结构。
      </p>

      <nav class="boss-nav">
        <RouterLink :to="`${routePrefix}/overview`">股份概览</RouterLink>
      </nav>
    </aside>

    <main class="boss-content">
      <header class="boss-topbar">
        <div>
          <h2>公司股份总览</h2>
        </div>
      </header>
      <RouterView />
    </main>
  </div>
</template>

<script lang="ts">
import { computed, defineComponent } from "vue";
import { useRoute } from "vue-router";

export default defineComponent({
  name: "BossLayout",
  setup() {
    const route = useRoute();

    const routePrefix = computed(() =>
      route.path.startsWith("/preview/boss") ? "/preview/boss" : "/boss"
    );

    return {
      routePrefix,
    };
  },
});
</script>

<style scoped>
.boss-shell {
  display: grid;
  grid-template-columns: 290px 1fr;
  min-height: 100vh;
  background: radial-gradient(
      circle at top right,
      rgba(255, 241, 220, 0.95),
      transparent 28%
    ),
    linear-gradient(135deg, #f8f2e8 0%, #efe3cf 48%, #e6d6bc 100%);
  color: #261b13;
  font-family: "PingFang SC", "Segoe UI", sans-serif;
}

.boss-sidebar {
  position: sticky;
  top: 0;
  height: 100vh;
  padding: 34px 24px;
  display: flex;
  flex-direction: column;
  gap: 18px;
  border-right: 1px solid rgba(90, 66, 42, 0.12);
  background: linear-gradient(
    180deg,
    rgba(255, 252, 246, 0.92),
    rgba(248, 240, 224, 0.92)
  );
  backdrop-filter: blur(14px);
}

.boss-sidebar__tag {
  width: fit-content;
  padding: 8px 12px;
  border-radius: 999px;
  background: #f4e6ce;
  color: #896848;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.boss-sidebar__title {
  margin: 0;
  font-family: Georgia, serif;
  font-size: 42px;
  line-height: 1;
}

.boss-sidebar__subtitle {
  margin: 0;
  color: #7c6753;
  line-height: 1.7;
  font-size: 14px;
}

.boss-nav {
  display: grid;
  gap: 10px;
  margin-top: 14px;
}

.boss-nav a {
  padding: 14px 16px;
  border-radius: 16px;
  color: #5c4630;
  text-decoration: none;
  background: rgba(255, 255, 255, 0.52);
  font-weight: 700;
}

.boss-nav a.router-link-active {
  background: linear-gradient(135deg, #fff8ef, #f5e5cc);
  box-shadow: 0 12px 28px rgba(101, 76, 49, 0.12);
}

.boss-content {
  padding: 28px 32px 36px;
}

.boss-topbar {
  margin-bottom: 20px;
}

.boss-topbar h2 {
  margin: 0 0 6px;
  font-size: 28px;
}

.boss-topbar span {
  color: #7c6753;
  font-size: 14px;
}

@media (max-width: 1080px) {
  .boss-shell {
    grid-template-columns: 1fr;
  }

  .boss-sidebar {
    position: relative;
    height: auto;
  }
}
</style>
