<template>
  <section class="page">
    <section class="panel">
      <div class="panel-head">
        <div>
          <h3>操作日志</h3>
          <span>新增 / 更新 / 删除轨迹</span>
        </div>
        <div class="status-pill">共 {{ visibleLogs.length }} 条</div>
      </div>

      <div class="toolbar">
        <input
          v-model.trim="keywordInput"
          type="text"
          placeholder="按操作、说明或标签查询"
          @keyup.enter="applySearch"
        />
      </div>

      <div class="log-list">
        <article
          v-for="entry in visibleLogs"
          :key="entry.time + entry.title"
          class="log-item"
        >
          <time>{{ entry.time }}</time>
          <div>
            <strong>{{ entry.title }}</strong>
            <p>{{ entry.description }}</p>
          </div>
          <i>{{ entry.tag }}</i>
        </article>
        <div v-if="visibleLogs.length === 0" class="empty-state">
          当前没有匹配的操作日志
        </div>
      </div>
    </section>
  </section>
</template>

<script lang="ts">
import { computed, defineComponent, onMounted, ref } from "vue";
import { useStore } from "vuex";
import { storeKey } from "@/app/store";

export default defineComponent({
  name: "WarehouseAdminLogs",
  setup() {
    const store = useStore(storeKey);
    const keywordInput = ref("");
    const keyword = ref("");
    const logs = computed(() => store.state.warehouseAdmin.operationLogs);
    const visibleLogs = computed(() => {
      const query = keyword.value.toLowerCase();
      if (!query) {
        return logs.value;
      }

      return logs.value.filter((entry) =>
        [entry.title, entry.description, entry.tag, entry.time]
          .join(" ")
          .toLowerCase()
          .includes(query)
      );
    });

    onMounted(() => {
      // 操作流优先复用会话缓存，避免切页后重复初始化。
      void store.dispatch("warehouseAdmin/ensureLogs");
    });

    const applySearch = () => {
      keyword.value = keywordInput.value.trim();
    };

    return {
      logs,
      visibleLogs,
      keywordInput,
      keyword,
      applySearch,
    };
  },
});
</script>

<style scoped>
.page {
  display: grid;
  width: 100%;
  height: 100%;
  box-sizing: border-box;
}

.panel {
  width: 100%;
  box-sizing: border-box;
  border-radius: 12px;
  padding: 18px;
  border: 1px solid #e7e9ee;
  background: #ffffff;
  box-shadow: 0 12px 28px rgba(16, 24, 40, 0.06);
}

.panel-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  margin-bottom: 16px;
}

.panel-head h3 {
  margin: 0 0 4px;
  color: #0f172a;
  font-size: 20px;
}

.panel-head span {
  color: #64748b;
  font-size: 12px;
}

.status-pill {
  padding: 10px 14px;
  border-radius: 8px;
  border: 1px solid #e7e9ee;
  background: #f1f2f5;
  color: #4f46e5;
  font-size: 12px;
  font-weight: 700;
}

.toolbar {
  margin-bottom: 14px;
}

input {
  width: min(100%, 360px);
  box-sizing: border-box;
  border: 1px solid #e7e9ee;
  border-radius: 8px;
  padding: 11px 12px;
  background: #ffffff;
  color: #0f172a;
  font-size: 13px;
}

input:focus-visible {
  outline: 3px solid rgba(79, 70, 229, 0.24);
  outline-offset: 2px;
  border-color: #4f46e5;
}

.log-list {
  display: grid;
  gap: 10px;
}

.log-item {
  display: grid;
  grid-template-columns: 88px 1fr 84px;
  gap: 14px;
  align-items: center;
  padding: 16px;
  border-radius: 10px;
  border: 1px solid #e7e9ee;
  background: #ffffff;
}

time {
  font-size: 16px;
  font-weight: 700;
  color: #4f46e5;
  letter-spacing: 0;
}

strong {
  display: block;
  color: #0f172a;
}

p {
  margin: 6px 0 0;
  color: #64748b;
}

i {
  justify-self: end;
  font-style: normal;
  padding: 8px 12px;
  border-radius: 999px;
  background: #e7f1ed;
  color: #4f46e5;
  font-size: 12px;
  font-weight: 700;
}

.empty-state {
  padding: 28px;
  border-radius: 10px;
  border: 1px dashed #e7e9ee;
  background: #f1f2f5;
  color: #64748b;
  text-align: center;
  font-size: 13px;
}

@media (max-width: 900px) {
  .log-item {
    grid-template-columns: 1fr;
  }

  i {
    justify-self: start;
  }
}
</style>
