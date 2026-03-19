<template>
  <section class="page">
    <div class="panel">
      <div class="panel-head">
        <div>
          <h3>待接诊队列</h3>
          <p>按照到院时间和优先等级快速排序，保持桌面式临床清单感。</p>
        </div>
        <button>刷新队列</button>
      </div>

      <table>
        <thead>
          <tr>
            <th>宠物</th>
            <th>主人</th>
            <th>主诉</th>
            <th>优先级</th>
            <th>到院时间</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in items" :key="item.id">
            <td>{{ item.petName }}</td>
            <td>{{ item.ownerName }}</td>
            <td>{{ item.symptom }}</td>
            <td>
              <span class="tag" :class="item.level">{{ item.level }}</span>
            </td>
            <td>{{ item.arrivedAt }}</td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { queueItems } from "../../api/doctorMock";

export default defineComponent({
  name: "DoctorQueue",
  setup() {
    return { items: queueItems };
  },
});
</script>

<style scoped>
.page {
  display: grid;
}

.panel {
  border: 1px solid rgba(157, 188, 178, 0.24);
  border-radius: 28px;
  background: linear-gradient(180deg, rgba(255, 253, 248, 0.96), #f6fbf8);
  padding: 22px;
  box-shadow: 0 20px 38px rgba(49, 82, 77, 0.06);
}

.panel-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  margin-bottom: 18px;
  gap: 12px;
}

.panel-head h3,
.panel-head p {
  margin: 0;
}

.panel-head p {
  margin-top: 6px;
  color: #67807b;
  line-height: 1.6;
}

button {
  border: 1px solid rgba(144, 175, 166, 0.24);
  border-radius: 16px;
  padding: 11px 16px;
  background: linear-gradient(135deg, #29565a, #7d5348);
  color: #fffdfb;
  cursor: pointer;
  box-shadow: 0 12px 24px rgba(49, 82, 87, 0.12);
}

table {
  width: 100%;
  border-collapse: collapse;
  overflow: hidden;
}

th,
td {
  text-align: left;
  padding: 14px 12px;
  border-bottom: 1px solid rgba(226, 236, 232, 0.92);
  font-size: 13px;
}

th {
  color: #6f8582;
  font-weight: 600;
}

td {
  color: #19383b;
}

tbody tr {
  background: rgba(255, 255, 255, 0.46);
}

.tag {
  display: inline-flex;
  padding: 7px 11px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 700;
}

.tag.普通 {
  background: #edf5f1;
  color: #355a53;
}

.tag.优先 {
  background: #fff1d8;
  color: #96611d;
}

.tag.紧急 {
  background: #ffe2df;
  color: #b14739;
}

@media (max-width: 960px) {
  .panel-head {
    flex-direction: column;
    align-items: flex-start;
  }

  .panel {
    overflow: auto;
  }

  table {
    min-width: 680px;
  }
}
</style>
