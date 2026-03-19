<template>
  <section class="page">
    <section class="panel">
      <div class="panel-head">
        <div>
          <h3>操作日志</h3>
          <span>新增 / 更新 / 删除轨迹</span>
        </div>
        <div class="status-pill">Operation Feed</div>
      </div>

      <div class="log-list">
        <article
          v-for="entry in logs"
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
      </div>
    </section>
  </section>
</template>

<script lang="ts">
import { defineComponent, ref } from "vue";

export default defineComponent({
  name: "WarehouseAdminLogs",
  setup() {
    const logs = ref([
      {
        time: "09:20",
        title: "新增物品 · 犬用营养膏",
        description: "管理员 A-03 完成入库，数量 18，总价自动生成 ¥828.00。",
        tag: "Create",
      },
      {
        time: "11:45",
        title: "更新库存 · 麻醉针剂",
        description: "库存调整为 6，系统同步标记为低库存状态。",
        tag: "Update",
      },
      {
        time: "15:30",
        title: "删除物品 · 旧批次注射器",
        description: "执行删除确认后，记录已从库存面板移除并写入日志。",
        tag: "Delete",
      },
    ]);

    return {
      logs,
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
  border-radius: 20px;
  padding: 18px;
  border: 1px solid rgba(148, 197, 255, 0.28);
  background: linear-gradient(
    180deg,
    rgba(225, 237, 253, 0.97),
    rgba(205, 223, 247, 0.98)
  );
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
  color: #16385d;
}

.panel-head span {
  color: #587398;
  font-size: 12px;
}

.status-pill {
  padding: 10px 14px;
  border-radius: 999px;
  border: 1px solid rgba(173, 210, 255, 0.3);
  background: rgba(217, 230, 248, 0.84);
  color: #1d4d7f;
  font-size: 12px;
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
  border-radius: 16px;
  border: 1px solid rgba(176, 212, 255, 0.3);
  background: rgba(217, 230, 248, 0.84);
}

time {
  font-family: "Rajdhani", "Noto Sans SC", sans-serif;
  font-size: 24px;
  color: #1f5588;
}

strong {
  display: block;
  color: #173a60;
}

p {
  margin: 6px 0 0;
  color: #587398;
}

i {
  justify-self: end;
  font-style: normal;
  padding: 8px 12px;
  border-radius: 999px;
  background: linear-gradient(
    180deg,
    rgba(220, 240, 255, 0.98),
    rgba(198, 231, 255, 0.96)
  );
  color: #1d4d7f;
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
